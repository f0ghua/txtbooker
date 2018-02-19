#ifndef _ENCODECVT_H_
#define _ENCODECVT_H_

void enc_convert(const char *strIn, char *strOut, int sourceCodepage, int targetCodepage);
int read_gzip_file(const char *fname, char *p_plain_buf, int buflen);
char *qstrreplace(const char *mode, char *srcstr, const char *tokstr, const char *word);

#endif
