/* See LICENSE file for copyright and license details. */

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT			0x0500

#if USE_WINAPI
#include <windows.h>
#include <winuser.h>
#endif

#if USE_XLIB
#define DEFAULTFN  "fixed"
#elif USE_WINAPI
#define DEFAULTFN  "Small Fonts"
#endif

#define FG(dc, col)  ((col)[(dc)->invert ? ColBG : ColFG])
#define BG(dc, col)  ((col)[(dc)->invert ? ColFG : ColBG])

enum { ColBG, ColFG, ColBorder, ColLast };

typedef struct {
	int x, y, w, h;
#if USE_XLIB	
	Bool invert;
	Display *dpy;
	GC gc;
	Pixmap canvas;
#elif USE_WINAPI
	HWND hwnd;
	HWND hedit;
	HWND hcanvas;
	HDC gc; 
	BOOL invert;
	HDC canvas;
	HBITMAP hbmp;
	unsigned long norm[ColLast];
	unsigned long sel[ColLast];
#endif	
	struct {
		int ascent;
		int descent;
		int height;
		int width;
#if USE_XLIB		
		XFontSet set;
		XFontStruct *xfont;
#elif USE_WINAPI
		HFONT font;
		TEXTMETRIC tm;
#endif		
	} font;
} DC;  /* draw context */

#if USE_XLIB
void drawrect(DC *dc, int x, int y, unsigned int w, unsigned int h, Bool fill, unsigned long color);
#elif USE_WINAPI
void drawrect(DC *dc, int x, int y, unsigned int w, unsigned int h, BOOL fill, unsigned long color);
#endif
void drawtext(DC *dc, const char *text, unsigned long col[ColLast]);
void drawtextn(DC *dc, const char *text, size_t n, unsigned long col[ColLast]);
void eprintf(const char *fmt, ...);
void freedc(DC *dc);
unsigned long getcolor(DC *dc, const char *colstr);
DC *initdc(void);
void initfont(DC *dc, const char *fontstr);
#if USE_XLIB
void mapdc(DC *dc, Window win, unsigned int w, unsigned int h);
#elif USE_WINAPI
void mapdc(DC *dc, HWND win, unsigned int w, unsigned int h);
#endif
void resizedc(DC *dc, unsigned int w, unsigned int h);
int textnw(DC *dc, const char *text, size_t len);
int textw(DC *dc, const char *text);
