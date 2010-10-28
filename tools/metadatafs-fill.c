#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "libmetadatafs.h"

/* the options */
static int dry = 0;
static int verbose = 0;

typedef enum _mask_tag
{
	ARTIST,
	ALBUM,
	TITLE,
	TRACK,
	YEAR,
	GENRE
} mask_tag;

typedef struct _mask
{
	char *offset;
	mask_tag tag;
	char data[PATH_MAX];
} mask;

static void help(void)
{
	printf("metadatafs-rename -[OPTIONS] FORMAT FILES ...\n");
	printf("Options:\n");
	printf("v: Verbose\n");
	printf("d: Dry run\n");
	printf("h: This help\n");
	printf("Format:\n");
	printf("%%a: Artist\n");
	printf("%%t: Title\n");
	printf("%%b: Album\n");
	printf("%%g: Genre\n");
	printf("%%n: Track number\n");
	printf("%%y: Year\n");
}

static void parse_options(char *options)
{
	printf("options = %s\n", options);
}

static char * invstrchr(char *start, char *end, int c)
{
	while (end >= start)
	{
		if (*end == c)
			return end;
		end--;
	}
	return NULL;
}

static char * wrap(char *start, char *end, char *needle)
{
	size_t needlelen;

	/* Check for the null needle case */
	if (*needle == '\0')
		return start;

	needlelen = strlen(needle);
	for (; (end = invstrchr(start, end, *needle)) != NULL; end--)
	{
		if (end - needlelen < start)
			return NULL;
		if (strncmp(end, needle, needlelen) == 0)
			return end;
	}
	return NULL;
}

static int parse_mask(char *format, mask **mdst)
{
	mask *m = NULL;
	char *token;
	int count = 0;

	/* get the tokens */
	while ((token = strchr(format, '%')))
	{
		mask *mtmp;

		token++;
		m = realloc(m, sizeof(mask) * ++count);
		mtmp = &m[count - 1];
		mtmp->offset = token - 1;
		mtmp->data[0] = '\0';
		/* check the type and store it */
		switch (*token)
		{
			case 'a':
			mtmp->tag = ARTIST;
			break;
 
			case 't':
			mtmp->tag = TITLE;
			break;

			case 'b':
			mtmp->tag = ALBUM;
			break;

			case 'g':
			mtmp->tag = GENRE;
			break;

			case 'n':
			mtmp->tag = TRACK;
			break;

			case 'y':
			mtmp->tag = YEAR;
			break;

			default:
			printf("Unhandled token %c\n", *token);
			free(m);
			return 0;
			break;
		}
		format = token;
	}
	*mdst = m;
	return count;
}

static void fill_mask(mask *m, int count, char *file, char *format)
{
	char *tmp1;
	char *tmp2;
	char *token;
	char *efmt;
	char *efile;
	int i;

	efile = file + strlen(file) - 1;
	efmt = format + strlen(format) - 1;
	/* first advance to the last token */
	while (efmt > m[count - 1].offset + 1)
	{
		if (efile < file)
		{
			printf("We run of out chars to match\n");
			free(m);
			return;
		}
		if (*efile != *efmt)
		{
			printf("Not matching\n");
			free(m);
			return;
		}
		efile--;
		efmt--;
	}
	efile++;
	efmt++;
	for (i = count - 1; i >= 0; i--)
	{
		mask *mtmp = &m[i];
		char needle[PATH_MAX];
		char *start;
		size_t len;

		/* get the pattern to find */
		if (i - 1 >= 0)
			start = m[i - 1].offset + 2;
		else
			start = format;

		len = m[i].offset - start;
		strncpy(needle, start, len);
		needle[len] = '\0';

		/* find the pattern on the file */
		start = wrap(file, efile, needle);
		if (!start)
		{
			char tmp[PATH_MAX];

			strncpy(tmp, file, efile - file + 1);
			printf("pattern not found \"%s\" in %s\n", needle, tmp);
			return;
		}
		strncpy(m[i].data, start + len, efile - (start + len) + 1);
		efile = start - 1;
	}
}

static void process(char *file, char *format)
{
	int count;
	mask *m;
	int i;
	char *tmp;
	void *handle;

	/* get the full path of the file */
	struct stat st;
	if (stat(file, &st) < 0)
	{
		printf("Error processing file %s: %s", file, strerror(errno));
		return;
	}
	handle = libmetadatafs_open(file);
	if (!handle)
	{
		return;
	}
	/* remove the extension */
	for (tmp = file + strlen(file) - 1; *tmp; tmp--)
	{
		if (*tmp == '.')
		{
			*tmp = '\0';
			break;
		}
	}
	count = parse_mask(format, &m);
	fill_mask(m, count, file, format);
	/* check that the tokens are correctly */
	for (i = count - 1; i >= 0; i--)
	{
		mask *mtmp = &m[i];
		/* for each token, fill the id3 info */
		switch (mtmp->tag)
		{
			case ARTIST:
			printf("%s -> %s\n", libmetadatafs_artist_get(handle), mtmp->data);
			break;
 
			case TITLE:
			printf("%s -> %s\n", libmetadatafs_title_get(handle), mtmp->data);
			break;

			case ALBUM:
			printf("%s -> %s\n", libmetadatafs_album_get(handle), mtmp->data);
			break;

			case GENRE:
			break;

			case TRACK:
			break;

			case YEAR:
			break;

		}
	}
	free(m);
}

int main(int argc, char **argv)
{
	int pos = 1;
	char *format = NULL;
	int i;

	if (argc < 3)
	{
		help();
		return 0;
	}

	if (*argv[pos] == '-')
	{
		parse_options(argv[pos] + 1);
		pos++;
	}

	format = strdup(argv[pos++]);
	printf("format = %s\n", format);
	for (i = pos; i < argc; i++)
	{
		process(argv[i], format);
	}
	return 0;
}
