#ifndef _MSGPRINTF_H_
#define _MSGPRINTF_H_

#include <windows.h>
#include <tchar.h>
#include <stdio.h>

int MessageBoxPrintf(TCHAR *szFormat, ...);
void MessageDbgViewPrintf(TCHAR *szFormat, ...);

#define LOG MessageDbgViewPrintf

#endif
