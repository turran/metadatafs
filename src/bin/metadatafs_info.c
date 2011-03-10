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
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void mdfs_info_update(sqlite3 *db, Mdfs_Info *info)
{
	char *str;
	sqlite3_stmt *stmt;
	const char *tail;
	int error;
	char version[PATH_MAX];

	/* the basepath */
	str = sqlite3_mprintf("INSERT OR REPLACE INTO info (variable, value) VALUES\
			('basepath','%s');", info->basepath);
	error = sqlite3_prepare(db, str, -1, &stmt, &tail);
	sqlite3_free(str);
	if (error != SQLITE_OK)
		return;
	if (sqlite3_step(stmt) != SQLITE_DONE)
		return;
	sqlite3_finalize(stmt);

	/* the version */
	snprintf(version, PATH_MAX, "%d", info->version);
	str = sqlite3_mprintf("INSERT OR REPLACE INTO info (variable, value) VALUES\
			('version','%s');", version);
	error = sqlite3_prepare(db, str, -1, &stmt, &tail);
	sqlite3_free(str);
	if (error != SQLITE_OK)
		return;
	if (sqlite3_step(stmt) != SQLITE_DONE)
		return;
	sqlite3_finalize(stmt);
}

Mdfs_Info * mdfs_info_new(int version, char *basepath)
{
	Mdfs_Info *info;

	info = malloc(sizeof(Mdfs_Info));
	info->version = version;
	info->basepath = strdup(basepath);

	return info;
}

void mdfs_info_free(Mdfs_Info *info)
{
	free(info->basepath);
	free(info);
}

Mdfs_Info * mdfs_info_load(sqlite3 *db)
{
	Mdfs_Info *info = NULL;
	char *str;
	sqlite3_stmt *stmt;
	const char *tail;
	int error;

	/* query the information */
	str = sqlite3_mprintf("SELECT variable,value FROM info;");
	error = sqlite3_prepare(db, str, -1, &stmt, &tail);
	sqlite3_free(str);
	if (error != SQLITE_OK)
		return NULL;
	if (sqlite3_step(stmt) != SQLITE_ROW)
	{
		goto end;
	}
	else
	{
		info = malloc(sizeof(Mdfs_Info));
		do
		{
			char *variable;
			char *value;

			variable = sqlite3_column_text(stmt, 0);
			value = sqlite3_column_text(stmt, 1);
			if (!strcmp(variable, "path"))
				info->basepath = strdup(value);
			else if (!strcmp(variable, "version"))
				info->version = atoi(value);
		} while (sqlite3_step(stmt) != SQLITE_ROW);
	}
end:
	sqlite3_finalize(stmt);
	return info;
}

int mdfs_info_init(sqlite3 *db)
{
	sqlite3_stmt *stmt;
	const char *tail;
	int error;

	error = sqlite3_prepare(db,
			"CREATE TABLE IF NOT EXISTS "
			"info(variable TEXT PRIMARY KEY, value TEXT);",
			-1, &stmt, &tail);
	if (error != SQLITE_OK)
	{
		printf("error info\n");
		return 0;
	}
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	return 1;

}
