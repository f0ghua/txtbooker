#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <gc.h>

#include "encodecvt.h"

// enc_convert(strA_in,strB_out,CP_UTF8,CP_ACP)			//UTF8 to ANSI
// enc_convert(strA_out, strB_in, CP_ACP, CP_UTF8)		//ANSI to UTF8
void enc_convert(const char *strIn, char *strOut, int sourceCodepage, int targetCodepage)
{
    int len = lstrlen(strIn);
    int unicodeLen = MultiByteToWideChar(sourceCodepage, 0, strIn, -1, NULL, 0);

    wchar_t* pUnicode = NULL;
    pUnicode = GC_malloc(sizeof(wchar_t)*(unicodeLen + 1));
    memset(pUnicode, 0, (unicodeLen + 1)*sizeof(wchar_t));
    MultiByteToWideChar(sourceCodepage, 0, strIn, -1, (LPWSTR)pUnicode, unicodeLen);

    BYTE * pTargetData = NULL;
    int targetLen = WideCharToMultiByte(targetCodepage, 0,
		(LPWSTR)pUnicode, -1, (char *)pTargetData, 0, NULL, NULL);

    pTargetData = GC_malloc(targetLen+1);
    memset(pTargetData, 0, targetLen + 1);
    WideCharToMultiByte(targetCodepage, 0, (LPWSTR)pUnicode,
		-1, (char *)pTargetData, targetLen, NULL, NULL);
    lstrcpy(strOut, (char*)pTargetData);

    GC_free(pUnicode);
    GC_free(pTargetData);
}
