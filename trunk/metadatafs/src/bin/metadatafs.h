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
#include <sys/types.h>
#include <pwd.h>

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
	unsigned int artist;
};

struct _Mdfs_Title
{
	unsigned int id;
	char *name;
	unsigned int album;
};

struct _Mdfs_File
{
	unsigned int id;
	char *path;
	time_t mtime;
	unsigned int title;
};

/* album model */
Mdfs_Album * mdfs_album_get_from_id(sqlite3 *db, unsigned int id);
Mdfs_Album * mdfs_album_get_from_name(sqlite3 *db, const char *name);
Mdfs_Album * mdfs_album_get(sqlite3 *db, const char *name, unsigned int artist);
Mdfs_Album * mdfs_album_new(sqlite3 *db, const char *name, unsigned int artist);
void mdfs_album_free(Mdfs_Album *album);
int mdfs_album_init(sqlite3 *db);

/* title model */
Mdfs_Title * mdfs_title_get_from_id(sqlite3 *db, unsigned int id);
Mdfs_Title * mdfs_title_get_from_name(sqlite3 *db, const char *name);
Mdfs_Title * mdfs_title_get(sqlite3 *db, const char *name, unsigned int album);
Mdfs_Title * mdfs_title_new(sqlite3 *db, const char *name, unsigned int album);
void mdfs_title_free(Mdfs_Title *title);
int mdfs_title_init(sqlite3 *db);

/* file model */
Mdfs_File * mdfs_file_get_from_id(sqlite3 *db, unsigned int id);
Mdfs_File * mdfs_file_get(sqlite3 *db, const char *path, time_t mtime, unsigned int title);
Mdfs_File * mdfs_file_new(sqlite3 *db, const char *path, time_t mtime, unsigned int title);
void mdfs_file_free(Mdfs_File *file);
int mdfs_file_init(sqlite3 *db);

/* artist model */
Mdfs_Artist * mdfs_artist_get_from_id(sqlite3 *db, unsigned int id);
Mdfs_Artist * mdfs_artist_get(sqlite3 *db, const char *name);
Mdfs_Artist * mdfs_artist_new(sqlite3 *db, const char *name);
void mdfs_artist_free(Mdfs_Artist *artist);
int mdfs_artist_init(sqlite3 *db);

#endif
