#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>

int upload_to_dropbox(const char *local_path, const char *dropbox_path) { // WORKS
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

int delete_from_dropbox(const char *dropbox_path) { // WORKS
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

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else {
        stbuf->st_mode = S_IFREG | 0644;
        stbuf->st_nlink = 1;
        stbuf->st_size = 1024; // Dummy size, should be replaced with actual file size
    }

    return 0;
}




static int do_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
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

static int do_create(const char *path, mode_t mode, struct fuse_file_info *fi) { // WORKS
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

static int do_unlink(const char *path) { // WORKS
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

static int do_mkdir(const char *path, mode_t mode) {  // WORKS
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

static int do_rmdir(const char *path) { // WORKS
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

static int do_rename(const char *from, const char *to, unsigned int flags) { //WORKS
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
    // return fuse_main(argc, argv, &dropbox_oper, NULL);
    // do_mkdir("/TestDir2", 0);
    // do_rmdir("/TestDir");
    // do_rename("/TestDir2", "/TestDir3", 0);
    



    // const char *test_path = "/testfile.txt";  // Simulated path within the mounted filesystem
    // mode_t test_mode = 0644;                 // Standard file permissions
    // struct fuse_file_info test_fi;           // Dummy struct, not used in this implementation

    // printf("Testing do_create function...\n");

    // int result = do_create(test_path, test_mode, &test_fi);
    // if (result == 0) {
    //     printf("do_create succeeded for path: %s\n", test_path);
    // } else {
    //     printf("do_create failed for path: %s with error code: %d\n", test_path, result);
    // }


    // do_unlink(test_path);


    // const char *test_path = "/testfile.txt"; // Simulated path
    // const char *local_test_path = "testfile.txt"; // Corresponding local file
    // char write_buf[] = "This is a test data for write operation.";
    // size_t write_size = sizeof(write_buf);
    // char read_buf[1024]; // Buffer to read into
    // off_t offset = 0; // Start from the beginning of the file
    // struct fuse_file_info dummy_fi; // Dummy file_info struct
    
    // // Step 1: Test do_write
    // printf("Testing do_write...\n");
    // int write_result = do_write(test_path, write_buf, write_size, offset, &dummy_fi);
    // if (write_result > 0) {
    //     printf("do_write succeeded, wrote %d bytes\n", write_result);
    // } else {
    //     printf("do_write failed with error code: %d\n", write_result);
    // }

    // // Step 2: Test do_read
    // printf("\nTesting do_read...\n");
    // int read_result = do_read(test_path, read_buf, sizeof(read_buf), offset, &dummy_fi);
    // if (read_result > 0) {
    //     printf("do_read succeeded, read %d bytes: %.*s\n", read_result, read_result, read_buf);
    // } else {
    //     printf("do_read failed with error code: %d\n", read_result);
    // }


    return fuse_main(argc, argv, &dropbox_oper, NULL);

    // return 0;
}
