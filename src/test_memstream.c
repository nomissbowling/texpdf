/*
  test_memstream.c

  > gcc -m32 -o test_memstream test_memstream.c memstream.c (to 32 bit OK)
  > gcc -o test_memstream test_memstream.c memstream.c (to 64 bit OK)
  > test_memstream
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memstream.h"

#define SRCBUF "abcdefghijklmnopqrstuvwxyz123456"
#define MAX_BUF 16
#define NUM_STREAM 681 // 682 BAD

int main(int ac, char **av)
{
  int i;
  char buf[MAX_BUF];
  char obuf[MAX_BUF - 1];
  char *pobuf = obuf;
  size_t l, r;
  MEM_STREAM *ist[NUM_STREAM], *ost[NUM_STREAM];
  for(i = 0; i < NUM_STREAM; ++i){
    ist[i] = memstream_open(SRCBUF, strlen(SRCBUF));
    ost[i] = memstream_open(NULL, 0);
    if(!ist[i] || !ost[i]) return 0;
  }
  fprintf(stdout, "%d\n", ftell(ost[0]->fp));

  l = fread(buf, sizeof(char), sizeof(buf), ist[0]->fp);
  fwrite(buf, sizeof(char), l, ost[0]->fp);
  fprintf(stdout, "A(%d) [%s]\n", l, buf); fflush(stdout);
  fprintf(stdout, "%d\n", ftell(ost[0]->fp));

  l = fread(buf, sizeof(char), sizeof(buf), ist[0]->fp);
  fwrite(buf, sizeof(char), l, ost[0]->fp);
  fprintf(stdout, "B(%d) [%s]\n", l, buf); fflush(stdout);
  fprintf(stdout, "%d\n", ftell(ost[0]->fp));

  r = fseek(ost[0]->fp, -18, SEEK_CUR); // not support (now bug)
  fprintf(stdout, "r: %d\n", r);
  fprintf(stdout, "%d\n", ftell(ost[0]->fp));

  fwrite(buf, sizeof(char), l, ost[0]->fp);
  fprintf(stdout, "C(%d) [%s]\n", l, buf); fflush(stdout);
  fprintf(stdout, "%d\n", ftell(ost[0]->fp));
/*
  r = fseek(ost[0]->fp, 0, SEEK_SET);
  fprintf(stdout, "r: %d\n", r);
  fprintf(stdout, "%d\n", ftell(ost[0]->fp));
*/
  l = sizeof(obuf);
  r = memstream_written(ost[0], 1, &pobuf, &l);
  fprintf(stdout, "X(%d) [%s]\n", r, obuf); fflush(stdout);
  fprintf(stdout, "%d\n", ftell(ost[0]->fp));

  fwrite(buf, sizeof(char), sizeof(buf), ost[0]->fp);
  fprintf(stdout, "D(%d) [%s]\n", sizeof(buf), buf); fflush(stdout);
  fprintf(stdout, "%d\n", ftell(ost[0]->fp));

  r = memstream_written(ost[0], 1, NULL, NULL);
  fprintf(stdout, "X(%d)%d[%s]\n", r, ost[0]->sz, ost[0]->buf); fflush(stdout);
  fprintf(stdout, "%d\n", ftell(ost[0]->fp));

  for(i = 0; i < NUM_STREAM; ++i){
    memstream_close(ost[i]);
    memstream_release(&ost[i]);
    memstream_close(ist[i]);
    memstream_release(&ist[i]);
  }
  return 0;
}
