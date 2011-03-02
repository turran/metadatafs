#include "metadatafs.h"
#include "libmetadatafs.h"

#define BLOCK

/*
 * TODO
 * - whenever a query fails with no results with should go back on the query and go
 * deleting entries on the database whenever the mtime of a file has changed, get
 * the id3 and store again the info
 * - on mkdir() under a tag, create such entry on the database
 * - on rmdir() under a tag, delete such entry on the database
 * - whenever a file is moved into another directory, update the metadata of the
 * file
 * - add a header file
 * - move the libid3tag into a backend
 *
 * - artist
 * -- album
 * --- title
 * ---- file
 *
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static char *basepath;
static int debug = 0;
static sqlite3 *db;

typedef struct _metadatafs
{
	pthread_mutex_t lock;
	pthread_mutex_t debug_lock;
	char *basepath;
	pthread_t scanner;
#if HAVE_INOTIFY
	pthread_t monitor;
	int inotify_fd;
	int inotify_wd;
#endif
} metadatafs;

const char *_fields[] = {
	"Artist",
	"Title",
	"Album",
	"Files",
	"Genre",
};

const char *_tables[] = {
	"artist",
	"title",
	"album",
	"files",
	"genre",
};

typedef enum metadatafs_fields
{
	FIELD_ARTIST,
	FIELD_TITLE,
	FIELD_ALBUM,
	FIELD_FILES,
	FIELD_GENRE,
	FIELDS
} metadatafs_fields;

typedef enum metadatafs_mask
{
	MASK_ARTIST = (1 << FIELD_ARTIST),
	MASK_TITLE =  (1 << FIELD_TITLE),
	MASK_ALBUM =  (1 << FIELD_ALBUM),
	MASK_FILES =  (1 << FIELD_FILES),
	MASK_GENRE =  (1 << FIELD_GENRE),
} metadatafs_mask;

typedef struct _metadatafs_query
{
	char entries[FIELDS][PATH_MAX];
	metadatafs_mask fields;
	int last_field;
	int last_is_field;
} metadatafs_query;

/******************************************************************************
 *                                  Queries                                   *
 ******************************************************************************/
static int _path_to_query(char *path, metadatafs_query *q)
{
	char *token;

	memset(q, 0, sizeof(metadatafs_query));
	q->last_is_field = -1;
	token = strtok(path, "/");
        if (!token) return 1;

	do
	{
		int i;
		int is_field = 0;

		for (i = 0; i < FIELDS; i++)
		{
			if (!strcmp(token, _fields[i]))
			{
				is_field = 1;
				/* mark the last field */
				q->last_field = i;
				q->fields |= (1 << i);
				/* get next token for the value */
				token = strtok(NULL, "/");
				if (!token)
				{
					q->last_is_field = i;
					return 1;
				}
				strncpy(q->entries[i], token, PATH_MAX);
			}
		}
		if (!is_field)
			return 0;
	} while (token = strtok(NULL, "/"));

	return 1;
}

/* FIXME we should fix this function, horrible if's */
static char * _query_to_string(metadatafs_query *q)
{
	char str[4096];
	metadatafs_mask m;

	str[0] = '\0';
	if (q->last_is_field == FIELD_ARTIST)
	{
		sprintf(str, "SELECT DISTINCT %s.name FROM artist AS %s", _fields[FIELD_ARTIST], _fields[FIELD_ARTIST]);
		if (q->fields & MASK_FILES)
		{
			sprintf(str, "%s JOIN album,title,files WHERE album.artist = artist.id AND title.album = album.id and and files.title = title.id", str);
		}
		else if (q->fields & MASK_TITLE)
		{
			sprintf(str, "%s JOIN album,title WHERE album.artist = artist.id AND title.album = album.id", str);
		}
		else if (q->fields & MASK_ALBUM)
		{
			sprintf(str, "%s JOIN album WHERE album.artist = artist.id", str);
		}
	}
	else if (q->last_is_field == FIELD_ALBUM)
	{
		sprintf(str, "SELECT DISTINCT %s.name FROM album AS %s", _fields[FIELD_ALBUM], _fields[FIELD_ALBUM]);
		if (q->fields & MASK_ARTIST)
		{
			if (q->fields & MASK_FILES)
			{
				sprintf(str, "%s JOIN artist,title,files WHERE artist.id = album.artist = artist.id AND title.album = album.id and and files.title = title.id", str);
			}
			else if (q->fields & MASK_TITLE)
			{
				sprintf(str, "%s JOIN artist,title WHERE album.artist = artist.id AND title.album = album.id", str);
			}
			else
			{
				sprintf(str, "%s JOIN artist WHERE album.artist = artist.id", str);
			}
		}
		else if (q->fields & MASK_FILES)
		{
			sprintf(str, "%s JOIN title,files WHERE title.album = album.id and and files.title = title.id", str);
		}
		else if (q->fields & MASK_TITLE)
		{
			sprintf(str, "%s JOIN title WHERE title.album = album.id", str);
		}
	}
	else if (q->last_is_field == FIELD_TITLE)
	{
		sprintf(str, "SELECT DISTINCT %s.name FROM title AS %s", _fields[FIELD_TITLE], _fields[FIELD_TITLE]);
		if (q->fields & MASK_FILES)
		{
			if (q->fields & MASK_ALBUM)
			{
				sprintf(str, "%s JOIN files,album WHERE files.title = title.id AND album.id = title.album", str);
			}
			else if (q->fields & MASK_ARTIST)
			{
				sprintf(str, "%s JOIN files,album,artist WHERE files.title = title.id AND album.artist = artist.id AND title.album = album.id", str);
			}
			else
			{
				sprintf(str, "%s JOIN files WHERE files.title = title.id", str);
			}
		}
		else if (q->fields & MASK_ALBUM)
		{
			sprintf(str, "%s JOIN album WHERE album.id = title.album", str);
		}
		else if (q->fields & MASK_ARTIST)
		{
			sprintf(str, "%s JOIN album,artist WHERE album.artist = artist.id AND title.album = album.id", str);
		}
	}
	else if (q->last_is_field == FIELD_FILES)
	{
		sprintf(str, "SELECT DISTINCT %s.id FROM files AS %s", _fields[FIELD_FILES], _fields[FIELD_FILES]);
		if (q->fields & MASK_ARTIST)
		{
			sprintf(str, "%s JOIN artist,album,title WHERE artist.id = album.artist AND album.id = title.album AND files.title = title.id", str);
		}
		else if (q->fields & MASK_ALBUM)
		{
			sprintf(str, "%s JOIN album,title WHERE album.id = title.album AND files.title = title.id", str);
		}
		else if (q->fields & MASK_TITLE)
		{
			sprintf(str, "%s JOIN title WHERE files.title = title.id", str);
		}
	}
	m = q->fields & ~(1 << q->last_is_field);
	if (m)
	{
		int i;

		/* the conditionals */
		for (i = 0; i < FIELDS; i++)
		{
			if (m & (1 << i))
			{
				if (i == FIELD_FILES)
					sprintf(str, "%s AND %s.file = '%s'", str, _fields[i], q->entries[i]);
				else
					sprintf(str, "%s AND %s.name = '%s'", str, _fields[i], q->entries[i]);
			}
		}
	}
	//printf("QUERY = %s\n", str);
	return strdup(str);
}

static void _query_dump(metadatafs_query *q)
{
	printf("flags = %08x\n", q->fields);
	printf("artist = %s\n", q->fields & MASK_ARTIST ? q->entries[FIELD_ARTIST] : "<none>");
	printf("album = %s\n", q->fields & MASK_ALBUM ? q->entries[FIELD_ALBUM] : "<none>");
	printf("title = %s\n", q->fields & MASK_TITLE ? q->entries[FIELD_TITLE] : "<none>");
	printf("file = %s\n", q->fields & MASK_FILES ? q->entries[FIELD_FILES] : "<none>");
	printf("last_is_field = %d\n", q->last_is_field);
}

static inline char * _get_last_delim(const char *start, const char *end, char delim)
{
	char *tmp;

	for (tmp = end; tmp >= start; tmp--)
	{
		if (*tmp == delim)
			break;
	}
	return ++tmp;
}

static inline char * _get_last_field(const char *str, int *field)
{
	char *prv;
	char *tmp;
	int i;

	prv = tmp = str + strlen(str);
again:
	tmp = _get_last_delim(str, tmp, '/');
	for (i = 0; i < FIELDS; i++)
	{
		if (!strncmp(tmp, _fields[i], strlen(_fields[i])))
			break;
	}
	if (i == FIELDS && tmp >= str + 2)
	{
		prv = tmp;
		tmp -= 2;
		goto again;
	}
	*field = i;
	return prv;
}
/******************************************************************************
 *                                 Database                                   *
 ******************************************************************************/
static void db_cleanup(void)
{
	sqlite3_close(db);
}

static int db_insert_artist(const char *artist)
{
	char *str;
	sqlite3_stmt *stmt;
	const char *tail;
	int error;
	int id = -1;

	/* insert the new artist */
	str = sqlite3_mprintf("INSERT OR IGNORE INTO artist (name) VALUES ('%q');",
			artist);
	error = sqlite3_prepare(db, str, -1, &stmt, &tail);
	sqlite3_free(str);
	if (error != SQLITE_OK)
	{
		printf("error artist %s\n", artist);
	}
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	/* get the id */
	str = sqlite3_mprintf("SELECT id FROM artist WHERE name = '%q';",
			artist);
	error = sqlite3_prepare(db, str, -1, &stmt, &tail);
	sqlite3_free(str);
	if (error != SQLITE_OK)
	{
		printf("error artist %s\n", artist);
		return id;
	}
	if (sqlite3_step(stmt) != SQLITE_ROW)
	{
		printf("error querying artist\n");
		return id;
	}
	id = sqlite3_column_int(stmt, 0);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	return id;
}

static int db_insert_album(const char *album, int aid)
{
	char *str;
	sqlite3_stmt *stmt;
	const char *tail;
	int error;
	int id = -1;

	/* insert the new album in case it does not exist already */
	str = sqlite3_mprintf("SELECT id FROM album WHERE name = '%q' AND artist = %d;",
			album, aid);
	error = sqlite3_prepare(db, str, -1, &stmt, &tail);
	sqlite3_free(str);
	if (error != SQLITE_OK)
	{
		printf("error album %s\n", album);
		return id;
	}
	if (sqlite3_step(stmt) != SQLITE_ROW)
	{
		sqlite3_finalize(stmt);
		str = sqlite3_mprintf("INSERT OR IGNORE INTO album (name, artist) VALUES ('%q',%d);",
				album, aid);
		error = sqlite3_prepare(db, str, -1, &stmt, &tail);
		sqlite3_free(str);
		if (error != SQLITE_OK)
		{
			printf("error album %s\n", album);
		}
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
		str = sqlite3_mprintf("SELECT id FROM album WHERE name = '%q' AND artist = %d;",
				album, aid);
		error = sqlite3_prepare(db, str, -1, &stmt, &tail);
		sqlite3_free(str);
		if (error != SQLITE_OK)
		{
			printf("error album %s\n", album);
			return id;
		}
		if (sqlite3_step(stmt) != SQLITE_ROW)
		{
			printf("error querying album %s %d\n", album, aid);
			return id;
		}
	}
	id = sqlite3_column_int(stmt, 0);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	return id;
}

static int db_insert_title(const char *title, int album)
{
	char *str;
	sqlite3_stmt *stmt;
	const char *tail;
	int error;
	int id = -1;

	/* insert the new album in case it does not exist already */
	str = sqlite3_mprintf("SELECT id FROM title WHERE name = '%q' AND album = %d;",
			title, album);
	error = sqlite3_prepare(db, str, -1, &stmt, &tail);
	sqlite3_free(str);
	if (error != SQLITE_OK)
	{
		printf("1 error title %s\n", title);
		return id;
	}
	if (sqlite3_step(stmt) != SQLITE_ROW)
	{
		sqlite3_finalize(stmt);
		str = sqlite3_mprintf("INSERT OR IGNORE INTO title (name, album) VALUES ('%q',%d);",
				title, album);
		error = sqlite3_prepare(db, str, -1, &stmt, &tail);
		sqlite3_free(str);
		if (error != SQLITE_OK)
		{
			printf("2 error title %s\n", title);
		}
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
		str = sqlite3_mprintf("SELECT id FROM title WHERE name = '%q' AND album = %d;",
				title, album);
		error = sqlite3_prepare(db, str, -1, &stmt, &tail);
		sqlite3_free(str);
		if (error != SQLITE_OK)
		{
			printf("3 error title %s\n", title);
			return id;
		}
		if (sqlite3_step(stmt) != SQLITE_ROW)
		{
			printf("error querying title %s %d\n", title, album);
			return id;
		}
	}
	id = sqlite3_column_int(stmt, 0);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	return id;

}

static int db_file_changed(const char *file, time_t mtime)
{
	char *str;
	sqlite3_stmt *stmt;
	const char *tail;
	int error;

	str = sqlite3_mprintf("SELECT mtime FROM files WHERE file = '%q'", file);
	/* check if the file exists if so check the mtime and compare */
	error = sqlite3_prepare(db, str, -1, &stmt, &tail);
	sqlite3_free(str);
	if (error != SQLITE_OK)
	{
		printf("1 error file %s\n", file);
		return 1;
	}
	if (sqlite3_step(stmt) != SQLITE_ROW)
	{
		return 1;
	}
	else
	{
		time_t dbtime;

		dbtime = sqlite3_column_int(stmt, 0);
		sqlite3_finalize(stmt);
		if (dbtime < mtime)
			return 1;
		else
			return 0;
	}
}

static void db_insert_file(const char *file, time_t mtime, int title)
{
	char *str;
	sqlite3_stmt *stmt;
	const char *tail;
	int error;
	int id = -1;

	str = sqlite3_mprintf("INSERT OR IGNORE INTO files (file, mtime, title) VALUES ('%q',%d,%d);",
			file, mtime, title);
	error = sqlite3_prepare(db, str, -1, &stmt, &tail);
	sqlite3_free(str);
	if (error != SQLITE_OK)
	{
		printf("1 error file %s\n", file);
		return;
	}
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	printf("file found %s %ld %d\n", file, mtime, title);
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
	if (sqlite3_open("/tmp/metadatafs.db", &db) != SQLITE_OK)
	{
		printf("could not open the db\n");
		return 0;
	}

	error = sqlite3_prepare(db,
			"CREATE TABLE IF NOT EXISTS "
			"artist(id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT UNIQUE);",
			-1, &stmt, &tail);
	if (error != SQLITE_OK)
	{
		printf("error artist\n");
		return 0;
	}
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	/* TODO add a year */
	error = sqlite3_prepare(db,
			"CREATE TABLE IF NOT EXISTS "
			"album(id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, artist INTEGER, "
			"FOREIGN KEY (artist) REFERENCES artist (id));",
			-1, &stmt, &tail);
	if (error != SQLITE_OK)
	{
		printf("error album\n");
		return 0;
	}
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	/* TODO add the genre */
	error = sqlite3_prepare(db,
			"CREATE TABLE IF NOT EXISTS "
			"title(id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, album INTEGER, "
			"FOREIGN KEY (album) REFERENCES album (id));",
			-1, &stmt, &tail);
	if (error != SQLITE_OK)
	{
		printf("error title\n");
		return 0;
	}
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	error = sqlite3_prepare(db,
			"CREATE TABLE IF NOT EXISTS "
			"files(id INTEGER PRIMARY KEY AUTOINCREMENT, file TEXT, dbfile TEXT, "
			"mtime INTEGER, title INTEGER, "
			"FOREIGN KEY (title) REFERENCES title (id));",
			-1, &stmt, &tail);
	if (error != SQLITE_OK)
	{
		printf("error file\n");
		return 0;
	}
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	return 1;
}
/******************************************************************************
 *                               metadatafs                                   *
 ******************************************************************************/
static void _scan(const char *path)
{
	DIR *dp;
	struct dirent *de;

	dp = opendir(path);
	if (!dp)
	{
		printf("cannot scan dir\n");
		return;
	}

	while ((de = readdir(dp)) != NULL)
	{
		char realfile[PATH_MAX];
		struct stat st;

		if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
			continue;

		/* also check subdirs */
		strncpy(realfile, path, PATH_MAX);
		strncat(realfile, "/", PATH_MAX - strlen(de->d_name));
		strncat(realfile, de->d_name, PATH_MAX - strlen(de->d_name));

		if (stat(realfile, &st) < 0)
		{
			printf("err on stat %d %s\n", errno, realfile);
			continue;
		}

		if (S_ISDIR(st.st_mode))
		{
			_scan(realfile);
		}
		else if (S_ISREG(st.st_mode))
		{
			int id;
			char *str;
			void *handle;

			//printf("processing file %s\n", realfile);
			if (!db_file_changed(realfile, st.st_mtime))
				continue;

			handle = libmetadatafs_open(realfile);
			if (!handle) continue;

			/* artist */
			str = libmetadatafs_artist_get(handle);
			id = db_insert_artist(str);
			free(str);
			if (id < 0) goto end;
			/* album */
			str = libmetadatafs_album_get(handle);
			id = db_insert_album(str, id);
			free(str);
			if (id < 0) goto end;
			/* title */
			str = libmetadatafs_title_get(handle);
			id = db_insert_title(str, id);
			free(str);
			if (id < 0) goto end;

			/* file */
			db_insert_file(realfile, st.st_mtime, id);
end:
			libmetadatafs_close(handle);
		}
	}
	closedir(dp);
}

static void * _scanner(void *data)
{
	metadatafs *mfs = data;

	printf("path = %p\n", mfs->basepath);
	_scan(mfs->basepath);
	return NULL;
}


static void metadatafs_scan(metadatafs *mfs)
{
	int ret;
	pthread_attr_t attr;

	ret = pthread_attr_init(&attr);
	if (ret) {
		perror("pthread_attr_init");
		return;
	}

	ret = pthread_create(&mfs->scanner, &attr, _scanner, mfs);
	if (ret) {
		perror("pthread_create");
		return;
	}
}

#if HAVE_INOTIFY
static void * _monitor(void *data)
{
	metadatafs *mfs = data;

	printf("starting the monitor\n");
	mfs->inotify_fd = inotify_init();
	if (mfs->inotify_fd < 0)
	{
		printf("error initializing inotify\n");
		return NULL;
	}
	mfs->inotify_wd = inotify_add_watch(mfs->inotify_fd, mfs->basepath, IN_MODIFY | IN_CREATE | IN_DELETE);
	//mfs->inotify_wd = inotify_add_watch(mfs->inotify_fd, mfs->basepath, IN_ALL_EVENTS);
	if (mfs->inotify_wd < 0)
	{
		printf("error adding the watch\n");
		return NULL;
	}
	while (1)
	{
		char buf[BUF_LEN];
		int len, i = 0;

		len = read(mfs->inotify_fd, buf, BUF_LEN);
		while (i < len)
		{
			struct inotify_event *event;

		        event = (struct inotify_event *) &buf[i];
			printf("wd=%d mask=%u cookie=%u len=%u\n",
        	        	event->wd, event->mask,
	                	event->cookie, event->len);

        		if (event->len)
		                printf ("name=%s\n", event->name);

		        i += EVENT_SIZE + event->len;
		}
	}
	//inotify_rm_watch(mfs->inotify_fd, mfs->inotify_wd);
	//close(mfs->inotify_fd);
}

static void metadatafs_monitor(metadatafs *mfs)
{
	int ret;
	pthread_attr_t attr;

	ret = pthread_attr_init(&attr);
	if (ret) {
		perror("pthread_attr_init");
		return;
	}

	ret = pthread_create(&mfs->monitor, &attr, _monitor, mfs);
	if (ret) {
		perror("pthread_create");
		return;
	}
}
#endif

static void metadatafs_destroy(void *data)
{
	db_cleanup();
}

static metadatafs * metadatafs_new(char *path)
{
	metadatafs *mfs;
	int ret;

	mfs = calloc(1, sizeof(metadatafs));
	ret = pthread_mutex_init(&mfs->lock, NULL);
	if (ret)
	{
		free(mfs);
		return NULL;
	}
	mfs->basepath = strdup(path);

	return mfs;
}

static void metadatafs_free(metadatafs *mfs)
{
	if (mfs->scanner)
	{
		pthread_cancel(mfs->scanner);
		pthread_join(mfs->scanner, NULL);
	}
#if HAVE_INOTIFY
	if (mfs->monitor)
	{
		pthread_cancel(mfs->monitor);
		pthread_join(mfs->monitor, NULL);
	}
#endif
	free(mfs->basepath);
	free(mfs);
}
/******************************************************************************
 *                                   FUSE                                     *
 ******************************************************************************/
static int metadatafs_readlink(const char *path, char *buf, size_t size)
{
	Mdfs_File *file;
	int id;
	char *tmp;
	char *last;
	size_t len;

	/* FIXME get the last entry on the path */
	for (tmp = path + strlen(path); tmp >= path; tmp--)
	{
		if (*tmp == '/')
		{
			tmp++;
			break;
		}
	}
	file = mdfs_file_get_from_id(db, atoi(tmp));
	if (!file)
		return -ENOENT;

	strncpy(buf, file->path, size);
	buf[size - 1] = '\0';
	mdfs_file_free(file);

	return 0;
}

static int metadatafs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		off_t offset, struct fuse_file_info *fi)
{
	metadatafs_query q;
	char *tmp;
	int ret;

	tmp = strdup(path);
	ret = _path_to_query(tmp, &q);
	free(tmp);
	if (!ret)
	{
		printf("invalid directory %s\n", path);
		return -ENOENT;
	}
	/* the last directory on the path is not a metadata field */
	//printf("readdir %s %d %d\n", path, q.last_is_field, q.fields);
	if (q.last_is_field < 0)
	{
		int i = 0;
		metadatafs_mask f;

		if (q.fields & MASK_FILES)
			goto end;

		/* invert the flags found on the path */
		f = ~q.fields & ((1 << FIELDS) - 1);
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
	}
	/* given the path, select the needed artist/album/whatever */
	else
	{
		sqlite3_stmt *stmt;
		const char *tail;
		int error;
		char *query;

		query = _query_to_string(&q);
		error = sqlite3_prepare(db, query, -1, &stmt, &tail);
		free(query);
		if (error != SQLITE_OK)
		{
			return -ENOENT;
		}
		if (sqlite3_step(stmt) != SQLITE_ROW)
		{
			return -ENOENT;
			sqlite3_finalize(stmt);
		}

		if (q.last_is_field == FIELD_FILES)
		{
			do
			{
				unsigned char name[PATH_MAX];
				int id;

				id = sqlite3_column_int(stmt, 0);
				snprintf(name, PATH_MAX, "%08d", id);
				if (filler(buf, name, NULL, 0))
					break;
			} while (sqlite3_step(stmt) == SQLITE_ROW);
		}
		else
		{
			do
			{
				const unsigned char *name;

				name = sqlite3_column_text(stmt, 0);
				if (filler(buf, name, NULL, 0))
					break;
			} while (sqlite3_step(stmt) == SQLITE_ROW);
		}
		sqlite3_finalize(stmt);
	}
end:
	/* add simple '.' and '..' files */
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	return 0;
}

static int metadatafs_getattr(const char *path, struct stat *stbuf)
{
	char *file;
	int field = 0;
	int i;

	memset(stbuf, 0, sizeof(struct stat));
	/* simplest case */
	if (!strcmp(path, "/"))
	{
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		return 0;
	}
	/* first check if the last entry is a field */
	file = _get_last_delim(path, path + strlen(path), '/');
	for (i = 0; i < FIELDS; i++)
	{
		if (!strncmp(file, _fields[i], strlen(_fields[i])))
		{
			stbuf->st_mode = S_IFDIR | 0755;
			stbuf->st_nlink = 2;
			return 0;
		}
	}
	/* now get the last valid field from the file */
	file = _get_last_field(path, &field);
	printf("field = %d file = %s\n", field, file);
	/* set the default mode */
	stbuf->st_mode = S_IFDIR | 0755;
	stbuf->st_nlink = 2;

	switch (field)
	{
		/* if previous dir is Files then all the contents must be links */
		case FIELD_FILES:
		{
			Mdfs_File *f;

			f = mdfs_file_get_from_id(db, atoi(file));
			if (!f) return -ENOENT;
			mdfs_file_free(f);
			stbuf->st_mode = S_IFLNK | 0644;
			stbuf->st_nlink = 1;
		}
		break;

		case FIELD_ALBUM:
		{
			Mdfs_Album *album;
			album = mdfs_album_get_from_name(db, file);
			if (!album) return -ENOENT;
			mdfs_album_free(album);
		}
		break;

		case FIELD_ARTIST:
		{
			Mdfs_Artist *artist;
			artist = mdfs_artist_get(db, file);
			if (!artist) return -ENOENT;
			mdfs_artist_free(artist);
		}
		break;

		/* FIXME, this only handles correctly the case of /Title/T, /Artist/A/Title/T
		 * must be double checked on the artist too
		 */
		case FIELD_TITLE:
		{
			Mdfs_Title *title;
			title = mdfs_title_get_from_name(db, file);
			if (!title) return -ENOENT;
			mdfs_title_free(title);
		}
		break;

		default:
		return -ENOENT;
	}
	/* check the last metadata directory */
	return 0;
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

/**
 * Here we handle all the logic of the mv operation
 */
int metadatafs_rename(const char *orig, const char *dest)
{
	metadatafs_query src;
	metadatafs_query dst;
	char *tmp;
	int ret;

	tmp = strdup(orig);
	ret = _path_to_query(tmp, &src);
	free(tmp);

	tmp = strdup(dest);
	ret = _path_to_query(tmp, &dst);
	free(tmp);

	/* the last field on the path should be the same */
	if (src.last_field != dst.last_field)
		return -EINVAL;
	/* we cannot move fields */
	if (src.last_is_field >= 0)
		return -EINVAL;

	/* we cannot change file ids */
	if (src.last_field == FIELD_FILES &&
			strcmp(src.entries[FIELD_FILES], dst.entries[FIELD_FILES]))
		return -EINVAL;

	/* now we can update the metadata */
	printf("rename %s %s!!!\n", orig, dest);
	_query_dump(&src);
	_query_dump(&dst);
	return -EACCES;
}

static void * metadatafs_init(struct fuse_conn_info *conn)
{
	struct fuse_context *ctx;
	metadatafs *mfs;

	/* setup the connection info */
	conn->async_read = 0;
	/* get the context */
	ctx = fuse_get_context();
	mfs = ctx->private_data;
	/* read/create the database */
	if (!db_setup()) return NULL;
	/* update the database */
	metadatafs_scan(mfs);
	/* monitor file changes */
#if HAVE_INOTIFY
	metadatafs_monitor(mfs);
#endif
	return NULL;
}

static struct fuse_operations metadatafs_ops = {
	.getattr  = metadatafs_getattr,
	.readlink = metadatafs_readlink,
	.readdir  = metadatafs_readdir,
	.open     = metadatafs_open,
	.read     = metadatafs_read,
	.statfs   = metadatafs_statfs,
	.rename   = metadatafs_rename,
	.init     = metadatafs_init,
};

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

int main(int argc, char **argv)
{
	struct fuse_args args;
	metadatafs *mfs;

	if (argc < 2)
	{
		return 0;
	}

	basepath = strdup(argv[1]);
	argv[1] = argv[0];

	mfs = metadatafs_new(basepath);
	args.argc = argc - 1;
	args.argv = argv + 1;
	args.allocated = 0;

	//fuse_opt_match(args,
	fuse_main(argc - 1, argv + 1, &metadatafs_ops, mfs);
	metadatafs_free(mfs);
	free(basepath);

	return 0;
}
