MAINTAINERCLEANFILES = Makefile.in config.h.in
AM_CFLAGS = $(fuse_CFLAGS) $(id3tag_CFLAGS) $(sqlite3_CFLAGS)

bin_PROGRAMS	= metadatafs
metadatafs_SOURCES = metadatafs.c libid3tag.c
metadatafs_LDADD = $(fuse_LIBS) $(id3tag_LIBS) $(sqlite3_LIBS)

noinst_HEADERS = metadatafs.h
