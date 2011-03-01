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
static Mdfs_Artist * mdfs_artist_new_internal(unsigned int id, const char *name)
{
	Mdfs_Artist *thiz;

	thiz = calloc(1, sizeof(Mdfs_Artist));
	if (!thiz) return NULL;
	thiz->id = id;
	thiz->name = strdup(name);

	return thiz;
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Mdfs_Artist * mdfs_artist_get(sqlite3 *db, const char *name)
{
	Mdfs_Artist *artist;
	char *str;
	sqlite3_stmt *stmt;
	const char *tail;
	int error;
	unsigned int id;

	str = sqlite3_mprintf("SELECT id FROM artist WHERE name = '%q';",
			name);
	error = sqlite3_prepare(db, str, -1, &stmt, &tail);
	sqlite3_free(str);
	if (error != SQLITE_OK)
		return NULL;
	if (sqlite3_step(stmt) != SQLITE_ROW)
		return NULL;
	id = sqlite3_column_int(stmt, 0);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	artist = mdfs_artist_new_internal(id, name);
	return artist;
}

Mdfs_Artist * mdfs_artist_new(sqlite3 *db, const char *name)
{
	Mdfs_Artist *artist;
	char *str;
	sqlite3_stmt *stmt;
	const char *tail;
	int error;

	/* insert the new artist */
	str = sqlite3_mprintf("INSERT OR IGNORE INTO artist (name) VALUES ('%q');",
			name);
	error = sqlite3_prepare(db, str, -1, &stmt, &tail);
	sqlite3_free(str);
	if (error != SQLITE_OK)
		return NULL;
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	return mdfs_artist_get(db, name);
}

void mdfs_artist_free(Mdfs_Artist *artist)
{
	free(artist->name);
	free(artist);
}

int mdfs_artist_init(sqlite3 *db)
{
	sqlite3_stmt *stmt;
	const char *tail;
	int error;

	/* TODO we should get the version of the database and update it in case we need */
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

	return 1;
}
