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
static Mdfs_Album * mdfs_album_new_internal(unsigned int id, const char *name,
		Mdfs_Artist *artist)
{
	Mdfs_Album *thiz;

	thiz = calloc(1, sizeof(Mdfs_Album));
	if (!thiz) return NULL;
	thiz->id = id;
	thiz->name = strdup(name);
	thiz->artist = artist;

	return thiz;
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Mdfs_Album * mdfs_album_get_from_name(sqlite3 *db, const char *name)
{
	Mdfs_Album *album;
	char *str;
	sqlite3_stmt *stmt;
	const char *tail;
	int error;
	int id;
	unsigned int artist_id;

	str = sqlite3_mprintf("SELECT id,artist FROM album WHERE name = '%q';",
			name);
	error = sqlite3_prepare(db, str, -1, &stmt, &tail);
	sqlite3_free(str);
	if (error != SQLITE_OK)
		return NULL;
	if (sqlite3_step(stmt) != SQLITE_ROW)
		return NULL;
	id = sqlite3_column_int(stmt, 0);
	artist_id = sqlite3_column_int(stmt, 1);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	album = mdfs_album_new_internal(id, name, NULL);
	return album; 
}


Mdfs_Album * mdfs_album_get(sqlite3 *db, const char *name, Mdfs_Artist *artist)
{
	Mdfs_Album *album;
	char *str;
	sqlite3_stmt *stmt;
	const char *tail;
	int error;
	int id;

	str = sqlite3_mprintf("SELECT id FROM album WHERE name = '%q' AND artist = %d;",
			name, artist->id);
	error = sqlite3_prepare(db, str, -1, &stmt, &tail);
	sqlite3_free(str);
	if (error != SQLITE_OK)
		return NULL;
	if (sqlite3_step(stmt) != SQLITE_ROW)
		return NULL;
	id = sqlite3_column_int(stmt, 0);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	album = mdfs_album_new_internal(id, name, artist);
	return album; 
}

Mdfs_Album * mdfs_album_new(sqlite3 *db, const char *name, Mdfs_Artist *artist)
{
	char *str;
	sqlite3_stmt *stmt;
	const char *tail;
	int error;
	int id = -1;

	str = sqlite3_mprintf("INSERT OR IGNORE INTO album (name, artist) VALUES ('%q',%d);",
			name, artist->id);
	error = sqlite3_prepare(db, str, -1, &stmt, &tail);
	sqlite3_free(str);
	if (error != SQLITE_OK)
		return NULL;
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	return mdfs_album_get(db, name, artist);
}

void mdfs_album_free(Mdfs_Album *album)
{
	free(album->name);
	free(album);
}

int mdfs_album_init(sqlite3 *db)
{
	sqlite3_stmt *stmt;
	const char *tail;
	int error;

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

	return 1;
}

