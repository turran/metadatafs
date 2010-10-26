#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>

/* the options */
static int dry = 0;
static int verbose = 0;
char *format = NULL;

enum
{
	ARTIST,
	ALBUM,
	TITLE,
	TRACK,
	YEAR,
	GENRE
};

union my_va
{
	va_list varargs;
	void *packed;
};

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

static void process(char *file)
{
	struct id3_file *id3;
	char *token;
	int count = 0;
	char *sscanf_format;
	char *tmp;
	union my_va sscanf_va;
	int ret;
	int i;

	sscanf_format = strdup(format);
	tmp = strdup(format);
	/* count the number of arguments we'll need */
	token = strtok(tmp, "%");
	while (token)
	{
		switch (*token)
		{
			case 'a':
			case 't':
			case 'b':
			case 'g':
			case 'n':
			case 'y':
			break;
		}
		count++;
		*(sscanf_format + (token - tmp)) = 's';
		/* check the type and store it */
		token = strtok(NULL, "%");
	}
	sscanf_va.packed = alloca(PATH_MAX * count);
	
	printf("scanf %s %s (%s)\n", sscanf_format, file, format);
	ret = sscanf(file, sscanf_format, sscanf_va.varargs);
	printf("ret = %d\n", ret);
	for (i = 0; i < ret; i++)
	{
		char *tmp;

		tmp = sscanf_va.packed;
		printf("matching = %s\n", tmp + (PATH_MAX * i));
	}
	
#if 0
	id3 = id3_file_open(file, ID3_FILE_MODE_READONLY);
	if (!id3) return;
#endif
	free(sscanf_format);
	free(tmp);
}

int main(int argc, char **argv)
{
	int pos = 1;
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
		process(argv[i]);
	}
	return 0;
}
