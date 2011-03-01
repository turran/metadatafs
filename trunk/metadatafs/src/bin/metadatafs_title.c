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
static Mdfs_Title * mdfs_title_new_internal(unsigned int id, const char *name,
		Mdfs_Album *album)
{
	Mdfs_Title *thiz;

	thiz = calloc(1, sizeof(Mdfs_Title));
	if (!thiz) return NULL;
	thiz->id = id;
	thiz->name = strdup(name);
	thiz->album = album;

	return thiz;
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Mdfs_Title * mdfs_title_get_from_name(sqlite3 *db, const char *name)
{
	Mdfs_Title *title;
	char *str;
	sqlite3_stmt *stmt;
	const char *tail;
	int error;
	int id;
	unsigned int album_id;

	str = sqlite3_mprintf("SELECT id,album FROM title WHERE name = '%q'",
			name);
	error = sqlite3_prepare(db, str, -1, &stmt, &tail);
	sqlite3_free(str);
	if (error != SQLITE_OK)
		return NULL;
	if (sqlite3_step(stmt) != SQLITE_ROW)
		return NULL;
	id = sqlite3_column_int(stmt, 0);
	album_id = sqlite3_column_int(stmt, 1);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	title = mdfs_title_new_internal(id, name, NULL);
	return title;
}

Mdfs_Title * mdfs_title_get(sqlite3 *db, const char *name, Mdfs_Album *album)
{
	Mdfs_Title *title;
	char *str;
	sqlite3_stmt *stmt;
	const char *tail;
	int error;
	int id;

	str = sqlite3_mprintf("SELECT id FROM title WHERE name = '%q' AND album = %d;",
			name, album->id);
	error = sqlite3_prepare(db, str, -1, &stmt, &tail);
	sqlite3_free(str);
	if (error != SQLITE_OK)
		return NULL;
	if (sqlite3_step(stmt) != SQLITE_ROW)
		return NULL;
	id = sqlite3_column_int(stmt, 0);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	title = mdfs_title_new_internal(id, name, album);
	return title;
}

Mdfs_Title * mdfs_title_new(sqlite3 *db, const char *name, Mdfs_Album *album)
{
	char *str;
	sqlite3_stmt *stmt;
	const char *tail;
	int error;
	int id = -1;

	/* insert the new artist */
	str = sqlite3_mprintf("INSERT OR IGNORE INTO title (name, album) VALUES ('%q',%d);",
			name, album);
	error = sqlite3_prepare(db, str, -1, &stmt, &tail);
	sqlite3_free(str);
	if (error != SQLITE_OK)
		return NULL;	
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	return mdfs_title_get(db, name, album);
}

void mdfs_title_free(Mdfs_Title *title)
{
	free(title->name);
	if (title->album)
		mdfs_album_free(title->album);
	free(title);
}

int mdfs_title_init(sqlite3 *db)
{
	sqlite3_stmt *stmt;
	const char *tail;
	int error;

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

	return 1;
}

