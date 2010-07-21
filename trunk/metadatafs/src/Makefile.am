MAINTAINERCLEANFILES = Makefile.in config.h.in
AM_CFLAGS = $(fuse_CFLAGS) $(id3tag_CFLAGS)

bin_PROGRAMS	= metadatafs
metadatafs_SOURCES = metadatafs.c
metadatafs_LDADD = $(fuse_LIBS) $(id3tag_LIBS)
