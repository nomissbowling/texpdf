/*
  pdftx.h
*/

#ifndef __PDFTX_H__
#define __PDFTX_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "memstream.h"
#include "pdfobjects.h"

MEM_STREAM *hexstr(int e, char *str);

MEM_STREAM *oq(MEM_STREAM *st, int n, MEM_STREAM *m);
MEM_STREAM *oo(MEM_STREAM *st, char *p);
MEM_STREAM *op(MEM_STREAM *st, char *p);
MEM_STREAM *bq(MEM_STREAM *st);
MEM_STREAM *eQ(MEM_STREAM *st);
MEM_STREAM *bt(MEM_STREAM *st);
MEM_STREAM *et(MEM_STREAM *st);
MEM_STREAM *bi(MEM_STREAM *st);
MEM_STREAM *ei(MEM_STREAM *st);
MEM_STREAM *id(MEM_STREAM *st);

MEM_STREAM *tf(MEM_STREAM *st, char *p, int sz);
MEM_STREAM *tx(MEM_STREAM *st, int ah, char *p);
MEM_STREAM *qm(MEM_STREAM *st, int x, int y, int cx, int cy, int q, char *t);
MEM_STREAM *cm(MEM_STREAM *st, int x, int y, int cx, int cy, char *t);
MEM_STREAM *Tm(MEM_STREAM *st, int x, int y, int cx, int cy, char *t);
MEM_STREAM *rg(MEM_STREAM *st, int fS, float r, float g, float b);
MEM_STREAM *val(MEM_STREAM *st, char *p, float v);
MEM_STREAM *poly(MEM_STREAM *st, int fS, int n, int *p); // 2 * n
MEM_STREAM *curve(MEM_STREAM *st, int fS, int n, int *p); // 2 * (1 + 3 * n)
MEM_STREAM *im(MEM_STREAM *st, char *p);
MEM_STREAM *vs(MEM_STREAM *st, int f, char *p, ...); // f, p, (bnvs), t

#endif // __PDFTX_H__
