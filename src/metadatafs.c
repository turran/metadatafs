#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <sys/statvfs.h>

// determine the system's max path length
#ifdef PATH_MAX
    const int pathmax = PATH_MAX;
#else
    const int pathmax = 1024;
#endif

char *basepath;
int debug = 0;

static int metadatafs_readlink(const char *path, char *buf, size_t size)
{
	return 0;
}

static int metadatafs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		off_t offset, struct fuse_file_info *fi)
{
	return 0;
}

static int metadatafs_getattr(const char *path, struct stat *stbuf)
{
	int ret = 0;

	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0)
	{
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	}
	else
		ret = -ENOENT;
	return ret;
}

static int metadatafs_open(const char *path, struct fuse_file_info *fi)
{
	return 0;
}

static int metadatafs_read(const char *path, char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi)
{
	return 0;
}

static int metadatafs_statfs(const char *path, struct statvfs *stbuf)
{
	return 0;
}

static int metadatafs_release(const char *path, struct fuse_file_info *fi)
{
    
	return 0;
}

static void * metadatafs_init(struct fuse_conn_info *conn)
{
	conn->async_read = 0;

	return NULL;
}


static struct fuse_operations metadatafs_ops = {
	.getattr  = metadatafs_getattr,
	.readlink = metadatafs_readlink,
	.readdir  = metadatafs_readdir,
	.open     = metadatafs_open,
	.read     = metadatafs_read,
	.statfs   = metadatafs_statfs,
	.release  = metadatafs_release,
	.init     = metadatafs_init,
};

int main(int argc, char **argv)
{
	struct fuse_args args;

	if (argc < 2)
	{
		return 0;
	}

	basepath = strdup(argv[1]);
	argv[1] = argv[0];

	args.argc = argc - 1;
	args.argv = argv + 1;
	args.allocated = 0;

	//fuse_opt_match(args, 
	fuse_main(argc - 1, argv + 1, &metadatafs_ops, NULL);

	free(basepath);

	return 0;
}
