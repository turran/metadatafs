MAINTAINERCLEANFILES = Makefile.in
AM_CFLAGS = $(fuse_CFLAGS) $(id3tag_CFLAGS) $(sqlite3_CFLAGS)

noinst_LTLIBRARIES	= libmetadatafs.la
libmetadatafs_la_SOURCES = libmetadatafs.c libid3tag.c
libmetadatafs_la_LIBADD = $(id3tag_LIBS)

noinst_HEADERS = libmetadatafs.h
