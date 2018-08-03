/*
  pdftx.c
*/

#include "pdftx.h"

MEM_STREAM *hexstr(int e, char *str)
{
  char *dlm[] = {"", "#", "\\x"};
  int i;
  MEM_STREAM *m = memstream_open(NULL, 0);
  if(!m) fprintf(stderr, "ERROR: hexstr memstream_open" ALN);
  else{
    for(i = 0; i < strlen(str); ++i)
      fprintf(m->fp, "%s%02x", dlm[e], str[i] & 0x0FF);
    if(!memstream_written(m, 1, NULL, NULL))
      fprintf(stderr, "ERROR: hexstr memstream_written" ALN);
    memstream_close(m);
  }
  return m;
}

MEM_STREAM *oq(MEM_STREAM *st, int n, MEM_STREAM *m){
  return append_stream(st, n, m->buf, m->sz);
}
MEM_STREAM *oo(MEM_STREAM *st, char *p){ return append_stream(st, 0, p, 0); }
MEM_STREAM *op(MEM_STREAM *st, char *p){ return append_stream(st, 2, p, 0); }
MEM_STREAM *bq(MEM_STREAM *st){ return op(st, "q"); }
MEM_STREAM *eQ(MEM_STREAM *st){ return op(st, "Q"); }
MEM_STREAM *bt(MEM_STREAM *st){ return op(st, "BT"); }
MEM_STREAM *et(MEM_STREAM *st){ return op(st, "ET"); }
MEM_STREAM *bi(MEM_STREAM *st){ return op(st, "BI"); }
MEM_STREAM *ei(MEM_STREAM *st){ return op(st, "EI"); }
MEM_STREAM *id(MEM_STREAM *st){ return op(st, "ID"); }

MEM_STREAM *tf(MEM_STREAM *st, char *p, int sz)
{
  fprintf(st->fp, "/%s %d %s"LN, p, sz, "Tf");
  return st;
}

MEM_STREAM *tx(MEM_STREAM *st, int ah, char *p)
{
  char *bk[] = {"()", "<>"};
  MEM_STREAM *m = hexstr(0, p);
  fprintf(st->fp, "%c%s%c Tj"LN, bk[ah][0], ah ? m->buf : p, bk[ah][1]);
  memstream_release(&m);
  return st;
}

MEM_STREAM *qm(MEM_STREAM *st, int x, int y, int cx, int cy, int q, char *t)
{
  char *qc[] = {"cm", "Tm"};
  fprintf(st->fp, "%d %d %d %d %d %d %s", x, 0, 0, y, cx, cy, qc[q]);
  if(t) fprintf(st->fp, " %% %s", t);
  return op(st, "");
}

MEM_STREAM *cm(MEM_STREAM *st, int x, int y, int cx, int cy, char *t)
{
  return qm(st, x, y, cx, cy, 0, t);
}

MEM_STREAM *Tm(MEM_STREAM *st, int x, int y, int cx, int cy, char *t)
{
  return qm(st, x, y, cx, cy, 1, t);
}

MEM_STREAM *rg(MEM_STREAM *st, int fS, float r, float g, float b)
{
  fprintf(st->fp, "%3.1f %3.1f %3.1f %s"LN, r, g, b, fS ? "RG" : "rg");
  return st;
}

MEM_STREAM *val(MEM_STREAM *st, char *p, float v)
{
  fprintf(st->fp, "%3.1f %s"LN, v, p);
  return st;
}

MEM_STREAM *poly(MEM_STREAM *st, int fS, int n, int *p) // 2 * n
{
  int i, k;
  for(i = 0; i < n; ++i){
    k = i * 2;
    fprintf(st->fp, "%d %d %s"LN, p[k], p[k + 1], i ? "l" : "m");
  }
  return op(st, fS ? "S" : "f");
}

MEM_STREAM *curve(MEM_STREAM *st, int fS, int n, int *p) // 2 * (1 + 3 * n)
{
  int i, j, k;
  fprintf(st->fp, "%d %d %s"LN, p[0], p[1], "m");
  for(i = 0; i < n; ++i){
    for(j = 0; j < 3; ++j){
      k = (1 + i * 3 + j) * 2;
      fprintf(st->fp, "%d %d ", p[k], p[k + 1]);
    }
    op(st, "c");
  }
  return op(st, fS ? "S" : "f");
}

MEM_STREAM *im(MEM_STREAM *st, char *p)
{
  fprintf(st->fp, "/%s Do"LN, p);
  return st;
}

MEM_STREAM *vs(MEM_STREAM *st, int f, char *p, ...) // f, p, (bnvs), t
{
  va_list va;
  va_start(va, p);
  int b, n;
  double v;
  char *s, *t;
  // f = 0: b(F/T), 1: n, 2: v, 3: s(add /), 4: s(pass through)
  fprintf(st->fp, "/%s ", p);
  switch(f){
  case 0:{ b = va_arg(va, int); fprintf(st->fp, b ? "true" : "false"); } break;
  case 1:{ n = va_arg(va, int); fprintf(st->fp, "%d", n); } break;
  case 2:{ v = va_arg(va, double); fprintf(st->fp, "%3.1f", v); } break;
  case 3:{ s = va_arg(va, char *); fprintf(st->fp, "/%s", s); } break;
  default:{ s = va_arg(va, char *); fprintf(st->fp, "%s", s); } // through % OK
  }
  t = va_arg(va, char *);
  if(t) fprintf(st->fp, " %% %s", t);
  va_end(va);
  return op(st, "");
}
