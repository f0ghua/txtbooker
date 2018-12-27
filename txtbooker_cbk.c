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
#include "ini.h"

//#define F_NO_DEBUG
//#define F_DEBUG_DLSPEED

#define MAX_STRLEN 256

#define CPYMATCH2STR(i, s, l)                       \
	do {                                            \
        int len = prog->endp[i] - prog->startp[i];  \
        len = (len > l)?l:len;                      \
        memcpy(s, prog->startp[i], len);            \
        (s)[len] = '\0';                            \
    } while (0);

typedef struct _page_info {
	char title[MAX_STRLEN];
	char url[MAX_STRLEN];
} page_info_t;

typedef struct _site_info {
	char domain[MAX_STRLEN];
	char title_pattern[MAX_STRLEN];
	char link_str_start[MAX_STRLEN];
	char link_str_end[MAX_STRLEN];
	char link_pattern[MAX_STRLEN];
	char content_pattern[MAX_STRLEN];
    int force_utf8;
} site_info_t;

typedef struct _book_info {
	char url[MAX_STRLEN];
	int pages;
	page_info_t pi[1024*5]; // how many pages in a book?
} book_info_t;

typedef struct _th_info {
	book_info_t bi;
	site_info_t si;
	int start;
	int end;
	HWND hwnd_pgbar;
	HWND hwnd_pagenum;
	HWND hwnd_log;
} th_info_t;

book_info_t *g_pbi;
site_info_t *g_psi;

const char g_config_fname[] 	= "./config.ini";
const char g_index_fname[] 		= "./tmp.html";
const char g_page_fname[] 		= "./page.html";

const char *g_index_url 		= "https://www.dawenxue.net/937/";
const char *g_pattern 			= "^[ \t]*<dd><a href=\"([^\"]*)\">([^<]*)<.*";
const char *g_title_pattern 	= "";
const char *g_link_str_start 	= "<div id=\"list\">";
const char *g_link_str_end 		= "</div>";
const char *g_link_pattern 		= "<dd><a href=\"([^\"]*)\">([^<]*)</a></dd>";
const char *g_content_pattern 	= "";

/*
chuangshi.qq.com

const char *g_link_str_start 	= "<div class=\"index_area\">";
const char *g_link_str_end 		= "<div class=\"footer\">";
const char *g_link_pattern 		= "<li><a href=\"([^\"]*)\"[^>]*><b class=\"title\">([^<]*)</b></a></li>";
*/

static int load_config(char *url)
{
	ini_t *config;
	char domain[MAX_STRLEN];

	int r = regex_match_ERE(url, "https?://([^/]*).*");
	if (r != 0) {
		ERR("URL link is wrong");
		return -1;
	}

	strncpy(domain, REGEX_MATCH(1), sizeof(domain));
#ifndef F_NO_DEBUG
	LOG("domain = %s", domain);
#endif
	config = ini_load(g_config_fname);
	if (config == NULL) {
		ERR("load config file error");
		return -1;
	}

	const char *p_tpattern = ini_get(config, domain, "title_pattern");
	const char *p_cpattern = ini_get(config, domain, "content_pattern");
	const char *p_lstart = ini_get(config, domain, "link_str_start");
	const char *p_lend = ini_get(config, domain, "link_str_end");
	const char *p_lpattern = ini_get(config, domain, "link_pattern");
    const char *p_lforceutf8 = ini_get(config, domain, "force_utf8");

	if ((p_lstart == NULL)||(p_lend == NULL)||(p_lpattern == NULL)||
		(p_cpattern == NULL)) {
		ERR("The site has not supported, please contact the author.");
		return -1;
	}

	strncpy(g_psi->title_pattern, g_title_pattern, sizeof(g_psi->title_pattern));
	strncpy(g_psi->link_str_start, p_lstart, sizeof(g_psi->link_str_start));
	strncpy(g_psi->link_str_end, p_lend, sizeof(g_psi->link_str_end));
	strncpy(g_psi->link_pattern, p_lpattern, sizeof(g_psi->link_pattern));
	strncpy(g_psi->content_pattern, p_cpattern, sizeof(g_psi->content_pattern));

    if (p_lforceutf8 != NULL) {
        g_psi->force_utf8 = 1;
    } else {
        g_psi->force_utf8 = 0;
    }

	ini_free(config);

	return 0;
}

// function which call this subroutine should use GC_free to free the buffer
// later
static char *read_file_all(char *fname, int *bufsize)
{
	char *p_content;
    char *p_ansiOut;
	FILE *fp;
    size_t len = 0;
    int size, plbuf_len;

	*bufsize = 0;

    fp = fopen(fname, "rb");
    if (fp != NULL) {
        /* Go to the end of the file. */
        if (fseek(fp, 0L, SEEK_END) == 0) {
            /* Get the size of the file. */
            size = ftell(fp);
            if (size == -1) { /* Error */
                fclose(fp);
                return NULL;
            }
            /* Go back to the start of the file. */
            if (fseek(fp, 0L, SEEK_SET) != 0) {
                /* Handle error here */
                fclose(fp);
                return NULL;
            }
			/* Allocate our buffer to that size. */
            p_content = GC_malloc(sizeof(char) * (size + 1));
            /* Read the entire file into memory. */
            len = fread(p_content, sizeof(char), size, fp);
#ifndef F_NO_DEBUG
			LOG("read len = %d, size = %d", len, size);
#endif
            if (len == 0) {
                ERR("Error reading file", stderr);
                fclose(fp);
				GC_free(p_content);
                return NULL;
            } else {
                p_content[++len] = '\0'; /* Just to be safe. */
            }
        }

        // gzip can be >2*len, but 4*len should enough
        plbuf_len = sizeof(char) * (size + 1) * 4;
        if (((unsigned char)(*p_content) == 0x1F) &&
            ((unsigned char)*(p_content+1) == 0x8B)) { // gzip file
#ifndef F_NO_DEBUG
            LOG("gzip file detected, uncompress it");
#endif
            GC_free(p_content);
            p_content = GC_malloc(plbuf_len);
            read_gzip_file(fname, p_content, &plbuf_len);
#ifndef F_NO_DEBUG
            LOG("gzip file %d -> %d", size, plbuf_len);
#endif
        }

        if (g_psi->force_utf8 ||
            (((unsigned char)(*p_content) == 0xEF) &&
             ((unsigned char)*(p_content+1) == 0xBB) &&
             ((unsigned char)*(p_content+2) == 0xBF))) { // UTF8
            // convert UTF8 to GB2312(ASCII)
#ifndef F_NO_DEBUG
            LOG("file is UTF8, convert to ASCII");
#endif
            p_ansiOut = utf8_to_ansi(p_content);
            GC_free(p_content);
            p_content = p_ansiOut;
        }

        fclose(fp);
    }

	return p_content;
}

long Dlg100ParseSelected(ST_BUTTON *ctrl,struct _Dlg100 *dlg)
{
	char url[MAX_STRLEN], pattern[MAX_STRLEN];
	int i, r;
	FILE *fp;
	char *p, *s, *e;
	char *p_lstart, *p_lend;
	//char line[1024];
	int bufsiz;
	char *p_content;
	char *p_ansiOut;
	regexp *prog;

	dlg->idurl->GetWindowText(url, sizeof(url));
	//strncpy(url, g_index_url, sizeof(url));

	if (load_config(url) != 0) {
		return -1;
	}

	r = GetHttpURL(url, g_index_fname);
	if (r != 0){
		int e = GetLastError();
		ERR("error of gethttpurl = %d", e);
		return -1;
	}

	dlg->idcbpstart->ResetContent();
	dlg->idcbpend->ResetContent();

	memset(g_pbi, 0, sizeof(*g_pbi));
	g_pbi->pages = 0;

	p_content = read_file_all(g_index_fname, &bufsiz);
	if (p_content == NULL) {
		return -1;
	}

	p_ansiOut = p_content;

	if ((p_lstart = strstr(p_ansiOut, g_psi->link_str_start)) == NULL) {
		ERR("link start is not found");
		return -1;
	}

	if ((p_lend = strstr(p_lstart, g_psi->link_str_end)) == NULL) {
		ERR("link start is not found");
		return -1;
	}

	snprintf(pattern, sizeof(pattern), "^%s", g_psi->link_pattern);
	if ((prog = regcomp(pattern)) == NULL) {
		ERR("regcomp error");
        return -1;
    }

	p = p_lstart;
#ifndef F_NO_DEBUG
    //LOG("p = %s", p);
#endif
	while (p < p_lend) {
		if ((*p == *g_psi->link_pattern) &&
			((r = regexec(prog, p)) != 0)) { // match
			char stmp[MAX_STRLEN];

			CPYMATCH2STR(1, stmp, MAX_STRLEN);
            snprintf(g_pbi->pi[g_pbi->pages].url,
                     sizeof(g_pbi->pi[g_pbi->pages].url), "%s%s",
                     url, stmp);

			CPYMATCH2STR(2, stmp, MAX_STRLEN);
			strncpy(g_pbi->pi[g_pbi->pages].title,
                    stmp, sizeof(g_pbi->pi[g_pbi->pages].title));
			g_pbi->pages++;

			p += prog->endp[0] - prog->startp[0];

			CPYMATCH2STR(0, stmp, MAX_STRLEN);
#ifndef F_NO_DEBUG
			//LOG("prog0 = %s", stmp);
			//LOG("match p = %x, %x, %x, title = %s", p, prog->startp[0], prog->endp[0], stmp);
#endif
    	} else {
			p++;
			//LOG("no match p = %x", p);
		}
	}

	free(p_content);

#ifndef F_NO_DEBUG
	LOG("pages = %d", g_pbi->pages);
#endif

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
	FILE *fp;
    char *p_content = NULL;
    long bufsize = -1;
	const char *pattern = g_psi->content_pattern;

	r = GetHttpURL(p_url, g_page_fname);
	if (r != 0){
		int e = GetLastError();
		ERR("error[%d] of GetHttpURL = %s", e, p_url);
		return NULL;
	}

#ifndef F_DEBUG_DLSPEED
    p_ansiOut = read_file_all(g_page_fname, &bufsize);

	qstrreplace("tr", p_ansiOut, "\r\n", "");
    qstrreplace("sr", p_ansiOut, "<br />", "@");
    qstrreplace("sr", p_ansiOut, "<br/>", "@");
	qstrreplace("sr", p_ansiOut, "</br>", "@");

    //LOG("p_ansiOut = %s", p_ansiOut);

    r = regex_match_ERE(p_ansiOut, pattern);
#ifndef F_NO_DEBUG
	LOG("content match = %d", r);
#endif
    if (r == 0) {
        char *p = REGEX_MATCH(1);

		char *p_dbc = GC_malloc(strlen(p)+1);
		sbc_to_dbc(p, p_dbc);
		p = p_dbc;
        qstrreplace("sr", p, "&nbsp;", " ");
        qstrreplace("sr", p, " ", "");
        qstrreplace("tr", p, "\r\n\t", "");
        qstrreplace("sr", p, "@@", "@");
        qstrreplace("sr", p, "@", "\r\n");

        p_content = p;
    }

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
#ifndef F_NO_DEBUG
		LOG("get title = %s, url = %s", pbi->pi[i].title, pbi->pi[i].url);
#endif
		p_content = get_url_content(pbi->pi[i].url);
		if (p_content == NULL) {
#ifdef F_DEBUG_DLSPEED
			SendMessage(pth->hwnd_pgbar, PBM_SETPOS, (WPARAM)(i-idx_start+1), (LPARAM)0 );
			char nstr[16];
			sprintf(nstr, "%d", (i-idx_start+1));
			SendMessage(pth->hwnd_pagenum, WM_SETTEXT, (WPARAM)(strlen(nstr)), (LPARAM)(nstr) );
			char logstr[256];
			sprintf(logstr, "GET PAGE: %s", pbi->pi[i].title);
			SendMessage(pth->hwnd_log, LB_INSERTSTRING, (WPARAM)(0), (LPARAM)(logstr) );
			//if (((i-idx_start+1)%200) == 0) {
			//	Sleep(5000);
			//}
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
		sprintf(nstr, "%d/%d", (i-idx_start+1), idx_end-idx_start+1);
		SendMessage(pth->hwnd_pagenum, WM_SETTEXT, (WPARAM)(strlen(nstr)), (LPARAM)(nstr) );
		char logstr[256];
		sprintf(logstr, "GET PAGE: %s", pbi->pi[i].title);
		SendMessage(pth->hwnd_log, LB_INSERTSTRING, (WPARAM)(0), (LPARAM)(logstr));
	}

	fclose(fp);

    GC_free(p_content);
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
	p_thi->hwnd_log = dlg->idlstlog->WindowsParams->hwnd;

	p_thi->start = dlg->idcbpstart->GetCurSel();
	p_thi->end = dlg->idcbpend->GetCurSel();

	dlg->idprogress->SetRange(0, p_thi->end-p_thi->start+1);
	dlg->idprogress->SetStep(1);
	dlg->idlstlog->ResetContent();

	beginthread(thread_get_pages, 0, p_thi);

	return 0;
}

long Dlg100Init(ST_DIALOGBOX *ctrl,struct _Dlg100 *dlg)
{
	int scrWidth, scrHeight;
    RECT rect;

	// init the global structure
	g_pbi = (book_info_t *)GC_malloc(sizeof(*g_pbi));
	g_psi = (site_info_t *)GC_malloc(sizeof(*g_psi));

	dlg->idurl->SetWindowText(g_index_url);

	// set dialog icon
	HICON hicon = LoadImage(GetModuleHandle(NULL), (LPCSTR)MAKEINTRESOURCE(IDICON200),
		IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE);
	SendMessage(dlg->hwnd, WM_SETICON, ICON_BIG, (LPARAM)hicon);

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
	GC_free(g_psi);
	return;
}
