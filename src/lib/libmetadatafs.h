#ifndef _LIBMETADATAFS_H
#define _LIBMETADATAFS_H

typedef struct _libmetadatafs_backend
{
	int (*supported)(char *file);
	void * (*open)(char *file);
	void (*close)(void *handle);
	char * (*artist_get)(void *handle);
	char * (*title_get)(void *handle);
	char * (*album_get)(void *handle);
	void (*artist_set)(void *handle, char *artist);
	void (*title_set)(void *handle, char *title);
	void (*album_set)(void *handle, char *album);
} libmetadatafs_backend;

void * libmetadatafs_open(char *file);
void libmetadatafs_close(void *handle);
char * libmetadatafs_artist_get(void *handle);
char * libmetadatafs_album_get(void *handle);
char * libmetadatafs_title_get(void *handle);
void libmetadatafs_artist_set(void *handle, char *artist);
void libmetadatafs_album_set(void *handle, char *album);
void libmetadatafs_title_set(void *handle, char *title);

int metadatafs_name_is_empty(char *str);
char * metadatafs_path_last_char(char *path, char token);

#endif
