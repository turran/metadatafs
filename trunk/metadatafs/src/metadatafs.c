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
#include <id3tag.h>

/* whenever a query fails with no results with should go back on the query and go
 * deleting entries on the database whenever the mtime of a file has changed, get
 * the id3 and store again the info
 */

static char *basepath;
static int debug = 0;
static sqlite3 *db;

const char *_fields[] = {
	"Artist",
	"Title",
	"Album",
	"Genre",
};

const char *unknown = "Unknown";

typedef enum metadatafs_fields
{
	METADATAFS_ARTIST = (1 << 0),
	METADATAFS_TITLE =  (1 << 1),
	METADATAFS_ALBUM =  (1 << 2),
	METADATAFS_GENRE =  (1 << 3),
} metadatafs_fields;

#define METADATAFS_FIELDS 4

typedef struct _metadatafs_query
{
	char entries[METADATAFS_FIELDS][PATH_MAX];
	metadatafs_fields fields;
} metadatafs_query;

static char * _path_last_char(char *path, char token)
{
	char *tmp;

	tmp = path + strlen(path) - 1;
	while (tmp >= path && *tmp != token) tmp--;

	return tmp + 1;
}

static void _path_to_query(char *path, metadatafs_query *q)
{
	char *token;

	memset(q, 0, sizeof(metadatafs_query));
	token = strtok(path, "/");
        if (!token)
		return;
	do
	{
		int i;

		for (i = 0; i < METADATAFS_FIELDS; i++)
		{
			if (!strncmp(token, _fields[i], strlen(_fields[i])))
			{
				/* get next token for */
				token = strtok(NULL, "/");
				if (!token) return;

				q->fields |= (1 << i);
				strncpy(q->entries[i], token, PATH_MAX);
			}
		}
	} while (token = strtok(NULL, "/"));
}

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

			alist[0] = '\0';
			printf("# strings %d\n", id3_field_getnstrings(field));
			for (i = 0; i < id3_field_getnstrings(field); i++)
			{
				char *tmp;

				tmp = id3_ucs4_utf8duplicate(id3_field_getstrings(field, i));
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
	return title ? title : strdup(unknown);

}

static inline char * _tag_get_title(struct id3_tag *tag)
{
	return _tag_get_text(tag, ID3_FRAME_TITLE);
}

static inline char * _tag_get_artist(struct id3_tag *tag)
{
	return _tag_get_text(tag, ID3_FRAME_ARTIST);
}

/* given a path like /artist/foo/album/bar mark artist and album fields */
static void _path_mark_fields(char *path, metadatafs_fields *f)
{
	char *token;

	*f = 0;
	token = strtok(path, "/");
        if (!token)
		return;
	do
	{
		int i;

		for (i = 0; i < METADATAFS_FIELDS; i++)
		{
			if (!strncmp(token, _fields[i], strlen(_fields[i])))
			{
				*f |= (1 << i);
			}
		}
	} while (token = strtok(NULL, "/"));
}

static int metadatafs_readlink(const char *path, char *buf, size_t size)
{
	return 0;
}

static int metadatafs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		off_t offset, struct fuse_file_info *fi)
{
	metadatafs_fields f;
	char *tmp;
	int i = 0;

	tmp = strdup(path);
	_path_mark_fields(tmp, &f);
	free(tmp);
	/* invert the flags found on the path */
	f = ~f & ((1 << METADATAFS_FIELDS) - 1);
	/* append default directories */
	while (f)
	{
		if (f & 0x1)
		{
			if (filler(buf, _fields[i], NULL, 0))
				break;
		}
		f >>= 1;
		i++;
	}

	/* add simple '.' and '..' files */
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	/* given the path, select the needed artist/album/whatever */

	return 0;
}

static int metadatafs_getattr(const char *path, struct stat *stbuf)
{
	int ret = 0;

	memset(stbuf, 0, sizeof(struct stat));
	if (!strcmp(path, "/"))
	{
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	}
	else
	{
		char *file;
		int is_field = 0;
		int i;

		file = _path_last_char(path, '/');
		for (i = 0; i < METADATAFS_FIELDS; i++)
		{
			if (!strncmp(file, _fields[i], strlen(_fields[i])))
			{
				stbuf->st_mode = S_IFDIR | 0755;
				stbuf->st_nlink = 2;
				is_field = 1;
				break;
			}
		}
		/* regular file or entry on the db */
		if (!is_field)
		{

		}
	}
	/* check the last metadata directory */
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
static void db_cleanup(void)
{
	sqlite3_close(db);
}

static void db_insert_artist(const char *artist)
{
	sqlite3_stmt *stmt;
	const char *tail;
	int error;

	printf("inserting artist %s\n", artist);

}

static void db_insert_album(const char *album)
{
	printf("inserting album %s\n", album);
}

static void db_insert_track(const char *track, const char *file, time_t mtime)
{
	/* check if the file exists,, if so check the mtime and compare */
	printf("file found %s %s %ld\n", track, file, mtime);
}

static int db_setup(void)
{
	sqlite3_stmt *stmt;
	const char *tail;
	int error;
	/* we should generate the database here
	 * in case it already exists, just
	 * compare mtimes of files
	 */
	if (sqlite3_open("/tmp/metatagfs.db", &db) != SQLITE_OK)
	{
		printf("could not open the db\n");
		return 0;
	}

	error = sqlite3_prepare(db, "CREATE TABLE IF NOT EXISTS artist (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT);",
			-1, &stmt, &tail);
	if (error != SQLITE_OK)
	{
		printf("error artist\n");
		return 0;
	}
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	error = sqlite3_prepare(db, "CREATE TABLE IF NOT EXISTS album (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT);",
			-1, &stmt, &tail);
	if (error != SQLITE_OK)
	{
		printf("error album\n");
		return 0;
	}
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	error = sqlite3_prepare(db, "CREATE TABLE IF NOT EXISTS track (id INTEGER PRIMARY KEY AUTOINCREMENT, file TEXT, mtime INTEGER);",
			-1, &stmt, &tail);
	if (error != SQLITE_OK)
	{
		printf("error track\n");
		return 0;
	}
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	return 1;
}

static void * metadatafs_init(struct fuse_conn_info *conn)
{
	DIR *dp;
	struct dirent *de;

	/* setup the connection info */
	conn->async_read = 0;

	/* TODO read the database */
	/* TODO update it */

	dp = opendir(basepath);
	if (!dp) return NULL;

	if (!db_setup()) return NULL;

	while ((de = readdir(dp)) != NULL)
	{
		char *extension;

		/* TODO also check subdirs */
		extension = _path_last_char(de->d_name, '.');
		if (!strcmp(extension, "mp3"))
		{
			char realfile[PATH_MAX];
			struct id3_file *file;
			struct id3_tag *tag;
			struct stat st;
			char *artist;
			char *album;
			char *title;

			strncpy(realfile, basepath, PATH_MAX);
			strncat(realfile, de->d_name, PATH_MAX - strlen(de->d_name));

			stat(realfile, &st);
			file = id3_file_open(realfile, ID3_FILE_MODE_READONLY);
			tag = id3_file_tag(file);
			artist = _tag_get_artist(tag);
			db_insert_artist(artist);
			free(artist);
			title = _tag_get_title(tag);
			db_insert_track(title, realfile, st.st_mtime);
			free(title);
			id3_file_close(file);
		}
	}
	closedir(dp);

	return NULL;
}

static void metadatafs_destroy(void *data)
{
	db_cleanup();
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
