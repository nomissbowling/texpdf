/*
  pdfobjects.h
*/

#ifndef __PDFOBJECTS_H__
#define __PDFOBJECTS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "memstream.h"

#define CR "\x0D"
#define LF "\x0A"
#define LN "\x0D\x0A"
#define ALN "\a\x0D\x0A"

#define PDF_HEAD ("%PDF-1.4"LN LN)
#define PDF_FOOT ("%%EOF"LN)
#define PDF_IDX ("%010d %05d %c"LN)

#define RES_XOBJ "XObject"
#define RES_FONT "Font"

#define MAX_BUF 1024
#define MAX_IDX_LST 500

typedef struct _PDF_OBJ {
  int id;
  int gen;
  MEM_STREAM *atr;
  MEM_STREAM *stream;
} PDF_OBJ;

typedef struct _PDF_XREF {
  PDF_OBJ *obj;
  int len;
  int sub;
  char kwd;
} PDF_XREF;

//

MEM_STREAM *append_stream(MEM_STREAM *st, int n, char *str, int sz);
MEM_STREAM *load_stream(char *str);
MEM_STREAM *flush_chomp(MEM_STREAM *st, int n);
int flush_obj(PDF_OBJ *obj);

int init_xref(PDF_OBJ *obj);
int create_obj(PDF_OBJ *obj);
int create_xobj(PDF_OBJ *obj, MEM_STREAM *atr, MEM_STREAM *stream);
int create_image(PDF_OBJ *obj, int w, int h, int bpc, MEM_STREAM *stream);
MEM_STREAM *load_AHx(char *fn, int *w, int *h, int *c, int *b);
int load_image(PDF_OBJ *obj, char *fn);
int create_metr(PDF_OBJ *obj, char *face, int a, int b, int c, int d, char *st);
int create_descf(PDF_OBJ *obj, PDF_OBJ *m, char *face);
int create_font(PDF_OBJ *obj, PDF_OBJ *d, char *face, char *enc);
int create_resource(PDF_OBJ *obj);
int add_resource(PDF_OBJ *obj, char *rn, char *ra, PDF_OBJ *r);
int create_contents(PDF_OBJ *obj, MEM_STREAM *stream);
int create_page(PDF_OBJ *obj, PDF_OBJ *res, PDF_OBJ *ct);
int set_parent(PDF_OBJ *obj, PDF_OBJ *pt);
int create_pages(PDF_OBJ *obj, PDF_OBJ *p, int cnt, int x, int y, int w, int h);
int create_root(PDF_OBJ *obj, PDF_OBJ *pgs);
int create_info(PDF_OBJ *obj, char *dt, char *ttl, char *ath, char *prd);

int out_head(FILE *fp);
int out_objects(FILE *fp);
int out_xref(FILE *fp);
int out_trailer(FILE *fp, PDF_OBJ *root, PDF_OBJ *info);
int out_foot(FILE *fp, int offset);

int out_pdf(char *fname, PDF_OBJ *info, PDF_OBJ *root);

#endif // __PDFOBJECTS_H__
