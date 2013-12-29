/* See LICENSE file for copyright and license details. */
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if USE_XLIB
#include <X11/Xlib.h>
#endif
#include "draw.h"

#define MAX(a, b)  ((a) > (b) ? (a) : (b))
#define MIN(a, b)  ((a) < (b) ? (a) : (b))
#define DEFAULTFN  "fixed"

#if USE_XLIB
static Bool loadfont(DC *dc, const char *fontstr);
#elif USE_WINAPI
static BOOL loadfont(DC *dc, const char *fontstr);
#endif

void
#if USE_XLIB
drawrect(DC *dc, int x, int y, unsigned int w, unsigned int h, Bool fill, unsigned long color) {
	XSetForeground(dc->dpy, dc->gc, color);
#elif USE_WINAPI
drawrect(DC *dc, int x, int y, unsigned int w, unsigned int h, BOOL fill, unsigned long color) { /* bbggrr */
	SetDCBrushColor(dc->canvas, color); /* bbggrr */
#endif	
	if(fill)
#if USE_XLIB
		XFillRectangle(dc->dpy, dc->canvas, dc->gc, dc->x + x, dc->y + y, w, h);
#elif USE_WINAPI
	{
		RECT r;
		HBRUSH hbr = CreateSolidBrush(color); /* bbggrr */
		SetRect(&r, dc->x + x, dc->y + y, dc->x + x + w, dc->y + y + h);
		FillRect(dc->canvas, &r, hbr);
	}
#endif		
	else
#if USE_XLIB
		XDrawRectangle(dc->dpy, dc->canvas, dc->gc, dc->x + x, dc->y + y, w-1, h-1);
#elif USE_WINAPI
		Rectangle(dc->canvas, dc->x + x, dc->y + y, dc->x + x + w-1, dc->y + y + h-1);
#endif
}

void
drawtext(DC *dc, const char *text, unsigned long col[ColLast]) {
	char buf[BUFSIZ];
	size_t mn, n = strlen(text);

	/* shorten text if necessary */
	for(mn = MIN(n, sizeof buf); textnw(dc, text, mn) + dc->font.height/2 > dc->w; mn--)
		if(mn == 0)
			return;
	memcpy(buf, text, mn);
	if(mn < n)
		for(n = MAX(mn-3, 0); n < mn; buf[n++] = '.');

#if USE_XLIB
	drawrect(dc, 0, 0, dc->w, dc->h, True, BG(dc, col));
#elif USE_WINAPI
	drawrect(dc, 0, 0, dc->w, dc->h, TRUE, BG(dc, col));
#endif	
	drawtextn(dc, buf, mn, col);
}

void
drawtextn(DC *dc, const char *text, size_t n, unsigned long col[ColLast]) {
	int x = dc->x + dc->font.height/2;
	int y = dc->y + dc->font.ascent+1;

#if USE_XLIB
	XSetForeground(dc->dpy, dc->gc, FG(dc, col));
	if(dc->font.set)
		XmbDrawString(dc->dpy, dc->canvas, dc->font.set, dc->gc, x, y, text, n);
	else {
		XSetFont(dc->dpy, dc->gc, dc->font.xfont->fid);
		XDrawString(dc->dpy, dc->canvas, dc->gc, x, y, text, n);
	}
#elif USE_WINAPI
	SelectObject(dc->canvas, dc->font.font);
	SetTextColor(dc->canvas, FG(dc, col));
	SetBkColor(dc->canvas, BG(dc, col));
	TextOut(dc->canvas, x, 0, TEXT(text), n);
#endif	
}

void
eprintf(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if(fmt[0] != '\0' && fmt[strlen(fmt)-1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	}
	exit(EXIT_FAILURE);
}

void
freedc(DC *dc) {
#if USE_XLIB	
	if(dc->font.set)
		XFreeFontSet(dc->dpy, dc->font.set);
	if(dc->font.xfont)
		XFreeFont(dc->dpy, dc->font.xfont);
	if(dc->canvas)
		XFreePixmap(dc->dpy, dc->canvas);
	XFreeGC(dc->dpy, dc->gc);
	XCloseDisplay(dc->dpy);
#elif USE_WINAPI
	if (dc->font.font)
		DeleteObject(dc->font.font);
	if (dc->hbmp)
	{
		DeleteObject(dc->hbmp);
		DeleteDC(dc->canvas);
	}
#endif	
	free(dc);
}

unsigned long
getcolor(DC *dc, const char *colstr) {
#if USE_XLIB		
	Colormap cmap = DefaultColormap(dc->dpy, DefaultScreen(dc->dpy));
	XColor color;

	if(!XAllocNamedColor(dc->dpy, cmap, colstr, &color, &color))
		eprintf("cannot allocate color '%s'\n", colstr);
	return color.pixel;
#elif USE_WINAPI
	unsigned int r,g,b;
	sscanf(colstr, "#%2x%2x%2x", &r,&g,&b);
	return RGB((BYTE)r,(BYTE)g,(BYTE)b); /* bbggrr */
#endif	
}

DC *
initdc(void) {
	DC *dc;

#if USE_XLIB
	if(!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		fputs("no locale support\n", stderr);
	if(!(dc = calloc(1, sizeof *dc)))
		eprintf("cannot malloc %u bytes:", sizeof *dc);
	if(!(dc->dpy = XOpenDisplay(NULL)))
		eprintf("cannot open display\n");

	dc->gc = XCreateGC(dc->dpy, DefaultRootWindow(dc->dpy), 0, NULL);
	XSetLineAttributes(dc->dpy, dc->gc, 1, LineSolid, CapButt, JoinMiter);
	return dc;
#elif USE_WINAPI
	if(!setlocale(LC_CTYPE, ""))
		fputs("no locale support\n", stderr);
	if(!(dc = calloc(1, sizeof *dc)))
		eprintf("cannot malloc %u bytes:", sizeof *dc);
#endif	
}

void
initfont(DC *dc, const char *fontstr) {
	if(!loadfont(dc, fontstr ? fontstr : DEFAULTFN)) {
		if(fontstr != NULL)
			fprintf(stderr, "cannot load font '%s'\n", fontstr);
		if(fontstr == NULL || !loadfont(dc, DEFAULTFN))
			eprintf("cannot load font '%s'\n", DEFAULTFN);
	}
	dc->font.height = dc->font.ascent + dc->font.descent;
}

#if USE_XLIB
Bool
loadfont(DC *dc, const char *fontstr) {
#elif USE_WINAPI
BOOL
loadfont(DC *dc, const char *fontstr) {
#endif
#if USE_XLIB	
	char *def, **missing, **names;
	int i, n;
	XFontStruct **xfonts;

	if(!*fontstr)
		return False;
	if((dc->font.set = XCreateFontSet(dc->dpy, fontstr, &missing, &n, &def))) {
		n = XFontsOfFontSet(dc->font.set, &xfonts, &names);
		for(i = 0; i < n; i++) {
			dc->font.ascent  = MAX(dc->font.ascent,  xfonts[i]->ascent);
			dc->font.descent = MAX(dc->font.descent, xfonts[i]->descent);
			dc->font.width   = MAX(dc->font.width,   xfonts[i]->max_bounds.width);
		}
	}
	else if((dc->font.xfont = XLoadQueryFont(dc->dpy, fontstr))) {
		dc->font.ascent  = dc->font.xfont->ascent;
		dc->font.descent = dc->font.xfont->descent;
		dc->font.width   = dc->font.xfont->max_bounds.width;
	}
	if(missing)
		XFreeStringList(missing);
	return dc->font.set || dc->font.xfont;
#elif USE_WINAPI
	if ((dc->font.font = CreateFont(10,0,0,0,0,0,0,0,0,0,0,0,0,TEXT(fontstr))))
	{
		SelectObject(dc->gc, dc->font.font);
		GetTextMetrics(dc->gc, &dc->font.tm);
		dc->font.ascent = dc->font.tm.tmAscent;
		dc->font.descent = dc->font.tm.tmDescent;
		dc->font.width = dc->font.tm.tmMaxCharWidth;
		dc->font.height = dc->font.tm.tmHeight;
	}
	return dc->font.font;
#endif	
}

#if USE_XLIB
void
mapdc(DC *dc, Window win, unsigned int w, unsigned int h) {
	XCopyArea(dc->dpy, dc->canvas, win, dc->gc, 0, 0, w, h, 0, 0);
#elif USE_WINAPI
void
mapdc(DC *dc, HWND win, unsigned int w, unsigned int h) {
	BitBlt(dc->gc, 0, 0, w, h, dc->canvas, 0, 0, SRCCOPY);
#endif	
}

void
resizedc(DC *dc, unsigned int w, unsigned int h) {
#if USE_XLIB	
	if(dc->canvas)
		XFreePixmap(dc->dpy, dc->canvas);

	dc->w = w;
	dc->h = h;
	dc->canvas = XCreatePixmap(dc->dpy, DefaultRootWindow(dc->dpy), w, h,
	                           DefaultDepth(dc->dpy, DefaultScreen(dc->dpy)));
#elif USE_WINAPI
	if (dc->canvas)
	{
		DeleteObject(dc->hbmp);
		DeleteDC(dc->canvas);
	}
	dc->w = w;
	dc->h = h;
	dc->canvas = CreateCompatibleDC(dc->gc);
    dc->hbmp = CreateCompatibleBitmap(dc->gc, w, h);
    SelectObject(dc->canvas, dc->hbmp);
#endif	                           
}

int
textnw(DC *dc, const char *text, size_t len) {
#if USE_XLIB	
	if(dc->font.set) {
		XRectangle r;

		XmbTextExtents(dc->font.set, text, len, NULL, &r);
		return r.width;
	}
	return XTextWidth(dc->font.xfont, text, len);
#elif USE_WINAPI
	SIZE size;
	GetTextExtentPoint32(dc->canvas, text, len, &size);
	return size.cx;
#endif	
}

int
textw(DC *dc, const char *text) {
	return textnw(dc, text, strlen(text)) + dc->font.height;
}
