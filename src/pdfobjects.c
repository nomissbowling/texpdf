/*
  pdfobjects.c
*/

#include "pdfobjects.h"

MEM_STREAM *append_stream(MEM_STREAM *st, int n, char *str, int sz)
{
  static char *e[] = {"", LF, LN};
  if(!st || !str) return st;
  if(sz) fwrite(str, 1, sz, st->fp);
  else fprintf(st->fp, "%s", str); // use %s to no care %%
  fprintf(st->fp, e[n]);
  return st;
}

MEM_STREAM *load_stream(char *str)
{
  return append_stream(memstream_open(NULL, 0), 0, str, 0);
}

MEM_STREAM *flush_chomp(MEM_STREAM *st, int n)
{
  if(st){
    if(!memstream_written(st, 0, NULL, NULL))
      fprintf(stderr, "ERROR: flush_chomp memstream_written"ALN);
    memstream_close(st);
    if(n) st->buf[st->sz -= n] = '\0';
  }
  return st;
}

int flush_obj(PDF_OBJ *obj)
{
  if(obj->atr){
    if(!memstream_written(obj->atr, 1, NULL, NULL))
      fprintf(stderr, "ERROR: obj->atr memstream_written"ALN);
    memstream_close(obj->atr);
  }
  return 0;
}

PDF_CTX *init_pdf()
{
  PDF_OBJ *obj;
  PDF_CTX *ctx = (PDF_CTX *)malloc(sizeof(PDF_CTX));
  if(!ctx) return NULL;
  ctx->XREF_MAX = 0;
  obj = &ctx->head;
  obj->id = ctx->XREF_MAX++;
  obj->gen = 0;
  obj->atr = NULL;
  obj->stream = NULL;
  ctx->XREF[0].obj = obj;
  ctx->XREF[0].len = strlen(PDF_HEAD);
  ctx->XREF[0].sub = 65535;
  ctx->XREF[0].kwd = 'f';
  create_resource(ctx, &ctx->resource);
  return ctx;
}

int release_pdf(PDF_CTX **ctx)
{
  if(ctx && *ctx){ free(*ctx); *ctx = NULL; };
  return 0;
}

int create_obj(PDF_CTX *ctx, PDF_OBJ *obj)
{
  obj->id = ctx->XREF_MAX++;
  obj->gen = 0;
  obj->atr = memstream_open(NULL, 0);
  obj->stream = NULL;
  ctx->XREF[obj->id].obj = obj;
  ctx->XREF[obj->id].len = 0;
  ctx->XREF[obj->id].sub = 0;
  ctx->XREF[obj->id].kwd = 'n';
  if(ctx->XREF_MAX >= MAX_IDX_LST)
    fprintf(stderr, "overflow obj->id"ALN);
  if(!obj->atr)
    fprintf(stderr, "ERROR: obj->atr memstream_open"ALN);
  // must call flush_obj() later
  return 0;
}

int create_xobj(PDF_CTX *ctx, PDF_OBJ *obj,
  MEM_STREAM *atr, MEM_STREAM *stream)
{
  create_obj(ctx, obj);
  fprintf(obj->atr->fp, "%s/Length %d"LF, atr ? atr->buf : "", stream->sz);
  obj->stream = stream;
  // must call flush_obj() later
  return 0;
}

int create_image(PDF_CTX *ctx, PDF_OBJ *obj,
  int w, int h, int bpc, MEM_STREAM *stream)
{
  create_xobj(ctx, obj, NULL, stream);
  fprintf(obj->atr->fp, "/Type /XObject"LF);
  fprintf(obj->atr->fp, "/Subtype /Image"LF);
  fprintf(obj->atr->fp, "/Width %d"LF, w);
  fprintf(obj->atr->fp, "/Height %d"LF, h);
  fprintf(obj->atr->fp, "/BitsPerComponent %d"LF, bpc);
  fprintf(obj->atr->fp, "/ColorSpace /DeviceRGB"LF);
  fprintf(obj->atr->fp, "/Filter [ /AHx ]"LF);
  flush_obj(obj);
  return 0;
}

MEM_STREAM *load_AHx(char *fn, int *w, int *h, int *c, int *b)
{
  MEM_STREAM *st;
  char buf[MAX_BUF];
  int v, i, l;
  FILE *fp;
  if(!(fp = fopen(fn, "rb"))){
    fprintf(stderr, "file not found [%s]"ALN, fn);
    return NULL;
  }
  if(!fgets(buf, sizeof(buf), fp)){
    fprintf(stderr, "unexpected EOF [%s] (0)"ALN, fn);
    fclose(fp);
    return NULL;
  }
  if(sscanf(buf, "%d %d %d %d", w, h, c, b) != 4){
    fprintf(stderr, "not found (W H C B) [%s] (0)"ALN, fn);
    fclose(fp);
    return NULL;
  }
  v = *w * 2 * *b * *c / 8 + 2;
#if 0
  if(1){
    int sz = *h * v + 1;
    fprintf(stdout, "%d, %d, %d, %d, (%d), (%d)"ALN, *w, *h, *c, *b, v, sz);
  }
#endif
  if(!(st = memstream_open(NULL, 0))){
    fprintf(stderr, "ERROR: image mstream_open"ALN);
    fclose(fp);
    return NULL;
  }
  for(i = 0; i < *h; ++i){
    fgets(buf, sizeof(buf), fp);
    if((l = strlen(buf)) < v - 2){
      fprintf(stderr, "too short line length [%s] (%d)"ALN, fn, i + 1);
      fclose(fp);
      memstream_release(&st);
      return NULL;
    }
    if(buf[l-1] == '\x0A') buf[l-1] = '\0';
    if(buf[l-2] == '\x0D') buf[l-2] = '\0';
    append_stream(st, i == *h - 1 ? 0 : 2, buf, 0);
  }
  fclose(fp);
  append_stream(st, 0, ">", 1);
  return flush_chomp(st, 0); // written and close
}

int load_image(PDF_CTX *ctx, PDF_OBJ *obj, char *fn)
{
  int w, h, c, b;
  create_image(ctx, obj, w, h, c, load_AHx(fn, &w, &h, &c, &b)); // right args first
  return 0;
}

int create_metr(PDF_CTX *ctx, PDF_OBJ *obj,
  char *face, int a, int b, int c, int d, char *st)
{
  create_obj(ctx, obj);
  fprintf(obj->atr->fp, "/Type /FontDescriptor"LF);
  fprintf(obj->atr->fp, "/FontName /%s"LF, face);
  fprintf(obj->atr->fp, "/Flags %d"LF, 39);
  fprintf(obj->atr->fp, "/FontBBox [ %d %d %d %d ]"LF, a, b, c, d);
  fprintf(obj->atr->fp, "/MissingWidth %d"LF, 507);
  fprintf(obj->atr->fp, "/StemV %d"LF, 92);
  fprintf(obj->atr->fp, "/StemH %d"LF, 92);
  fprintf(obj->atr->fp, "/ItalicAngle %d"LF, 0);
  fprintf(obj->atr->fp, "/CapHeight %d"LF, 853);
  fprintf(obj->atr->fp, "/XHeight %d"LF, 597);
  fprintf(obj->atr->fp, "/Ascent %d"LF, 853);
  fprintf(obj->atr->fp, "/Descent %d"LF, -147);
  fprintf(obj->atr->fp, "/Leading %d"LF, 0);
  fprintf(obj->atr->fp, "/MaxWidth %d"LF, 1000);
  fprintf(obj->atr->fp, "/AvgWidth %d"LF, 507);
  fprintf(obj->atr->fp, "/Style << /Panose <%s> >>"LF, st);
  flush_obj(obj);
  return 0;
}

int create_descf(PDF_CTX *ctx, PDF_OBJ *obj, PDF_OBJ *m, char *face)
{
  create_obj(ctx, obj);
  fprintf(obj->atr->fp, "/Type /Font"LF);
  fprintf(obj->atr->fp, "/Subtype /%s"LF, "CIDFontType2");
  fprintf(obj->atr->fp, "/BaseFont /%s"LF, face);
  fprintf(obj->atr->fp, "/WinCharSet %d"LF, 128);
  fprintf(obj->atr->fp, "/FontDescriptor %d %d R"LF, m->id, m->gen);
  fprintf(obj->atr->fp, "/CIDSystemInfo"LF);
  fprintf(obj->atr->fp, "<<"LF);
  fprintf(obj->atr->fp, " /Registry(%s)"LF, "Adobe");
  fprintf(obj->atr->fp, " /Ordering(%s)"LF, "Japan1");
  fprintf(obj->atr->fp, " /Supplement %d"LF, 2);
  fprintf(obj->atr->fp, ">>"LF);
  fprintf(obj->atr->fp, "/DW %d"LF, 1000);
  fprintf(obj->atr->fp, "/W ["LF);
  fprintf(obj->atr->fp, " %d %d %d"LF, 231, 389, 500);
  fprintf(obj->atr->fp, " %d %d %d"LF, 631, 631, 500);
  fprintf(obj->atr->fp, "]"LF);
  flush_obj(obj);
  return 0;
}

int create_font(PDF_CTX *ctx, PDF_OBJ *obj, PDF_OBJ *d, char *face, char *enc)
{
  create_obj(ctx, obj);
  fprintf(obj->atr->fp, "/Type /Font"LF);
  fprintf(obj->atr->fp, "/Subtype /%s"LF, "Type0");
  fprintf(obj->atr->fp, "/BaseFont /%s"LF, face);
  fprintf(obj->atr->fp, "/DescendantFonts [ %d %d R ]"LF, d->id, d->gen);
  fprintf(obj->atr->fp, "/Encoding /%s"LF, enc);
  flush_obj(obj);
  return 0;
}

int create_resource(PDF_CTX *ctx, PDF_OBJ *obj)
{
  create_obj(ctx, obj);
  fprintf(obj->atr->fp, "/ProcSet [ /PDF /Text ]"LF);
  // must call flush_obj() later
  return 0;
}

int add_resource(PDF_OBJ *obj, char *rn, char *ra, PDF_OBJ *r)
{
  fprintf(obj->atr->fp, "/%s << /%s %d %d R >>"LF, rn, ra, r->id, r->gen);
  // must call flush_obj() later
  return 0;
}

int create_contents(PDF_CTX *ctx, PDF_OBJ *obj, MEM_STREAM *stream)
{
  create_xobj(ctx, obj, NULL, stream);
  flush_obj(obj);
  return 0;
}

int create_page(PDF_CTX *ctx, PDF_OBJ *obj, PDF_OBJ *res, PDF_OBJ *ct)
{
  create_obj(ctx, obj);
  fprintf(obj->atr->fp, "/Type /Page"LF);
  fprintf(obj->atr->fp, "/Resources %d %d R"LF, res->id, res->gen);
  fprintf(obj->atr->fp, "/Contents %d %d R"LF, ct->id, ct->gen);
  // must call set_parent() later to flush_obj()
  return 0;
}

int set_parent(PDF_OBJ *obj, PDF_OBJ *pt)
{
  fprintf(obj->atr->fp, "/Parent %d %d R"LF, pt->id, pt->gen);
  flush_obj(obj);
  return 0;
}

int create_pages(PDF_CTX *ctx, PDF_OBJ *obj,
  PDF_OBJ *p, int cnt, int x, int y, int w, int h)
{
  create_obj(ctx, obj);
  fprintf(obj->atr->fp, "/Type /Pages"LF);
  fprintf(obj->atr->fp, "/Kids [");
  for(int i = 0; i < cnt; ++i){
    fprintf(obj->atr->fp, " %d %d R", p[i].id, p[i].gen);
    set_parent(&p[i], obj);
  }
  fprintf(obj->atr->fp, " ]"LF);
  fprintf(obj->atr->fp, "/Count %d"LF, cnt);
  fprintf(obj->atr->fp, "/MediaBox [ %d %d %d %d ]"LF, x, y, w, h);
  flush_obj(obj);
  return 0;
}

int create_root(PDF_CTX *ctx, PDF_OBJ *obj, PDF_OBJ *pgs)
{
  create_obj(ctx, obj);
  fprintf(obj->atr->fp, "/Type /Catalog"LF);
  fprintf(obj->atr->fp, "/Pages %d %d R"LF, pgs->id, pgs->gen);
  flush_obj(obj);
  return 0;
}

int create_info(PDF_CTX *ctx, PDF_OBJ *obj,
  char *dt, char *ttl, char *ath, char *prd)
{
  create_obj(ctx, obj);
  fprintf(obj->atr->fp, "/CreationDate (D:%s)"LF, dt);
  fprintf(obj->atr->fp, "/Title (%s)"LF, ttl);
  fprintf(obj->atr->fp, "/Author (%s)"LF, ath);
  fprintf(obj->atr->fp, "/Producer (%s)"LF, prd);
  flush_obj(obj);
  return 0;
}

int out_head(FILE *fp)
{
  fputs(PDF_HEAD, fp);
  return 0;
}

int out_objects(FILE *fp, PDF_CTX *ctx)
{
  char *p;
  int i, l;
  for(i = 1; i < ctx->XREF_MAX; ++i){
    PDF_OBJ *obj = ctx->XREF[i].obj;
    l = fprintf(fp, "%d %d obj"LN, obj->id, obj->gen);
    l += fprintf(fp, "<<"LN);
    if(obj->atr){
      for(p = obj->atr->buf; p = strtok(p, LF); p += strlen(p) + 1)
        l += fprintf(fp, " %s"LN, p);
      memstream_release(&obj->atr);
    }
    l += fprintf(fp, ">>"LN);
    if(obj->stream){
      l += fprintf(fp, "stream"LN);
      l += fwrite(obj->stream->buf, 1, obj->stream->sz, fp);
      l += fprintf(fp, LN);
      l += fprintf(fp, "endstream"LN);
      memstream_release(&obj->stream);
    }
    l += fprintf(fp, "endobj"LN LN);
    ctx->XREF[i].len = l;
  }
  return 0;
}

int out_xref(FILE *fp, PDF_CTX *ctx)
{
  int i, offset = 0;
  fprintf(fp, "xref"LN"%d %d"LN, 0, ctx->XREF_MAX);
  for(i = 0; i < ctx->XREF_MAX; ++i){
    fprintf(fp, PDF_IDX, offset, ctx->XREF[i].sub, ctx->XREF[i].kwd);
    offset += ctx->XREF[i].len;
  }
  return offset;
}

int out_trailer(FILE *fp, PDF_CTX *ctx)
{
  fprintf(fp, "trailer"LN"<<"LN);
  fprintf(fp, " /Root %d %d R"LN, ctx->root.id, ctx->root.gen);
  fprintf(fp, " /Info %d %d R"LN, ctx->info.id, ctx->info.gen);
  fprintf(fp, " /Size %d"LN, ctx->XREF_MAX);
  fprintf(fp, ">>"LN);
  return 0;
}

int out_foot(FILE *fp, int offset)
{
  fprintf(fp, "startxref"LN"%d"LN, offset);
  fputs(PDF_FOOT, fp);
  return 0;
}

int merge_pdf(PDF_CTX *ctx, PDF_OBJ *p, int cnt, int x, int y, int w, int h,
  char *dt, char *ttl, char *ath, char *prd)
{
  flush_obj(&ctx->resource);
  create_pages(ctx, &ctx->pages, p, cnt, x, y, w, h);
  create_root(ctx, &ctx->root, &ctx->pages);
  create_info(ctx, &ctx->info, dt, ttl, ath, prd);
  return 0;
}

int out_pdf(char *fname, PDF_CTX *ctx)
{
  int xref_offset;
  FILE *ofp = fopen(fname, "wb");
  if(!ofp){
    fprintf(stderr, "cannot output pdf: %s"ALN, fname);
    return -1;
  }
  out_head(ofp);
  out_objects(ofp, ctx);
  xref_offset = out_xref(ofp, ctx);
  out_trailer(ofp, ctx);
  out_foot(ofp, xref_offset);
  fclose(ofp);
  return 0;
}
