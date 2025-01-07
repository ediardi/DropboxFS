#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

// ... //

void add_dir( const char *dir_name )
{
	// Upload dir pe Dropbox
	char command[1024];
    snprintf(command, sizeof(command), "dbxcli mkdir %s", dir_name);
    int ret = system(command);


    if (ret == -1 || WEXITSTATUS(ret) != 0) {
        fprintf(stderr, "Failed to create Dropbox folder: %s\n", dir_name);
        return -EIO;
    }

}

int is_dir( const char *path )
{
    char command[1024];
    snprintf(command, sizeof(command), "dbxcli ls %s", path);
    int ret = system(command);

	if(ret == 0)
	{
		snprintf(command, sizeof(command), "dbxcli revs %s", path);
		ret = system(command);
		if(ret!=0)
			return 1;
	}
	return 0;
}

void add_file( const char *filename )
{

	char temp_path[1024];
    snprintf(temp_path, sizeof(temp_path), "/tmp/%s", filename);	

	// Create empty file and upload to Dropbox
	FILE *local_file = fopen(temp_path, "w");
    if (!local_file) {
        perror("fopen");
        return;
    }
    fclose(local_file); 

    char command[1024];
    snprintf(command, sizeof(command), "dbxcli put %s %s", temp_path, filename);
    int ret = system(command);
    if (ret == -1) {
        perror("Error executing dbxcli command");
        return;
    } else if (WEXITSTATUS(ret) != 0) {
        fprintf(stderr, "dbxcli command failed with exit code %d\n", WEXITSTATUS(ret));
        return;
    }
    printf("File uploaded successfully: %s -> %s\n", temp_path, filename);

}

int is_file( const char *path )
{
	
	//char temp_path[1024];
    //snprintf(temp_path, sizeof(temp_path), "/tmp%s", path);

    char command[1024];
    //snprintf(command, sizeof(command), "dbxcli get %s %s", path, temp_path);
	snprintf(command, sizeof(command), "dbxcli revs %s", path);
    int ret = system(command);

	if(ret == 0)
		return 1;
	return 0;
}

void write_to_file( const char *path, const char *new_content )
{

	char temp_path[1024];
    snprintf(temp_path, sizeof(temp_path), "/tmp/%s", path);	

	// Create empty file and upload to Dropbox
	FILE *local_file = fopen(temp_path, "w");
    if (!local_file) {
        perror("fopen");
        return;
    }
    fwrite(new_content, 1, strlen(new_content), local_file);
    fclose(local_file); 

    char command[1024];
    snprintf(command, sizeof(command), "dbxcli put %s %s", temp_path, path);
    int ret = system(command);
    if (ret == -1) {
        perror("Error executing dbxcli command");
        return;
    } else if (WEXITSTATUS(ret) != 0) {
        fprintf(stderr, "dbxcli command failed with exit code %d\n", WEXITSTATUS(ret));
        return;
    }
    printf("File uploaded successfully: %s -> %s\n", temp_path, path);


}

// ... //

static int do_getattr( const char *path, struct stat *st )
{
	char command[1024];
    snprintf(command, sizeof(command), "echo attr path %s", path);
	system(command);
	//debug lines



	st->st_uid = getuid(); // The owner of the file/directory is the user who mounted the filesystem
	st->st_gid = getgid(); // The group of the file/directory is the same as the group of the user who mounted the filesystem
	st->st_atime = time( NULL ); // The last "a"ccess of the file/directory is right now
	st->st_mtime = time( NULL ); // The last "m"odification of the file/directory is right now
	
	if (strcmp(path, "/Input") == 0) {
        // Attributes for the /Input directory
        st->st_mode = S_IFDIR | 0755;  // Directory with rwxr-xr-x permissions
        st->st_nlink = 2;              // Directory link count
    } else if (strcmp(path, "/Input/output") == 0) {
        // Attributes for the /Input/output file
        st->st_mode = S_IFREG | 0644;  // Regular file with rw-r--r-- permissions
        st->st_nlink = 1;              // Regular file link count
        st->st_size = 2048;  
		// sometimes the kernel want the atributes for /Input and /Input/output.
		// we do not have these files in our structure and don't know why the kernel does this
		// but we need to return some data
	} else
	if ( strcmp( path, "/" ) == 0 || is_dir( path ) == 1 )
	//if it's root directory or another folder
	{
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2; // Why "two" hardlinks instead of "one"? The answer is here: http://unix.stackexchange.com/a/101536
		// mode set to directory
	}
	else if ( is_file( path ) == 1 )
	{
		st->st_mode = S_IFREG | 0644;
		st->st_nlink = 1;
		st->st_size = 1024;
		// mode set to file
	}
	else
	{
		system("echo nodir nofile");
		return -ENOENT;
	}
	
	return 0;
}

static int do_readdir( const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi )
{

	printf(path);
	fflush(NULL);
	system("echo readdir");
	//debug lines

	filler( buffer, ".", NULL, 0 ); // Current Directory
	filler( buffer, "..", NULL, 0 ); // Parent Directory
	//filler populates the return buffer ,respecting conventions(from which ls will get file names)
	//the second argument is the file/folder name
	//https://www.cs.hmc.edu/~geoff/classes/hmc.cs135.201109/homework/fuse/fuse_doc.html#:~:text=Readdir%20Function,path%20doesn%27t%20exist.

	if ( strcmp( path, "/" ) == 0 || is_dir( path ) == 1 )
	{
		char buf[4086];
		FILE* fp;
		char command[1024];
    	snprintf(command, sizeof(command), "dbxcli ls %s", path);
		if ((fp = popen(command, "r")) == NULL) {
			printf("Error opening pipe!\n");
			return -1;
		}
		// popen opens a virtual file that contains the output of the command as if it was displayed in terminal
		// the command run is "dbxcli ls [path]"
		// dbxcli ls returns a string with folder and file names separeated by " ", /t and /n

		char line[256];
		while (fgets(line, sizeof(line), fp)) {
			char *filename = strtok(line, " \t\n");
			while(filename != NULL)
			{
				filler(buffer, strrchr(filename,'/')+1, NULL, 0);
				//we are only interested in the realtive filename to populate the buffer
				//we don't want ls to print the absolute paths
				// file = strrchr(filename,'/')+1 (filename is the absolute path)

				printf("OUTPUT: %s\n", filename);
				fflush(NULL);
				//debug lines

				filename = strtok(NULL, " \t\n");
				//get next file path
			}
		}

		//

		if (pclose(fp)) {
			printf("Command not found or exited with error status\n");
			return -1;
		}
	}
	
	return 0;
}

static int do_read( const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi )
{

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
    size_t bytes_read = fread(buffer, 1, size, file);
    fclose(file);

    return bytes_read;
}

static int do_mkdir( const char *path, mode_t mode )
{
	char command[1024];
    snprintf(command, sizeof(command), "dbxcli mkdir %s", path);
    int ret = system(command);

	
    if (ret == -1 || WEXITSTATUS(ret) != 0) {
        fprintf(stderr, "Failed to create Dropbox folder: %s\n", path);
        return -EIO;
    }

    return 0;
}

static int do_mknod( const char *path, mode_t mode, dev_t rdev )
{
	path++;
	add_file( path );
	
	return 0;
}

static int do_write( const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *info )
{
	write_to_file( path, buffer );
	
	return size;
}

static struct fuse_operations operations = {
    .getattr	= do_getattr,
    .readdir	= do_readdir,
    .read		= do_read,
    .mkdir		= do_mkdir,
    .mknod		= do_mknod,
    .write		= do_write,
};

int main( int argc, char *argv[] )
{
	return fuse_main( argc, argv, &operations, NULL );
}