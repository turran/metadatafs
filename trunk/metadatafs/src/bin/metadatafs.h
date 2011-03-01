#ifndef _METADATAFS_H
#define _METADATAFS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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
#include <sqlite3.h>
#include <pthread.h>

#if HAVE_INOTIFY
#include <sys/inotify.h>
/* size of the event structure, not counting name */
#define EVENT_SIZE  (sizeof (struct inotify_event))
/* reasonable guess as to size of 1024 events */
#define BUF_LEN        1024 * (EVENT_SIZE + 16)
#endif

typedef struct _Mdfs_Album Mdfs_Album;
typedef struct _Mdfs_Artist Mdfs_Artist;
typedef struct _Mdfs_Title Mdfs_Title;
typedef struct _Mdfs_File Mdfs_File;

struct _Mdfs_Artist
{
	unsigned int id;
	char *name;
};

struct _Mdfs_Album
{
	unsigned int id;
	char *name;
	Mdfs_Artist *artist;
};

struct _Mdfs_Title
{
	unsigned int id;
	char *name;
	Mdfs_Album *album;
};

struct _Mdfs_File
{
	unsigned int id;
	char *path;
	time_t mtime;
	Mdfs_Title *title;
};

#endif
