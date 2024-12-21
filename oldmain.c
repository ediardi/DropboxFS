#define FUSE_USE_VERSION 30

#include <fuse3/fuse.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>

int upload_to_dropbox(const char *local_path, const char *dropbox_path) {
    char command[1024];
    snprintf(command, sizeof(command), "dbxcli put %s %s", local_path, dropbox_path);
    int ret = system(command);
    if (ret == -1) {
        perror("Error executing dbxcli command");
        return -1;
    } else if (WEXITSTATUS(ret) != 0) {
        fprintf(stderr, "dbxcli command failed with exit code %d\n", WEXITSTATUS(ret));
        return -1;
    }
    printf("File uploaded successfully: %s -> %s\n", local_path, dropbox_path);
    return 0;
}

int delete_from_dropbox(const char *dropbox_path) {
    char command[1024];
    snprintf(command, sizeof(command), "dbxcli rm %s", dropbox_path);
    int ret = system(command);
    if (ret == -1) {
        perror("Error executing dbxcli command");
        return -1;
    } else if (WEXITSTATUS(ret) != 0) {
        fprintf(stderr, "dbxcli command failed with exit code %d\n", WEXITSTATUS(ret));
        return -1;
    }
    printf("File deleted successfully: %s\n", dropbox_path);
    return 0;
}

static int do_getattr(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));

    printf( "[getattr] Called\n" );
	printf( "\tAttributes of %s requested\n", path );
		
	stbuf->st_uid = getuid(); // The owner of the file/directory is the user who mounted the filesystem
	stbuf->st_gid = getgid(); // The group of the file/directory is the same as the group of the user who mounted the filesystem
	stbuf->st_atime = time( NULL ); // The last "a"ccess of the file/directory is right now
	stbuf->st_mtime = time( NULL ); 
    
    char temp_path[1024];
    snprintf(temp_path, sizeof(temp_path), "/tmp%s", path);

    char command[1024];
    snprintf(command, sizeof(command), "dbxcli get %s %s", path, temp_path);
    int ret = system(command);

    if (ret != 0) {
        stbuf->st_mode = 0x4000 | 0755;
        stbuf->st_nlink = 2;
    }
    else
    {
        return stat(temp_path,&stbuf);
    }
    return 0;
}

static int do_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {

    printf( "--> Getting The List of Files of %s\n", path );
    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    char command[1024];
    snprintf(command, sizeof(command), "dbxcli ls %s > /tmp/dropbox_ls.txt", path);
    int ret = system(command);

    if (ret != 0) {
        fprintf(stderr, "Failed to list Dropbox folder: %s\n", path);
        return -EIO;
    }

    FILE *ls_file = fopen("/tmp/dropbox_ls.txt", "r");
    if (!ls_file) {
        perror("fopen");
        return -EIO;
    }

    char line[256];
    while (fgets(line, sizeof(line), ls_file)) {
        char *filename = strtok(line, " \t\n");
        if (filename) {
            filler(buf, filename, NULL, 0, 0);
        }
    }
    fclose(ls_file);

    return 0;
}


static int do_open(const char *path, struct fuse_file_info *fi) {
    char command[1024];
    snprintf(command, sizeof(command), "dbxcli stat %s > /dev/null 2>&1", path);
    int ret = system(command);

    if (ret != 0) {
        fprintf(stderr, "File not found in Dropbox: %s\n", path);
        return -ENOENT;
    }

    return 0;
}

static int do_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {

    printf( "--> Trying to read %s, %u, %u\n", path, offset, size );

    char temp_path[1024];
    snprintf(temp_path, sizeof(temp_path), "/tmp%s", path);

    char command[1024];
    snprintf(command, sizeof(command), "dbxcli get %s %s", path, temp_path);
    int ret = system(command);

    if (ret != 0) {
        fprintf(stderr, "Failed to download file: %s\n", path);
        return -EIO;
    }

    FILE *file = fopen(temp_path, "r");
    if (!file) {
        perror("fopen");
        return -EIO;
    }

    fseek(file, offset, SEEK_SET);
    size_t bytes_read = fread(buf, 1, size, file);
    fclose(file);

    return bytes_read;
}


static int do_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    FILE *local_file = fopen(path + 1, "w"); // + 1 renunta la /
    if (!local_file) {
        perror("fopen");
        return -errno;
    }
    fwrite(buf, 1, size, local_file);
    fclose(local_file);

    if (upload_to_dropbox(path + 1, path) != 0) {
        fprintf(stderr, "Failed to upload %s to Dropbox\n", path);
        return -EIO;
    }

    return size;
}

static int do_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    FILE *local_file = fopen(path + 1, "w"); // + 1 renunta la /
    if (!local_file) {
        perror("fopen");
        return -errno;
    }
    fclose(local_file);

    if (upload_to_dropbox(path + 1, path) != 0) {
        fprintf(stderr, "Failed to upload %s to Dropbox\n", path);
        return -EIO;
    }

    return 0;
}

static int do_unlink(const char *path) {
    if (unlink(path + 1) != 0) {
        perror("unlink");
        return -errno;
    }

    if (delete_from_dropbox(path) != 0) {
        fprintf(stderr, "Failed to delete %s from Dropbox\n", path);
        return -EIO;
    }

    return 0;
}

static int do_mkdir(const char *path, mode_t mode) {
    if (mkdir(path + 1, mode) != 0) {
        perror("mkdir");
        return -errno;
    }

    char command[1024];
    snprintf(command, sizeof(command), "dbxcli mkdir %s", path);
    int ret = system(command);
    if (ret == -1 || WEXITSTATUS(ret) != 0) {
        fprintf(stderr, "Failed to create Dropbox folder: %s\n", path);
        return -EIO;
    }

    return 0;
}

static int do_rmdir(const char *path) {
    if (rmdir(path + 1) != 0) {
        perror("rmdir");
        return -errno;
    }

    if (delete_from_dropbox(path) != 0) {
        fprintf(stderr, "Failed to delete Dropbox folder: %s\n", path);
        return -EIO;
    }

    return 0;
}

static int do_rename(const char *from, const char *to, unsigned int flags) {
    if (rename(from + 1, to + 1) != 0) {
        perror("rename");
        return -errno;
    }

    char command[1024];
    snprintf(command, sizeof(command), "dbxcli mv %s %s", from, to);
    int ret = system(command);
    if (ret == -1 || WEXITSTATUS(ret) != 0) {
        fprintf(stderr, "Failed to rename %s to %s on Dropbox\n", from, to);
        return -EIO;
    }

    return 0;
}

static struct fuse_operations dropbox_oper = {
        .getattr = do_getattr,
        .readdir = do_readdir,
        .open = do_open,
        .read = do_read,
        .write = do_write,
        .create = do_create,
        .unlink = do_unlink,
        .mkdir = do_mkdir,
        .rmdir = do_rmdir,
        .rename = do_rename,
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &dropbox_oper, NULL);
}
