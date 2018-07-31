/*
  memstream.h
*/

#ifndef __MEMSTREAM_H__
#define __MEMSTREAM_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#define _MEMS_READ 0
#define _MEMS_WRITE 1
#define _MEMS_ERR 2

typedef struct _MEM_STREAM {
  FILE *fp;
  int flg[2];
#ifndef WIN32
  // without pipe
#else
  int pfd[2]; // 0: _MEMS_READ, 1: _MEMS_WRITE
#endif
  char *buf;
  int sz;
} MEM_STREAM;

MEM_STREAM *memstream_open(char *buf, size_t sz);
void memstream_close(MEM_STREAM *st);
int memstream_written(MEM_STREAM *st, int flg, char **buf, size_t *sz);
void memstream_release(MEM_STREAM **st);

#endif // __MEMSTREAM_H__
