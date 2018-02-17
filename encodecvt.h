#ifndef _ENCODECVT_H_
#define _ENCODECVT_H_

char* utf8_to_gbk(char *buf);
void utf8ascii(char *s);
void enc_convert(const char *strIn, char *strOut, int sourceCodepage, int targetCodepage);

#endif
