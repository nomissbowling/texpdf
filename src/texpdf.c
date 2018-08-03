/*
  PDF

  texpdf.c
  > gcc -m32 -o texpdf texpdf.c pdftx.c pdfobjects.c memstream.c (to 32 bit OK)
  > gcc -o texpdf texpdf.c pdftx.c pdfobjects.c memstream.c (to 64 bit OK)
  > texpdf _texpdf_out.pdf
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "memstream.h"
#include "pdfobjects.h"
#include "pdftx.h"

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

int chk_ext(char *fn, char *ext)
{
  char buf[EXT_LEN];
  int i, l;
  l = strlen(fn);
  if(l < EXT_LEN - 1) return -1;
  for(i = 0; i < EXT_LEN; ++i) buf[EXT_LEN - 1 - i] = (char)tolower(fn[l - i]);
  return strcmp(buf, ext);
}

MEM_STREAM *page_common()
{
  int a[] = {1, 1, 420, 1, 420, 296, 1, 296};
  int b[] = {-1, 1, -420, 1, -420, 296, -1, 296};
  int c[] = {-1, -1, -420, -1, -420, -296, -1, -296};
  int d[] = {1, -1, 420, -1, 420, -296, 1, -296};
  int e[] = {0, 0, 8, 0, 4, 12, 0, 12, -4, 12, -8, 0, 0, 0};
  int f[] = {105, 74};
  int i;
  MEM_STREAM *st = memstream_open(NULL, 0);
  if(!st) fprintf(stderr, "ERROR: page_common mstream_open" ALN);
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

MEM_STREAM *page0()
{
  MEM_STREAM *st = page_common();
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
  return flush_chomp(st, 2);
}

MEM_STREAM *page1()
{
  MEM_STREAM *st = page_common();
  bt(st);
  tf(st, FONT_ALIAS, 24);
  Tm(st, 2, 2, 420, 296, "2/72 (210, 148)");
  val(st, "g", 0.8);
  tx(st, 0, "(0, 0)");
  et(st);
  cm(st, 32, 64, 210, 512, "64/72 (420->26.25, 296->18.5) (296+216=512>32)");
  im(st, XOBJ_ALIAS);
  return flush_chomp(st, 2);
}

MEM_STREAM *page2()
{
  int w, h, c, b;
  MEM_STREAM *ahx = load_AHx(FILE_MSK, &w, &h, &c, &b);
  MEM_STREAM *st = page_common();
  bt(st);
  tf(st, FONT_ALIAS, 24);
  Tm(st, 1, 1, 421, 298, "1/72 (210, 148)");
  val(st, "g", 0.8);
  tx(st, 0, "(0, 0)");
  Tm(st, 1, 1, 321, 273, "1/72 210, 148");
  rg(st, 0, 0.8, 0.8, 0.0);
  tx(st, 1, TEXT_TEST);
  et(st);
  cm(st, w, h, 210, 512, "16/72 (420->26.25, 296->18.5) (296+216=512>32)");
  rg(st, 0, 0.8, 0.4, 0.8);
  bi(st);
  vs(st, 1, "W", w, "pixel");
  vs(st, 1, "H", h, "pixel");
  vs(st, 1, "BPC", c, "bits per component (16 x 64 x 3 / 8 = 384 bytes)");
  vs(st, 3, "F", "AHx", "filter ASCII Hex decode");
  vs(st, 0, "IM", !0, "mask");
  id(st);
  oq(st, 2, ahx);
  ei(st);
  memstream_release(&ahx);
  return flush_chomp(st, 2);
}

int cpdf(char *fname)
{
  PDF_OBJ head, resource, metr, descf, font, xobj, pages, root, info;
  PDF_OBJ contents[3], page[3];
  MEM_STREAM *face = hexstr(1, FONT_NAME);

  init_xref(&head);
  create_resource(&resource);

  create_metr(&metr, face->buf, -150, -147, 1100, 853, FONT_STYLE);
  create_descf(&descf, &metr, face->buf);
  create_font(&font, &descf, face->buf, FONT_ENC);
  add_resource(&resource, RES_FONT, FONT_ALIAS, &font);
  memstream_release(&face);

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
  out_pdf(fname, &info, &root);

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
