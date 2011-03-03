/* MetadataFS -
 * Copyright (C) 2010 Jorge Luis Zapata
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.
 * If not, see <http://www.gnu.org/licenses/>.
 */
#include "metadatafs.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static Mdfs_File * mdfs_file_new_internal(unsigned int id, const char *path,
		time_t mtime, unsigned int title)
{
	Mdfs_File *thiz;

	thiz = calloc(1, sizeof(Mdfs_File));
	if (!thiz) return NULL;
	thiz->id = id;
	thiz->mtime = mtime;
	thiz->path = strdup(path);
	thiz->title = title;

	return thiz;
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Mdfs_File * mdfs_file_get_from_id(sqlite3 *db, unsigned int id)
{
	Mdfs_File *file;
	char *str;
	sqlite3_stmt *stmt;
	const char *tail;
	int error;
	const unsigned char *path;
	time_t mtime;
	unsigned int title;

	str = sqlite3_mprintf("SELECT file, mtime, title FROM files WHERE id = %d", id);
	error = sqlite3_prepare(db, str, -1, &stmt, &tail);
	sqlite3_free(str);
	if (error != SQLITE_OK)
		return NULL;
	if (sqlite3_step(stmt) != SQLITE_ROW)
		return NULL;

	path = sqlite3_column_text(stmt, 0);
	mtime = sqlite3_column_int(stmt, 1);
	title = sqlite3_column_int(stmt, 2);

	file = mdfs_file_new_internal(id, path, mtime, title);
	sqlite3_finalize(stmt);

	return file;
}

Mdfs_File * mdfs_file_get_from_path(sqlite3 *db, const char *path)
{
	Mdfs_File *file;
	char *str;
	sqlite3_stmt *stmt;
	const char *tail;
	int error;
	int id;
	time_t mtime;
	unsigned int title;

	str = sqlite3_mprintf("SELECT id,mtime,title FROM files WHERE file = '%q';",
			path);
	error = sqlite3_prepare(db, str, -1, &stmt, &tail);
	sqlite3_free(str);
	if (error != SQLITE_OK)
		return NULL;
	if (sqlite3_step(stmt) != SQLITE_ROW)
		return NULL;
	id = sqlite3_column_int(stmt, 0);
	mtime = sqlite3_column_int(stmt, 1);
	title = sqlite3_column_int(stmt, 2);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	return mdfs_file_new_internal(id, path, mtime, title);
}

Mdfs_File * mdfs_file_get(sqlite3 *db, const char *path, time_t mtime, unsigned int title)
{
	Mdfs_File *file;
	char *str;
	sqlite3_stmt *stmt;
	const char *tail;
	int error;
	int id;

	str = sqlite3_mprintf("SELECT id FROM file WHERE file = '%q' AND mtime = %d AND title = %d;",
			path, mtime, title);
	error = sqlite3_prepare(db, str, -1, &stmt, &tail);
	sqlite3_free(str);
	if (error != SQLITE_OK)
		return NULL;
	if (sqlite3_step(stmt) != SQLITE_ROW)
		return NULL;
	id = sqlite3_column_int(stmt, 0);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	return mdfs_file_new_internal(id, path, mtime, title);
}

Mdfs_File * mdfs_file_new(sqlite3 *db, const char *path, time_t mtime, unsigned int title)
{
	Mdfs_File *file;
	char *str;
	sqlite3_stmt *stmt;
	const char *tail;
	int error;
	int id;

	str = sqlite3_mprintf("INSERT OR IGNORE INTO files (file, mtime, title) VALUES ('%q',%d,%d);",
			path, mtime, title);
	error = sqlite3_prepare(db, str, -1, &stmt, &tail);
	sqlite3_free(str);
	if (error != SQLITE_OK)
		return NULL;
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	file = mdfs_file_get(db, path, mtime, title);
	return file;
}

void mdfs_file_free(Mdfs_File *file)
{
	free(file->path);
	free(file);
}

void mdfs_file_update(Mdfs_File *file, sqlite3 *db, const char *path, time_t mtime, unsigned int title)
{
	char *str;
	sqlite3_stmt *stmt;
	const char *tail;
	int error;
	int id;

	str = sqlite3_mprintf("UPDATE files SET file='%q', mtime=%d, title=%d WHERE id = %d",
			path, mtime, title, file->id);
	error = sqlite3_prepare(db, str, -1, &stmt, &tail);
	sqlite3_free(str);
	if (error != SQLITE_OK)
		return NULL;
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (strcmp(file->path, path))
	{
		free(file->path);
		file->path = strdup(path);
	}
	file->mtime = mtime;
	file->title = title;
}

int mdfs_file_init(sqlite3 *db)
{
	sqlite3_stmt *stmt;
	const char *tail;
	int error;

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
