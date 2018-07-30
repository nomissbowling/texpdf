/*
  memstream.c

  _O_BINARY
  _O_TEXT | _O_NOINHERIT -> _dup() _dup2() ...
*/

#include "memstream.h"

MEM_STREAM *memstream_open(char *buf, size_t sz)
{
  MEM_STREAM *st = (MEM_STREAM *)malloc(sizeof(MEM_STREAM));
  if(!st) return NULL;
  st->buf = NULL;
  st->sz = 0;
#ifndef WIN32
  st->flg[_MEMS_WRITE] = st->flg[_MEMS_READ] = 1;
  st->pfd[_MEMS_WRITE] = st->pfd[_MEMS_READ] = 0;
  if(buf) st->fp = fmemopen(buf, sz, "rb");
  else st->fp = open_memstream(&st->buf, &st->sz);
  //st->fp = funopen(NULL, NULL, NULL, NULL, NULL); // read, write, seek, close
#else
  st->flg[_MEMS_WRITE] = st->flg[_MEMS_READ] = 0;
  if(buf){
    if(_pipe(st->pfd, (unsigned int)sz, _O_BINARY) != 0){
      free(st);
      return NULL;
    }
    _write(st->pfd[_MEMS_WRITE], buf, (unsigned int)sz);
    _close(st->pfd[_MEMS_WRITE]);
    st->flg[_MEMS_WRITE] = 1;
    st->fp = _fdopen(st->pfd[_MEMS_READ], "rb");
  }else{
    if(_pipe(st->pfd, (unsigned int)0, _O_BINARY) != 0){
      free(st);
      return NULL;
    }
    st->flg[_MEMS_READ] = 1;
    st->fp = _fdopen(st->pfd[_MEMS_WRITE], "wb+");
  }
#endif
  return st;
}

void memstream_close(MEM_STREAM *st)
{
  if(st && st->fp){
    fclose(st->fp); st->fp = NULL; // _fdopen()
#ifndef WIN32
    // without pipe
#else
    if(st->flg[_MEMS_READ]) _close(st->pfd[_MEMS_READ]); // _pipe()
    // if(st->flg[_MEMS_WRITE]) _close(st->pfd[_MEMS_WRITE]); // already closed
    st->flg[_MEMS_WRITE] = st->flg[_MEMS_READ] = 0;
#endif
  }
}

int memstream_written(MEM_STREAM *st, char **buf, size_t *sz)
{
  if(!st) return 0;
#ifndef WIN32
  return 0;
#else
  if(!st->flg[_MEMS_READ]) return 0;
  fseek(st->fp, 0, SEEK_CUR);
  st->sz = ftell(st->fp);
  fflush(st->fp);
  // fseek(st->fp, 0, SEEK_SET);
  if(!buf || !sz || (buf && !*buf) || (sz && !*sz)){
    st->buf = (char *)malloc(st->sz);
    if(!st->buf) return 0;
    if(buf && sz){
      *buf = st->buf;
      *sz = st->sz;
    }
  }
  if(buf && sz && *buf && *sz)
    return _read(st->pfd[_MEMS_READ], *buf, *sz);
  else return _read(st->pfd[_MEMS_READ], st->buf, st->sz);
#endif
}

void memstream_release(MEM_STREAM **st)
{
  if(st && *st){
    if((*st)->buf){ free((*st)->buf); (*st)->buf = NULL; }
    free(*st); *st = NULL;
  }
}
