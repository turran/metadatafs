#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include "libmetadatafs.h"

extern libmetadatafs_backend libid3tag_backend;
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static libmetadatafs_backend *_backend = &libid3tag_backend;
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/******************************************************************************
 *                                 Helpers                                    *
 ******************************************************************************/
int libmetadatafs_name_is_empty(char *str)
{
	char *tmp = str;

	/* empty */
	if (*str == '\0') return 1;
	/* only spaces */
	while (tmp && *tmp == ' ') tmp++;
	if (tmp == str + strlen(str) + 1) return 1;
	else return 0;
}

char * libmetadatafs_path_last_char(char *path, char token)
{
	char *tmp;

	tmp = path + strlen(path) - 1;
	while (tmp >= path && *tmp != token) tmp--;

	return tmp + 1;
}
/******************************************************************************
 *                                 Backend                                    *
 ******************************************************************************/
void * libmetadatafs_open(char *file)
{
	if (!_backend->supported(file)) return NULL;
	return _backend->open(file);

}

void libmetadatafs_close(void *handle)
{
	_backend->close(handle);
}

char * libmetadatafs_artist_get(void *handle)
{
	return _backend->artist_get(handle);
}

char * libmetadatafs_album_get(void *handle)
{
	return _backend->album_get(handle);

}

char * libmetadatafs_title_get(void *handle)
{
	return _backend->title_get(handle);
}

void libmetadatafs_artist_set(void *handle, char *artist)
{
	return _backend->artist_set(handle, artist);
}

void libmetadatafs_album_set(void *handle, char *album)
{
	return _backend->album_set(handle, album);

}

void libmetadatafs_title_set(void *handle, char *title)
{
	return _backend->title_set(handle, title);
}

