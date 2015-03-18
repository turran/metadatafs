## Introduction ##
FUSE based filesystem that generates a directory tree based on the multimedia files metadata.

## Features ##
  * Reads id3 information from mp3 files(**TODO** ogg, flac)
  * The scanning of files is done on another thread
  * File operations like mv or cp, updates the id3 tag
  * **TODO** The source directory is monitored through _inotify_ to update the directory tree live

## Usage example ##
```
metadatafs ~/media/mp3/ ~/media/metadata/

#> ls ~/media/metadata
Album  Artist  Files  Genre  Title

#> ls ~/media/metadata/Artist/
Archive  Dire Straits  James Blunt  Morcheeba  Unknown
```

## News ##
<wiki:gadget url="http://google-code-feed-gadget.googlecode.com/svn/trunk/gadget.xml" up\_feeds="http://www.turran.org/feeds/posts/default/-/metadatafs" width="500" height="400" border="0"/>