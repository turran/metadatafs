#include "libmetadatafs.h"
#include <id3tag.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static const char *unknown = "Unknown";

static void _tag_set_text(struct id3_tag *tag, const char *name, const char *text)
{
	struct id3_frame *frame;
	union id3_field *field;
	enum id3_field_textencoding enc;
	char *title = NULL;

	printf("setting text %s\n", text);
	frame = id3_tag_findframe(tag, name, 0);
	if (!frame)
	{
		frame = id3_frame_new(name);
		printf("creating new frame\n");
		//id3_tag_attachframe(tag, frame);
	}

	field = id3_frame_field(frame, 0);
	if (!field)
	{
		printf("no field 1\n");
		goto end;
	}
	enc = id3_field_gettextencoding(field);

	field = id3_frame_field(frame, 1);
	if (!field)
	{
		printf("no field 2\n");
		goto end;
	}

	if (enc != ID3_FIELD_TEXTENCODING_ISO_8859_1 && enc != ID3_FIELD_TEXTENCODING_UTF_8)
		goto end;

	switch (id3_field_type(field))
	{
		case ID3_FIELD_TYPE_STRING:
		id3_field_setstring(field, id3_utf8_ucs4duplicate(text));
		break;

		case ID3_FIELD_TYPE_STRINGFULL:
		id3_field_setfullstring(field, id3_utf8_ucs4duplicate(text));
		break;

		case ID3_FIELD_TYPE_STRINGLIST:
		{
			id3_ucs4_t *str;

			str = id3_utf8_ucs4duplicate(text);
			id3_field_setstrings(field, 1, &str);
		}
		break;
	}
end:
	return;
}

/* TODO rename this to set data */
static char * _tag_get_text(struct id3_tag *tag, const char *name)
{
	struct id3_frame *frame;
	union id3_field *field;
	enum id3_field_textencoding enc;
	char *title = NULL;

	frame = id3_tag_findframe(tag, name, 0);
	if (!frame) goto end;

	field = id3_frame_field(frame, 0);
	enc = id3_field_gettextencoding(field);

	field = id3_frame_field(frame, 1);
	if (!field) goto end;

	if (enc != ID3_FIELD_TEXTENCODING_ISO_8859_1 && enc != ID3_FIELD_TEXTENCODING_UTF_8)
		goto end;

	switch (id3_field_type(field))
	{
		case ID3_FIELD_TYPE_STRING:
		title = id3_ucs4_utf8duplicate(id3_field_getstring(field));
		break;

		case ID3_FIELD_TYPE_STRINGFULL:
		title = id3_ucs4_utf8duplicate(id3_field_getfullstring(field));
		break;

		case ID3_FIELD_TYPE_STRINGLIST:
		{
			unsigned int i;
			char alist[PATH_MAX];
			int nstrings;

			alist[0] = '\0';
			nstrings = id3_field_getnstrings(field);
			//printf("# strings %d\n", id3_field_getnstrings(field));
			for (i = 0; i < nstrings; i++)
			{
				char *tmp;
				id3_ucs4_t const *ucs4;

				ucs4 = id3_field_getstrings(field, i);
				if (!ucs4) continue;
				tmp = id3_ucs4_utf8duplicate(ucs4);
				if (i != 0)
					strncat(alist, " ", PATH_MAX - strlen(alist));
				strncat(alist, tmp, PATH_MAX - strlen(alist));
				free(tmp);
			}
			title = strdup(alist);
		}
		break;
	}
end:
	if (title && !libmetadatafs_name_is_empty(title))
		return title;
	else
		return strdup(unknown);
}

/******************************************************************************
 *                       Metadatafs backend interface                         *
 ******************************************************************************/
static int _supported(char *file)
{
	char *extension;

	extension = libmetadatafs_path_last_char(file, '.');
	if (!strcmp(extension, "mp3"))
		return 1;
	return 0;
}

static void * _open(char *file)
{
	struct id3_file *id3;
	id3 = id3_file_open(file, ID3_FILE_MODE_READWRITE);

	return id3;
}

static void _close(void *handle)
{
	struct id3_file *id3 = handle;

	id3_file_close(id3);
}

static char * _artist_get(void *handle)
{
	struct id3_file *id3 = handle;
	struct id3_tag *tag;

	tag = id3_file_tag(id3);
	return _tag_get_text(tag, ID3_FRAME_ARTIST);
}

static char * _title_get(void *handle)
{
	struct id3_file *id3 = handle;
	struct id3_tag *tag;

	tag = id3_file_tag(id3);
	return _tag_get_text(tag, ID3_FRAME_TITLE);
}

static char * _album_get(void *handle)
{
	struct id3_file *id3 = handle;
	struct id3_tag *tag;

	tag = id3_file_tag(id3);
	return _tag_get_text(tag, ID3_FRAME_ALBUM);
}

static void _artist_set(void *handle, char *artist)
{
	struct id3_file *id3 = handle;
	struct id3_tag *tag;

	tag = id3_file_tag(id3);
	_tag_set_text(tag, ID3_FRAME_ARTIST, artist);
	id3_file_update(id3);
}

static void _title_set(void *handle, char *title)
{
	struct id3_file *id3 = handle;
	struct id3_tag *tag;

	tag = id3_file_tag(id3);
	_tag_set_text(tag, ID3_FRAME_TITLE, title);
	id3_file_update(id3);
}

static void _album_set(void *handle, char *album)
{
	struct id3_file *id3 = handle;
	struct id3_tag *tag;
	int ret;

	tag = id3_file_tag(id3);
	_tag_set_text(tag, ID3_FRAME_ALBUM, album);
	id3_file_update(id3);
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
libmetadatafs_backend libid3tag_backend = {
	.supported = _supported,
	.open = _open,
	.close = _close,
	.artist_get = _artist_get,
	.album_get = _album_get,
	.title_get = _title_get,
	.artist_set = _artist_set,
	.album_set = _album_set,
	.title_set = _title_set,
};
