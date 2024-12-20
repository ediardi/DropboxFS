#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

int upload_to_dropbox(const char *local_path, const char *dropbox_path) {
    char command[1024];

    // Construct the dbxcli command
    snprintf(command, sizeof(command), "dbxcli put %s %s", local_path, dropbox_path);

    // Execute the command
    int ret = system(command);

    // Check for errors
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

    // Construct the dbxcli delete command
    snprintf(command, sizeof(command), "dbxcli rm %s", dropbox_path);

    // Execute the command
    int ret = system(command);

    // Check for errors
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
    /* 
        path = path to the folder
        stbuf = struct stat with metadata abaout the file
    */
    (void) fi; // Unused parameter
    memset(stbuf, 0, sizeof(struct stat)); // Initialize struct with 0
    if (strcmp(path, "/") == 0) { 
        // Root directory
        stbuf->st_mode = S_IFDIR | 0755;
        /*
            S_IFDIR = it is a directory

            Permissions: 
                        0 - Something for initialization idk
                        Owner : 7 = Read, Write, Execute
                        Group : 5 = Read, Execute
                        Others: 5 = Read, Execute
         */ 
        stbuf->st_nlink = 2; // A directory has at least 2 links
        // (.) for itself and (..) for its parent
    } else {
         // Any other file
        stbuf->st_mode = S_IFREG | 0644;
        /*
            S_IFREG = regular file
            Permissions:
                        0 - idk
                        Owner : 6 = Read, Write
                        Group : 4 = Read
                        Others: 4 = Read
         */

        stbuf->st_nlink = 1; // One link by default
        stbuf->st_size = 0; // Set the size of the file to 0 bytes
    }
    return 0;
}

static int do_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    // Implement readdir using Dropbox API
    return 0;
}

static int do_open(const char *path, struct fuse_file_info *fi) {
    // Implement open using Dropbox API
    return 0;
}

static int do_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    // Implement read using Dropbox API
    return 0;
}

static int do_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    // Implement write using Dropbox API
    return 0;
}

static int do_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    // Implement create using Dropbox API
    return 0;
}

static int do_unlink(const char *path) {
    // Implement unlink using Dropbox API
    return 0;
}

static int do_mkdir(const char *path, mode_t mode) {
    // Implement mkdir using Dropbox API
    return 0;
}

static int do_rmdir(const char *path) {
    // Implement rmdir using Dropbox API
    return 0;
}

static int do_rename(const char *from, const char *to, unsigned int flags) {
    // Implement rename using Dropbox API
    return 0;
}

static struct fuse_operations dropboxfs_oper = {
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
    const char *local_path = "testfile.txt";
    const char *dropbox_path = "/testfile.txt";

    // Create a dummy local file to upload
    FILE *f = fopen(local_path, "w");
    if (!f) {
        perror("Failed to create local file");
        return 1;
    }
    fprintf(f, "Hello, Dropbox! This is a test file.\n");
    fclose(f);

    if (upload_to_dropbox(local_path, dropbox_path) == 0) {
        printf("File upload succeeded!\n");
    } else {
        printf("File upload failed.\n");
    }

    // if (delete_from_dropbox(dropbox_path) == 0) {
    //     printf("File deletion succeeded!\n");
    // } else {
    //     printf("File deletion failed.\n");
    // }
    return fuse_main(argc, argv, &dropboxfs_oper, NULL);
}