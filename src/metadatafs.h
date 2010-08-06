#ifndef _METADATAFS_H
#define _METADATAFS_H

typedef struct _metadatafs_backend
{
	int (*supported)(char *file);
	void * (*open)(char *file);
	void (*close)(void *handle);
	char * (*artist_get)(void *handle);
	char * (*title_get)(void *handle);
	char * (*album_get)(void *handle);
	/* TODO
	void (*artist_set)(void *handle, char *artist);
	void (*title_set)(void *handle, char *title);
	void (*album_set)(void *handle, char *album);
	*/
} metadatafs_backend;

int metadatafs_name_is_empty(char *str);
char * metadatafs_path_last_char(char *path, char token);

extern metadatafs_backend libid3tag_backend;

#endif
