#include "msgprintf.h"

int MessageBoxPrintf(TCHAR *szFormat, ...)
{
    const TCHAR szCaption[] = TEXT("DBGMSG");
    TCHAR szBuffer[1024];
    va_list pArgList;

    va_start(pArgList, szFormat);
    _vsntprintf(szBuffer, sizeof(szBuffer)/sizeof(TCHAR), szFormat, pArgList);
    va_end(pArgList);

    return MessageBox(NULL, szBuffer, szCaption, 0);
}

void MessageDbgViewPrintf(TCHAR *szFormat, ...)
{
    TCHAR szBuffer[1024];
    va_list pArgList;

    va_start(pArgList, szFormat);
    _vsntprintf(szBuffer, sizeof(szBuffer)/sizeof(TCHAR), szFormat, pArgList);
    va_end(pArgList);

	OutputDebugString(szBuffer);

	return;
}
