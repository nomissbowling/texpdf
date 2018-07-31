/*
  PDF

  texpdf.c
  > gcc -m32 -o texpdf texpdf.c memstream.c (to 32 bit OK)
  > gcc -o texpdf texpdf.c memstream.c (to 64 bit OK)
  > texpdf _texpdf_out.pdf

  landscape mm  72          96          300         600         1200

  A0 1189  841  3370 2383   4494 3178   14043  9930 28087 19860 56173 39720
  A1  841  594  2383 1685   3178 2247    9930  7022 19860 14043 39720 28087
  A2  594  420  1685 1192   2247 1589    7022  4965 14043  9930 28087 19860
  A3  420  297  1192  843 * 1589 1123    4965  3511  9930  7022 19860 14043
  A4  297  210 * 843  596 * 1123  794    3511  2483  7022  4965 14043  9930
  A5  210  148   596  421    794  562    2483  1755  4965  3511  9930  7022
  A6  148  105   421  298    562  397    1755  1241  3511  2483  7022  4965
  A7  105   74   298  211    397  281    1241   878  2483  1755  4965  3511

  B0 1414 1000  4008 2835   5344 3780   16701 11811 33402 23622 66803 47244
  B1 1000  707  2835 2004   3780 2672   11811  8350 23622 16701 47244 33402
  B2  707  500  2004 1417   2672 1890    8350  5906 16701 11811 33402 23622
  B3  500  353  1417 1002 * 1890 1336    5906  4175 11811  8350 23622 16701
  B4  353  250  1002  709   1336  945    4175  2953  8350  5906 16701 11811
  B5  250  176   709  501    945  668    2953  2088  5906  4175 11811  8350
  B6  176  125   501  354    668  472    2088  1476  4175  2953  8350  5906
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "memstream.h"

#define INF_CREATIONDATE "19991231235959+09'00'"
#define INF_TITLE "Written TEXT"
#define INF_AUTHOR "gcc PDF writer"
#define INF_PRODUCER "texpdf.c"

#define FONT_ENC "90ms-RKSJ-H"
// #define FONT_ALIAS "MSGOTHIC"
// #define FONT_NAME "\x82\x6c\x82\x72\x83\x53\x83\x56\x83\x62\x83\x4e" // ...
// #define FONT_ALIAS "MSMINCHO"
// #define FONT_NAME "\x82\x6c\x82\x72\x96\xbe\x92\xa9" // ＭＳ明朝
// #define FONT_ALIAS "DFLgs9"
// #define FONT_NAME "\x82\x63\x82\x65\x97\xed\x89\xeb\x91\x76" // ＤＦ麗雅宋
#define FONT_ALIAS "MIGU1M"
#define FONT_NAME "Migu 1M"
#define FONT_STYLE "0805020B0609000000000000"

#define XOBJ_ALIAS "IMG0"
#define FILE_IMG0 "_texpdf_IMG0.axt"
#define FILE_MSK "_texpdf_MSK.axt"
#define TEXT_TEST "\x96\xbe\x92\xa9\x91\xcc\x89\x69" // 明朝体永

//

#define EXT_PDF ".pdf"
#define EXT_LEN sizeof(EXT_PDF)

#define MAX_STREAM 8192
#define MAX_BUF 1024
#define MAX_IDX_LST 500

#define RES_XOBJ "XObject"
#define RES_FONT "Font"

#define PDF_HEAD "%PDF-1.4\x0D\x0A\x0D\x0A"
#define PDF_FOOT "%%EOF\x0D\x0A"
#define PDF_IDX "%010d %05d %c\x0D\x0A"

typedef struct _PDF_STREAM {
  int sz;
  int l;
  char buf[0];
} PDF_STREAM;

typedef struct _PDF_OBJ {
  int id;
  int gen;
  MEM_STREAM *atr;
  PDF_STREAM *stream;
} PDF_OBJ;

typedef struct _PDF_XREF {
  PDF_OBJ *obj;
  int len;
  int sub;
  char kwd;
} PDF_XREF;

PDF_XREF XREF[MAX_IDX_LST];
int XREF_MAX = 0;

int chk_ext(char *fn, char *ext)
{
  char buf[EXT_LEN];
  int i, l;
  l = strlen(fn);
  if(l < EXT_LEN - 1) return -1;
  for(i = 0; i < EXT_LEN; ++i) buf[EXT_LEN - 1 - i] = (char)tolower(fn[l - i]);
  return strcmp(buf, ext);
}

char *hexstr(char *buf, int e, char *str)
{
  char *dlm[] = {"", "#", "\\x"};
  int i;
  for(i = 0; i < strlen(str); ++i)
    sprintf(buf + (e + 2) * i, "%s%02x", dlm[e], str[i] & 0x0FF);
  return buf;
}

PDF_STREAM *alloc_stream(int sz)
{
  PDF_STREAM *st = (PDF_STREAM *)malloc(sizeof(PDF_STREAM) + sz);
  if(!st) fprintf(stderr, "error malloc stream (%08x)\a\x0D\x0A", sz);
  else{
    st->sz = sz;
    st->l = 0;
    st->buf[0] = '\0';
  }
  return st;
}

PDF_STREAM *append_stream(PDF_STREAM *st, int n, char *str)
{
  static char *LN[] = {"%s", "%s\x0A", "%s\x0D\x0A"};
  if(!st || !str) return st;
  if(st->l + strlen(str) + n >= st->sz){
    fprintf(stderr, "overflow stream (%08x) + [%s]\a\x0D\x0A", st->l, str);
    return st;
  }
  // st->l = strlen(strncat(st->buf, str, st->sz - 1 - st->l)); // BAD + CRLF
  st->l += sprintf(st->buf + st->l, LN[n], str); // use %s to no care %%
  if(st->l >= st->sz)
    fprintf(stderr, "overflow stream (%08x)\a\x0D\x0A", st->l);
  return st;
}

PDF_STREAM *load_stream(char *str)
{
  return append_stream(alloc_stream(strlen(str) + 1), 0, str);
}

int release_stream(PDF_STREAM **st)
{
  if(*st){ free(*st); *st = NULL; }
  return 0;
}

int flush_obj(PDF_OBJ *obj)
{
  if(obj->atr){
    if(!memstream_written(obj->atr, 1, NULL, NULL))
      fprintf(stderr, "ERROR: obj->atr memstream_written\a\x0D\x0A");
    memstream_close(obj->atr);
  }
  return 0;
}

int init_xref(PDF_OBJ *obj)
{
  obj->id = XREF_MAX++;
  obj->gen = 0;
  obj->atr = NULL;
  obj->stream = NULL;
  XREF[0].obj = obj;
  XREF[0].len = strlen(PDF_HEAD);
  XREF[0].sub = 65535;
  XREF[0].kwd = 'f';
  return 0;
}

int create_obj(PDF_OBJ *obj)
{
  obj->id = XREF_MAX++;
  obj->gen = 0;
  obj->atr = memstream_open(NULL, 0);
  obj->stream = NULL;
  XREF[obj->id].obj = obj;
  XREF[obj->id].len = 0;
  XREF[obj->id].sub = 0;
  XREF[obj->id].kwd = 'n';
  if(XREF_MAX >= MAX_IDX_LST)
    fprintf(stderr, "overflow obj->id\a\x0D\x0A");
  if(!obj->atr)
    fprintf(stderr, "ERROR: obj->atr memstream_open\a\x0D\x0A");
  // must call flush_obj() later
  return 0;
}

int create_xobj(PDF_OBJ *obj, MEM_STREAM *atr, PDF_STREAM *stream)
{
  create_obj(obj);
  fprintf(obj->atr->fp, "%s/Length %d\x0A",
    atr ? atr->buf : "", strlen(stream->buf));
  obj->stream = stream;
  // must call flush_obj() later
  return 0;
}

int create_image(PDF_OBJ *obj, int w, int h, int bpc, PDF_STREAM *stream)
{
  create_xobj(obj, NULL, stream);
  fprintf(obj->atr->fp, "/Type /XObject\x0A");
  fprintf(obj->atr->fp, "/Subtype /Image\x0A");
  fprintf(obj->atr->fp, "/Width %d\x0A", w);
  fprintf(obj->atr->fp, "/Height %d\x0A", h);
  fprintf(obj->atr->fp, "/BitsPerComponent %d\x0A", bpc);
  fprintf(obj->atr->fp, "/ColorSpace /DeviceRGB\x0A");
  fprintf(obj->atr->fp, "/Filter [ /AHx ]\x0A");
  flush_obj(obj);
  return 0;
}

PDF_STREAM *load_AHx(char *fn, int *w, int *h, int *c, int *b)
{
  PDF_STREAM *st;
  char buf[MAX_BUF];
  int sz, v, i, l;
  FILE *fp;
  if(!(fp = fopen(fn, "rb"))){
    fprintf(stderr, "file not found [%s]\a\x0D\x0A", fn);
    return NULL;
  }
  if(!fgets(buf, sizeof(buf), fp)){
    fprintf(stderr, "unexpected EOF [%s] (0)\a\x0D\x0A", fn);
    fclose(fp);
    return NULL;
  }
  if(sscanf(buf, "%d %d %d %d", w, h, c, b) != 4){
    fprintf(stderr, "not found (W H C B) [%s] (0)\a\x0D\x0A", fn);
    fclose(fp);
    return NULL;
  }
  v = *w * 2 * *b * *c / 8 + 2;
  sz = *h * v + 1;
  // fprintf(stdout, "%d, %d, %d, %d, (%d), (%d)\n", *w, *h, *c, *b, v, sz);
  if(!(st = alloc_stream(sz))){
    fprintf(stderr, "error malloc image (%08x)\a\x0D\x0A", sz);
    fclose(fp);
    return NULL;
  }
  for(i = 0; i < *h; ++i){
    fgets(buf, sizeof(buf), fp);
    if((l = strlen(buf)) < v - 2){
      fprintf(stderr, "too short line length [%s] (%d)\a\x0D\x0A", fn, i + 1);
      fclose(fp);
      release_stream(&st);
      return NULL;
    }
    if(buf[l-1] == '\x0A') buf[l-1] = '\0';
    if(buf[l-2] == '\x0D') buf[l-2] = '\0';
    append_stream(st, i == *h - 1 ? 0 : 2, buf);
  }
  append_stream(st, 0, ">");
  fclose(fp);
  return st;
}

int load_image(PDF_OBJ *obj, char *fn)
{
  int w, h, c, b;
  create_image(obj, w, h, c, load_AHx(fn, &w, &h, &c, &b)); // right args first
  return 0;
}

int create_metr(PDF_OBJ *obj, char *face, int a, int b, int c, int d, char *st)
{
  create_obj(obj);
  fprintf(obj->atr->fp, "/Type /FontDescriptor\x0A");
  fprintf(obj->atr->fp, "/FontName /%s\x0A", face);
  fprintf(obj->atr->fp, "/Flags %d\x0A", 39);
  fprintf(obj->atr->fp, "/FontBBox [ %d %d %d %d ]\x0A", a, b, c, d);
  fprintf(obj->atr->fp, "/MissingWidth %d\x0A", 507);
  fprintf(obj->atr->fp, "/StemV %d\x0A", 92);
  fprintf(obj->atr->fp, "/StemH %d\x0A", 92);
  fprintf(obj->atr->fp, "/ItalicAngle %d\x0A", 0);
  fprintf(obj->atr->fp, "/CapHeight %d\x0A", 853);
  fprintf(obj->atr->fp, "/XHeight %d\x0A", 597);
  fprintf(obj->atr->fp, "/Ascent %d\x0A", 853);
  fprintf(obj->atr->fp, "/Descent %d\x0A", -147);
  fprintf(obj->atr->fp, "/Leading %d\x0A", 0);
  fprintf(obj->atr->fp, "/MaxWidth %d\x0A", 1000);
  fprintf(obj->atr->fp, "/AvgWidth %d\x0A", 507);
  fprintf(obj->atr->fp, "/Style << /Panose <%s> >>\x0A", st);
  flush_obj(obj);
  return 0;
}

int create_descf(PDF_OBJ *obj, PDF_OBJ *m, char *face)
{
  create_obj(obj);
  fprintf(obj->atr->fp, "/Type /Font\x0A");
  fprintf(obj->atr->fp, "/Subtype /%s\x0A", "CIDFontType2");
  fprintf(obj->atr->fp, "/BaseFont /%s\x0A", face);
  fprintf(obj->atr->fp, "/WinCharSet %d\x0A", 128);
  fprintf(obj->atr->fp, "/FontDescriptor %d %d R\x0A", m->id, m->gen);
  fprintf(obj->atr->fp, "/CIDSystemInfo\x0A");
  fprintf(obj->atr->fp, "<<\x0A");
  fprintf(obj->atr->fp, " /Registry(%s)\x0A", "Adobe");
  fprintf(obj->atr->fp, " /Ordering(%s)\x0A", "Japan1");
  fprintf(obj->atr->fp, " /Supplement %d\x0A", 2);
  fprintf(obj->atr->fp, ">>\x0A");
  fprintf(obj->atr->fp, "/DW %d\x0A", 1000);
  fprintf(obj->atr->fp, "/W [\x0A");
  fprintf(obj->atr->fp, " %d %d %d\x0A", 231, 389, 500);
  fprintf(obj->atr->fp, " %d %d %d\x0A", 631, 631, 500);
  fprintf(obj->atr->fp, "]\x0A");
  flush_obj(obj);
  return 0;
}

int create_font(PDF_OBJ *obj, PDF_OBJ *d, char *face, char *enc)
{
  create_obj(obj);
  fprintf(obj->atr->fp, "/Type /Font\x0A");
  fprintf(obj->atr->fp, "/Subtype /%s\x0A", "Type0");
  fprintf(obj->atr->fp, "/BaseFont /%s\x0A", face);
  fprintf(obj->atr->fp, "/DescendantFonts [ %d %d R ]\x0A", d->id, d->gen);
  fprintf(obj->atr->fp, "/Encoding /%s\x0A", enc);
  flush_obj(obj);
  return 0;
}

int create_resource(PDF_OBJ *obj)
{
  create_obj(obj);
  fprintf(obj->atr->fp, "/ProcSet [ /PDF /Text ]\x0A");
  // must call flush_obj() later
  return 0;
}

int add_resource(PDF_OBJ *obj, char *rn, char *ra, PDF_OBJ *r)
{
  fprintf(obj->atr->fp, "/%s << /%s %d %d R >>\x0A", rn, ra, r->id, r->gen);
  // must call flush_obj() later
  return 0;
}

int create_contents(PDF_OBJ *obj, PDF_STREAM *stream)
{
  create_xobj(obj, NULL, stream);
  flush_obj(obj);
  return 0;
}

int create_page(PDF_OBJ *obj, PDF_OBJ *res, PDF_OBJ *ct)
{
  create_obj(obj);
  fprintf(obj->atr->fp, "/Type /Page\x0A");
  fprintf(obj->atr->fp, "/Resources %d %d R\x0A", res->id, res->gen);
  fprintf(obj->atr->fp, "/Contents %d %d R\x0A", ct->id, ct->gen);
  // must call set_parent() later to flush_obj()
  return 0;
}

int set_parent(PDF_OBJ *obj, PDF_OBJ *pt)
{
  fprintf(obj->atr->fp, "/Parent %d %d R\x0A", pt->id, pt->gen);
  flush_obj(obj);
  return 0;
}

int create_pages(PDF_OBJ *obj, PDF_OBJ *p, int cnt, int x, int y, int w, int h)
{
  create_obj(obj);
  fprintf(obj->atr->fp, "/Type /Pages\x0A");
  fprintf(obj->atr->fp, "/Kids [");
  for(int i = 0; i < cnt; ++i){
    fprintf(obj->atr->fp, " %d %d R", p[i].id, p[i].gen);
    set_parent(&p[i], obj);
  }
  fprintf(obj->atr->fp, " ]\x0A");
  fprintf(obj->atr->fp, "/Count %d\x0A", cnt);
  fprintf(obj->atr->fp, "/MediaBox [ %d %d %d %d ]\x0A", x, y, w, h);
  flush_obj(obj);
  return 0;
}

int create_root(PDF_OBJ *obj, PDF_OBJ *pgs)
{
  create_obj(obj);
  fprintf(obj->atr->fp, "/Type /Catalog\x0A");
  fprintf(obj->atr->fp, "/Pages %d %d R\x0A", pgs->id, pgs->gen);
  flush_obj(obj);
  return 0;
}

int create_info(PDF_OBJ *obj, char *dt, char *ttl, char *ath, char *prd)
{
  create_obj(obj);
  fprintf(obj->atr->fp, "/CreationDate (D:%s)\x0A", dt);
  fprintf(obj->atr->fp, "/Title (%s)\x0A", ttl);
  fprintf(obj->atr->fp, "/Author (%s)\x0A", ath);
  fprintf(obj->atr->fp, "/Producer (%s)\x0A", prd);
  flush_obj(obj);
  return 0;
}

int out_objects(FILE *fp)
{
  char *p;
  int i, l;
  for(i = 1; i < XREF_MAX; ++i){
    l = fprintf(fp, "%d %d obj\x0D\x0A", XREF[i].obj->id, XREF[i].obj->gen);
    l += fprintf(fp, "<<\x0D\x0A");
    if(XREF[i].obj->atr){
      for(p = XREF[i].obj->atr->buf; p = strtok(p, "\x0A"); p += strlen(p) + 1)
        l += fprintf(fp, " %s\x0D\x0A", p);
      memstream_release(&XREF[i].obj->atr);
    }
    l += fprintf(fp, ">>\x0D\x0A");
    if(XREF[i].obj->stream){
      l += fprintf(fp, "stream\x0D\x0A");
      l += fwrite(XREF[i].obj->stream->buf, 1, XREF[i].obj->stream->l, fp);
      l += fprintf(fp, "\x0D\x0A");
      l += fprintf(fp, "endstream\x0D\x0A");
      release_stream(&XREF[i].obj->stream);
    }
    l += fprintf(fp, "endobj\x0D\x0A\x0D\x0A");
    XREF[i].len = l;
  }
  return 0;
}

int out_xref(FILE *fp)
{
  int i, offset = 0;
  fprintf(fp, "xref\x0D\x0A%d %d\x0D\x0A", 0, XREF_MAX);
  for(i = 0; i < XREF_MAX; ++i){
    fprintf(fp, PDF_IDX, offset, XREF[i].sub, XREF[i].kwd);
    offset += XREF[i].len;
  }
  return offset;
}

int out_trailer(FILE *fp, PDF_OBJ *root, PDF_OBJ *info)
{
  fprintf(fp, "trailer\x0D\x0A<<\x0D\x0A");
  fprintf(fp, " /Root %d %d R\x0D\x0A", root->id, root->gen);
  fprintf(fp, " /Info %d %d R\x0D\x0A", info->id, info->gen);
  fprintf(fp, " /Size %d\x0D\x0A", XREF_MAX);
  fprintf(fp, ">>\x0D\x0A");
  return 0;
}

PDF_STREAM *chomp(PDF_STREAM *st){ st->buf[st->l -= 2] = '\0'; return st; }
PDF_STREAM *oo(PDF_STREAM *st, char *p){ return append_stream(st, 0, p); }
PDF_STREAM *op(PDF_STREAM *st, char *p){ return append_stream(st, 2, p); }
PDF_STREAM *bq(PDF_STREAM *st){ return op(st, "q"); }
PDF_STREAM *eQ(PDF_STREAM *st){ return op(st, "Q"); }
PDF_STREAM *bt(PDF_STREAM *st){ return op(st, "BT"); }
PDF_STREAM *et(PDF_STREAM *st){ return op(st, "ET"); }
PDF_STREAM *bi(PDF_STREAM *st){ return op(st, "BI"); }
PDF_STREAM *ei(PDF_STREAM *st){ return op(st, "EI"); }
PDF_STREAM *id(PDF_STREAM *st){ return op(st, "ID"); }

PDF_STREAM *tf(PDF_STREAM *st, char *p, int sz)
{
  char buf[MAX_BUF];
  sprintf(buf, "/%s %d %s", p, sz, "Tf");
  return op(st, buf);
}

PDF_STREAM *tx(PDF_STREAM *st, int ah, char *p)
{
  char *bk[] = {"()", "<>"};
  char q[MAX_BUF];
  char buf[MAX_BUF];
  if(ah) hexstr(q, 0, p);
  sprintf(buf, "%c%s%c Tj", bk[ah][0], ah ? q : p, bk[ah][1]);
  return op(st, buf);
}

PDF_STREAM *qm(PDF_STREAM *st, int x, int y, int cx, int cy, int q, char *t)
{
  char *qc[] = {"cm", "Tm"};
  char buf[MAX_BUF];
  int l = 0;
  l += sprintf(buf + l, "%d %d %d %d %d %d %s", x, 0, 0, y, cx, cy, qc[q]);
  if(t) l += sprintf(buf + l, " %% %s", t);
  return op(st, buf);
}

PDF_STREAM *cm(PDF_STREAM *st, int x, int y, int cx, int cy, char *t)
{
  return qm(st, x, y, cx, cy, 0, t);
}

PDF_STREAM *Tm(PDF_STREAM *st, int x, int y, int cx, int cy, char *t)
{
  return qm(st, x, y, cx, cy, 1, t);
}

PDF_STREAM *rg(PDF_STREAM *st, int fS, float r, float g, float b)
{
  char buf[MAX_BUF];
  sprintf(buf, "%3.1f %3.1f %3.1f %s", r, g, b, fS ? "RG" : "rg");
  return op(st, buf);
}

PDF_STREAM *val(PDF_STREAM *st, char *p, float v)
{
  char buf[MAX_BUF];
  sprintf(buf, "%3.1f %s", v, p);
  return op(st, buf);
}

PDF_STREAM *poly(PDF_STREAM *st, int fS, int n, int *p) // 2 * n
{
  char buf[MAX_BUF];
  int i, k;
  for(i = 0; i < n; ++i){
    k = i * 2;
    sprintf(buf, "%d %d %s", p[k], p[k + 1], i ? "l" : "m");
    op(st, buf);
  }
  return op(st, fS ? "S" : "f");
}

PDF_STREAM *curve(PDF_STREAM *st, int fS, int n, int *p) // 2 * (1 + 3 * n)
{
  char buf[MAX_BUF];
  int i, j, k;
  sprintf(buf, "%d %d %s", p[0], p[1], "m");
  op(st, buf);
  for(i = 0; i < n; ++i){
    for(j = 0; j < 3; ++j){
      k = (1 + i * 3 + j) * 2;
      sprintf(buf, "%d %d ", p[k], p[k + 1]);
      oo(st, buf);
    }
    op(st, "c");
  }
  return op(st, fS ? "S" : "f");
}

PDF_STREAM *im(PDF_STREAM *st, char *p)
{
  char buf[MAX_BUF];
  sprintf(buf, "/%s Do", p);
  return op(st, buf);
}

PDF_STREAM *vs(PDF_STREAM *st, int f, char *p, ...) // f, p, (bnvs), t
{
  va_list va;
  va_start(va, p);
  int b, n;
  double v;
  char *s, *t;
  // f = 0: b(F/T), 1: n, 2: v, 3: s(add /), 4: s(pass through)
  char buf[MAX_BUF];
  int l = 0;
  l += sprintf(buf + l, "/%s ", p);
  switch(f){
  case 0:{ b = va_arg(va, int);
    l += sprintf(buf + l, "%s", b ? "true" : "false"); } break;
  case 1:{ n = va_arg(va, int); l += sprintf(buf + l, "%d", n); } break;
  case 2:{ v = va_arg(va, double); l += sprintf(buf + l, "%3.1f", v); } break;
  case 3:{ s = va_arg(va, char *); l += sprintf(buf + l, "/%s", s); } break;
  default:{ s = va_arg(va, char *); l += sprintf(buf + l, "%s", s); }
  }
  t = va_arg(va, char *);
  if(t) l += sprintf(buf + l, " %% %s", t);
  va_end(va);
  return op(st, buf);
}

PDF_STREAM *page_common()
{
  int a[] = {1, 1, 420, 1, 420, 296, 1, 296};
  int b[] = {-1, 1, -420, 1, -420, 296, -1, 296};
  int c[] = {-1, -1, -420, -1, -420, -296, -1, -296};
  int d[] = {1, -1, 420, -1, 420, -296, 1, -296};
  int e[] = {0, 0, 8, 0, 4, 12, 0, 12, -4, 12, -8, 0, 0, 0};
  int f[] = {105, 74};
  int i;
  PDF_STREAM *st = alloc_stream(MAX_STREAM);
  bq(st);
  cm(st, 1, 1, 421, 298, "1/72 (420, 296)");
  rg(st, 0, 0.0, 0.0, 0.5);
  poly(st, 0, sizeof(a) / sizeof(a[0]) / 2, a);
  rg(st, 0, 0.0, 0.5, 0.0);
  poly(st, 0, sizeof(b) / sizeof(b[0]) / 2, b);
  rg(st, 0, 0.5, 0.0, 0.0);
  poly(st, 0, sizeof(c) / sizeof(c[0]) / 2, c);
  rg(st, 0, 1.0, 1.0, 0.0);
  poly(st, 0, sizeof(d) / sizeof(d[0]) / 2, d);
  eQ(st);
  bq(st);
  cm(st, 2, 2, 421, 298, "2/72 (420->210, 296->148)");
  rg(st, 1, 0.9, 0.7, 0.2);
  val(st, "w", 2.0);
  for(i = 0; i < sizeof(e) / sizeof(e[0]) / 2; ++i)
    e[i * 2] += f[0], e[i * 2 + 1] += f[1];
  curve(st, 1, (sizeof(e) / sizeof(e[0]) / 2 - 1) / 3, e);
  eQ(st);
  return st;
}

PDF_STREAM *page0()
{
  PDF_STREAM *st = page_common();
  bt(st);
  cm(st, 2, 2, 421, 298, "2/72 (210, 148)");
  tf(st, FONT_ALIAS, 24);
  Tm(st, 1, 1, 1, 1, NULL);
  val(st, "g", 0.8);
  tx(st, 0, "(0, 0)");
  Tm(st, 1, 1, -100, -25, NULL);
  rg(st, 0, 0.8, 0.8, 0.0);
  tx(st, 1, TEXT_TEST);
  et(st);
  return chomp(st);
}

PDF_STREAM *page1()
{
  PDF_STREAM *st = page_common();
  bt(st);
  tf(st, FONT_ALIAS, 24);
  Tm(st, 2, 2, 420, 296, "2/72 (210, 148)");
  val(st, "g", 0.8);
  tx(st, 0, "(0, 0)");
  et(st);
  cm(st, 32, 64, 210, 512, "64/72 (420->26.25, 296->18.5) (296+216=512>32)");
  im(st, XOBJ_ALIAS);
  return chomp(st);
}

PDF_STREAM *page2()
{
  int w, h, c, b;
  PDF_STREAM *ahx;
  PDF_STREAM *st = page_common();
  bt(st);
  tf(st, FONT_ALIAS, 24);
  Tm(st, 1, 1, 421, 298, "1/72 (210, 148)");
  val(st, "g", 0.8);
  tx(st, 0, "(0, 0)");
  Tm(st, 1, 1, 321, 273, "1/72 210, 148");
  rg(st, 0, 0.8, 0.8, 0.0);
  tx(st, 1, TEXT_TEST);
  et(st);
  ahx = load_AHx(FILE_MSK, &w, &h, &c, &b);
  cm(st, w, h, 210, 512, "16/72 (420->26.25, 296->18.5) (296+216=512>32)");
  rg(st, 0, 0.8, 0.4, 0.8);
  bi(st);
  vs(st, 1, "W", w, "pixel");
  vs(st, 1, "H", h, "pixel");
  vs(st, 1, "BPC", c, "bits per component (16 x 64 x 3 / 8 = 384 bytes)");
  vs(st, 3, "F", "AHx", "filter ASCII Hex decode");
  vs(st, 0, "IM", !0, "mask");
  id(st);
  op(st, ahx->buf);
  ei(st);
  release_stream(&ahx);
  return chomp(st);
}

int cpdf(char *fname)
{
  char face[MAX_BUF];
  PDF_OBJ head, resource, metr, descf, font, xobj, pages, root, info;
  PDF_OBJ contents[3], page[3];
  int i, xref_offset;
  FILE *ofp = NULL;

  init_xref(&head);
  create_resource(&resource);

  hexstr(face, 1, FONT_NAME);
  create_metr(&metr, face, -150, -147, 1100, 853, FONT_STYLE);
  create_descf(&descf, &metr, face);
  create_font(&font, &descf, face, FONT_ENC);
  add_resource(&resource, RES_FONT, FONT_ALIAS, &font);

  create_contents(&contents[0], page0());
  create_page(&page[0], &resource, &contents[0]);

  load_image(&xobj, FILE_IMG0);
  add_resource(&resource, RES_XOBJ, XOBJ_ALIAS, &xobj);
  create_contents(&contents[1], page1());
  create_page(&page[1], &resource, &contents[1]);

  create_contents(&contents[2], page2());
  create_page(&page[2], &resource, &contents[2]);

  flush_obj(&resource);
  create_pages(&pages, page, sizeof(page) / sizeof(page[0]), 0, 0, 842, 595);
  create_root(&root, &pages);
  create_info(&info, INF_CREATIONDATE, INF_TITLE, INF_AUTHOR, INF_PRODUCER);

  if((ofp = fopen(fname, "wb")) == NULL){
    fprintf(stderr, "cannot output pdf: %s\n", fname);
    return -1;
  }
  fputs(PDF_HEAD, ofp);
  out_objects(ofp);
  xref_offset = out_xref(ofp);
  out_trailer(ofp, &root, &info);
  fprintf(ofp, "startxref\x0D\x0A%d\x0D\x0A", xref_offset);
  fputs(PDF_FOOT, ofp);
  fclose(ofp);

  fprintf(stdout, "done. [%s] (%08x)\n", fname, sizeof(size_t));
  return 0;
}

int main(int ac, char **av)
{
  if(ac < 2){
    fprintf(stderr, "Usage: %s outfile.pdf\n", av[0]);
    return -1;
  }
  if(chk_ext(av[1], EXT_PDF)){
    fprintf(stderr, "ext is not %s\n", EXT_PDF);
    return 1;
  }
  return cpdf(av[1]);
}
