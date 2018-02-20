/*@@ Generated by Wedit @@ */
#include <windows.h>
#include <lfc.h>
#include <netutils.h>
#include <gc.h>
#include <process.h>

#include "txtbookerres.h"
#include "msgprintf.h"
#include "regex_helper.h"
#include "encodecvt.h"

//#define F_DEBUG_DLSPEED

#define MAX_STRLEN 256

typedef struct page_info {
	char title[MAX_STRLEN];
	char url[MAX_STRLEN];
} page_info_t;

typedef struct book_info {
	char url[MAX_STRLEN];
	int pages;
	page_info_t pi[1024*5]; // how many pages in a book?
} book_info_t;

typedef struct th_info {
	book_info_t bi;
	int start;
	int end;
	HWND hwnd_pgbar;
	HWND hwnd_pagenum;
} th_info_t;

book_info_t *g_pbi;

const char g_index_url[128] = "https://www.dawenxue.net/44214/";
const char g_pattern[] 		= "^[ \t]*<dd><a href=\"([^\"]*)\">([^<]*)<.*";
const char g_index_fname[] = "./tmp.html";
const char g_page_fname[] = "./page.html";

long Dlg100ParseSelected(ST_BUTTON *ctrl,struct _Dlg100 *dlg)
{
	char url[MAX_STRLEN], pattern[MAX_STRLEN];
	int i, r;
	FILE *fp;
	char line[1024];
	char ansiOut[1024];

	dlg->idurl->GetWindowText(url, sizeof(url));
	//strncpy(url, g_index_url, sizeof(url));

	r = GetHttpURL(url, g_index_fname);
	if (r != 0){
		int e = GetLastError();
		ERR("error of gethttpurl = %d", e);
		return -1;
	}

	memset(g_pbi, 0, sizeof(*g_pbi));
	dlg->idcbpstart->ResetContent();
	dlg->idcbpend->ResetContent();
	g_pbi->pages = 0;
	fp = fopen(g_index_fname, "r");
	while (1) {
		if (fgets(line, sizeof(line), fp) == NULL)
			break;

		enc_convert(line, ansiOut, CP_UTF8, CP_ACP);

		r = regex_match_ERE(ansiOut, g_pattern);
		if (r == 0) {
			snprintf(g_pbi->pi[g_pbi->pages].url, sizeof(g_pbi->pi[g_pbi->pages].url), "%s%s", url, REGEX_MATCH(1));
			//strncpy(g_pbi->pi[g_pbi->pages].url, REGEX_MATCH(1), sizeof(g_pbi->pi[g_pbi->pages].url));
			strncpy(g_pbi->pi[g_pbi->pages].title, REGEX_MATCH(2), sizeof(g_pbi->pi[g_pbi->pages].title));

			g_pbi->pages++;
		}
	}
	fclose(fp);

	LOG("pages = %d", g_pbi->pages);

	for (i = 0; i < g_pbi->pages; i++) {
		dlg->idcbpstart->AddString(g_pbi->pi[i].title);
		dlg->idcbpstart->SetCurSel(0);
		dlg->idcbpend->AddString(g_pbi->pi[i].title);
		dlg->idcbpend->SetCurSel(g_pbi->pages-1);
	}

	return 0;
}

static char *get_url_content(char *p_url)
{
	int r = 0;
	char *p_ansiOut;
	char *p_plain_buf;
	int plbuf_len = 0;
	FILE *fp;
    char *p_content = NULL;
    long bufsize = -1;

	const char pattern[] = "^[ \t]*<div id=\"content\">(.*)</div>";

	//LOG("get url = %s", p_url);
	r = GetHttpURL(p_url, g_page_fname);
	if (r != 0){
		int e = GetLastError();
		ERR("error[%d] of GetHttpURL = %s", e, p_url);
		return NULL;
	}

#ifndef F_DEBUG_DLSPEED
	fp = fopen(g_page_fname, "rb");
    if (fp != NULL) {
        /* Go to the end of the file. */
        if (fseek(fp, 0L, SEEK_END) == 0) {
            /* Get the size of the file. */
            bufsize = ftell(fp);
            if (bufsize == -1) { /* Error */
                fclose(fp);
                return NULL;
            }

			//LOG("read bufsize = %d", bufsize);
        }
        fclose(fp);
    }

	plbuf_len = sizeof(char) * (bufsize + 1) * 4; // gzip can be >2*len, but 4*len should enough
	p_plain_buf = GC_malloc(plbuf_len);
	read_gzip_file(g_page_fname, p_plain_buf, plbuf_len);

    p_ansiOut = GC_malloc(plbuf_len);
	enc_convert(p_plain_buf, p_ansiOut, CP_UTF8, CP_ACP);

	char *dup = strdup(p_ansiOut);
	char *line = strtok(dup, "\n");
	while(line) {

		r = regex_match_ERE(line, pattern);
		if (r == 0) {
			char *p = REGEX_MATCH(1);
			//LOG("found match: %s", line);
			qstrreplace("sr", p, "&nbsp;", " ");
			qstrreplace("sr", p, "<br /><br />", "\r\n");
			p_content = p;
			break;
		}
   		line = strtok(NULL, "\n");
	}
	free(dup);

    GC_free(p_plain_buf);
	GC_free(p_ansiOut);
#endif

	return p_content;
}

void thread_get_pages(void *arg)
{
	th_info_t *pth = (th_info_t *)arg;
	book_info_t *pbi = &pth->bi;
	const char outfname[] = "./out.txt";
	FILE *fp;
	char *p_content;

	int idx_start = pth->start;
	int idx_end = pth->end;

	//LOG("idx_start = %d, idx_end = %d", idx_start, idx_end);

	//dlg->idprogress->SetRange(0, idx_end-idx_start+1);
	//dlg->idprogress->SetStep(1);

	fp = fopen(outfname, "w");
	for (int i = idx_start; i <= idx_end; i++) {
		p_content = get_url_content(pbi->pi[i].url);
		if (p_content == NULL) {
#ifdef F_DEBUG_DLSPEED
			SendMessage(pth->hwnd_pgbar, PBM_SETPOS, (WPARAM)(i-idx_start+1), (LPARAM)0 );
			char nstr[16];
			sprintf(nstr, "%d", (i-idx_start+1));
			SendMessage(pth->hwnd_pagenum, WM_SETTEXT, (WPARAM)(strlen(nstr)), (LPARAM)(nstr) );
			continue;
#endif
			fclose(fp);
			return;
		}
		// display content seems not good for presentation
		//dlg->idcontent->SetWindowText(p_content);
		fwrite(pbi->pi[i].title, 1, strlen(pbi->pi[i].title), fp);
		fwrite("\r\n", 1, strlen("\r\n"), fp);
		fwrite(p_content, 1, strlen(p_content), fp);
		fwrite("\r\n", 1, strlen("\r\n"), fp);

		SendMessage(pth->hwnd_pgbar, PBM_SETPOS, (WPARAM)(i-idx_start+1), (LPARAM)0 );
		char nstr[16];
		sprintf(nstr, "%d", (i-idx_start+1));
		SendMessage(pth->hwnd_pagenum, WM_SETTEXT, (WPARAM)(strlen(nstr)), (LPARAM)(nstr) );
		//dlg->idprogress->SetPos(i-idx_start+1);
		//Sleep(500);
	}
	fclose(fp);

	remove(g_page_fname);
	remove(g_index_fname);

	return;
}

long Dlg100GrabSelected(ST_BUTTON *ctrl,struct _Dlg100 *dlg)
{
	th_info_t *p_thi;

	p_thi = GC_malloc(sizeof(*p_thi));
	memcpy(&p_thi->bi, g_pbi, sizeof(*g_pbi));
	p_thi->hwnd_pgbar = dlg->idprogress->WindowsParams->hwnd;
	p_thi->hwnd_pagenum = dlg->idpagenum->WindowsParams->hwnd;
	p_thi->start = dlg->idcbpstart->GetCurSel();
	p_thi->end = dlg->idcbpend->GetCurSel();

	dlg->idprogress->SetRange(0, p_thi->end-p_thi->start+1);
	dlg->idprogress->SetStep(1);

	beginthread(thread_get_pages, 0, p_thi);

	return 0;
}

long Dlg100Init(ST_DIALOGBOX *ctrl,struct _Dlg100 *dlg)
{
	int scrWidth, scrHeight;
    RECT rect;

	g_pbi = (book_info_t *)GC_malloc(sizeof(*g_pbi));
	dlg->idurl->SetWindowText(g_index_url);

    scrWidth = GetSystemMetrics(SM_CXSCREEN);
    scrHeight = GetSystemMetrics(SM_CYSCREEN);
    GetWindowRect(dlg->hwnd, &rect);
#ifndef F_NO_DEBUG
	//LOG("w = %d, h = %d, rect.r,b,l,t = %d,%d,%d,%d",
	//	scrWidth, scrHeight, rect.right, rect.bottom, rect.left, rect.top);
#endif
    SetWindowPos(dlg->hwnd, HWND_TOP,
                 (scrWidth - (rect.right-rect.left)) / 2,
                 (scrHeight - (rect.bottom-rect.top)) / 2,
                 rect.right - rect.left,
                 rect.bottom - rect.top,
                 SWP_SHOWWINDOW);

	return 0;
}

BOOL WINAPI Dlg100Default(HWND hwnd,UINT msg,UINT wParam,DWORD lParam)
{
	return 0;
}

void Dlg100Destroy(HWND hwnd)
{
	EndDialog(hwnd,1);
	GC_free(g_pbi);
	return;
}
