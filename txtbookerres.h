/* Weditres generated include file. Do NOT edit */
#include <windows.h>
#include <lfc.h>
#define	IDD_MAINDIALOG	100
#define	IDCBPSTART	102
#define	IDCBPEND	103
#define	IDPARSE	104
#define	IDGRAP	105
#define	IDGRAB	106
#define	IDURL	107
#define	IDCONTENT	108
#define	IDLBLREG	110
#define	IDEDTREG	111
#define	IDLBLPSTART	112
#define	IDPROGRESS	113
#define	IDPAGENUM	114
#define	IDLSTLOG	115
#define	IDICON200	200
/*@ Prototypes @*/
#ifndef WEDIT_PROTOTYPES
#define WEDIT_PROTOTYPES
/*
 * Structure for dialog Dlg100
 */
struct _Dlg100 {
	ST_LISTBOX *idlstlog;
	ST_STATIC *idpagenum;
	ST_PROGRESS *idprogress;
	ST_EDIT *idurl;
	ST_BUTTON *idgrab;
	ST_BUTTON *idparse;
	ST_STATIC *ctrl101;
	ST_COMBOBOX *idcbpend;
	ST_STATIC *idlblpstart;
	ST_COMBOBOX *idcbpstart;
	HWND hwnd;
	WPARAM wParam;
	LPARAM lParam;
};


#endif
void SetDlgBkColor(HWND,COLORREF);
BOOL APIENTRY HandleCtlColor(UINT,DWORD);
/*
 * Callbacks for dialog Dlg100
 */
HWND StartDlg100(HWND parent);
int RunDlg100(HWND parent);
void AddGdiObject(HWND,HANDLE);
BOOL WINAPI HandleDefaultMessages(HWND hwnd,UINT msg,WPARAM wParam,DWORD lParam);
/* Control: IDPARSE*/
long Dlg100ParseSelected(ST_BUTTON *,struct _Dlg100 *);
/* Control: IDGRAB*/
long Dlg100GrabSelected(ST_BUTTON *,struct _Dlg100 *);
long Dlg100Init(ST_DIALOGBOX *,struct _Dlg100 *);
BOOL APIENTRY Dlg100(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
extern void *GetDialogArguments(HWND);
extern char *GetDico(int,long);
/*@@ End Prototypes @@*/
