MAINTAINERCLEANFILES = Makefile.in config.h.in
AM_CFLAGS = $(fuse_CFLAGS) $(id3tag_CFLAGS) $(sqlite3_CFLAGS) -I$(top_srcdir)/src/lib

bin_PROGRAMS	= metadatafs
metadatafs_SOURCES = \
	metadatafs.c \
	metadatafs_artist.c \
	metadatafs_album.c \
	metadatafs_title.c \
	metadatafs_file.c

metadatafs_LDADD = $(fuse_LIBS) $(sqlite3_LIBS) $(top_builddir)/src/lib/libmetadatafs.la

noinst_HEADERS = metadatafs.h
