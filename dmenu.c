/* See LICENSE file for copyright and license details. */
#if WINDOWS
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT			0x0500

#include <windows.h>
#include <winuser.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#if USE_XLIB
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif
#endif
#if USE_WINAPI
#include <mshtmcid.h>
#endif
#include "draw.h"

#define INTERSECT(x,y,w,h,r)  (MAX(0, MIN((x)+(w),(r).x_org+(r).width)  - MAX((x),(r).x_org)) \
                             * MAX(0, MIN((y)+(h),(r).y_org+(r).height) - MAX((y),(r).y_org)))
#define MIN(a,b)              ((a) < (b) ? (a) : (b))
#define MAX(a,b)              ((a) > (b) ? (a) : (b))


/* portibility stuff */
#if USE_WINAPI
#define WINDOW HWND
#define Pixmap HDC
#define GC HDC
#define False FALSE
#define True TRUE
#define Bool BOOL
#define XK_a 'A'
#define XK_b 'B'
#define XK_c 'C'
#define XK_d 'D'
#define XK_e 'E'
#define XK_f 'F'
#define XK_g 'G'
#define XK_h 'H'
#define XK_i 'I'
#define XK_J 'J'
#define XK_m 'M'
#define XK_M 'M'
#define XK_n 'N'
#define XK_p 'P'
#define XK_k 'K'
#define XK_u 'U'
#define XK_w 'W'
#define XK_y 'Y'
#define XK_Return VK_RETURN
#define XK_KP_Enter VK_RETURN
#define XK_bracketleft VK_OEM_4
#define XK_G 'G'
#define XK_j 'J'
#define XK_k 'K'
#define XK_l 'L'
#define XK_Home VK_HOME
#define XK_Left VK_LEFT
#define XK_Escape VK_ESCAPE
#define XK_Delete VK_DELETE
#define XK_End VK_END
#define XK_Right VK_RIGHT
#define XK_Escape VK_ESCAPE
#define XK_BackSpace VK_BACK
#define XK_Tab VK_TAB
#define XK_Down VK_DOWN
#define XK_Up VK_UP
#define ShiftMask VK_SHIFT
#define XK_Prior VK_PRIOR
#define XK_Next VK_NEXT
#endif

typedef struct Item Item;
struct Item {
	char *text;
	Item *left, *right;
	Bool out;
};


static void appenditem(Item *item, Item **list, Item **last);
static void calcoffsets(void);
static char *cistrstr(const char *s, const char *sub);
static void drawmenu(void);
static void grabkeyboard(void);
static void insert(const char *str, ssize_t n);
#if USE_XLIB
static void keypress(XKeyEvent *ev);
#elif USE_WINAPI
static void keypress(WPARAM ksym);
#endif
static void match(void);
static size_t nextrune(int inc);
static void paste(void);
static void readstdin(void);
static void run(void);
#if USE_XLIB
static void setup(void);
#elif USE_WINAPI
static void setup(HINSTANCE hInstance, int nCmdShow);
static void die(const char *errstr, ...);
#endif
static void usage(void);

static char text[BUFSIZ] = "";
static int bh, mw, mh;
static int inputw, promptw;
static size_t cursor = 0;
static unsigned long normcol[ColLast];
static unsigned long selcol[ColLast];
static unsigned long outcol[ColLast];
#if USE_XLIB
static Atom clip, utf8;
#endif
static DC *dc;
static Item *items = NULL;
static Item *matches, *matchend;
static Item *prev, *curr, *next, *sel;
#if USE_XLIB
static Window win;
static XIC xic;
#elif USE_WINAPI
static HWND win;
static int sx, sy, sw, sh; /* X display screen geometry x, y, width, height */ 
static int by, bh;    /* bar geometry y, height and layout symbol width */
static int wx, wy, ww, wh; /* window area geometry x, y, width, height, bar excluded */
#endif
static int mon = -1;

#include "config.h"
#if USE_WINAPI
/* macros */
#define LENGTH(x)               (sizeof x / sizeof x[0])
/* elements of the window whose color should be set to the values in the array below */
static int colorwinelements[] = { COLOR_ACTIVEBORDER, COLOR_INACTIVEBORDER };
static COLORREF colors[2][LENGTH(colorwinelements)] = { 
	{ 0, 0 }, /* used to save the values before dwm started */
	{ selbordercolor, normbordercolor },
};
#define NAME					"dmenu" 	/* Used for window name/class */
static HWND dmhwnd;
#endif

static int (*fstrncmp)(const char *, const char *, size_t) = strncmp;
static char *(*fstrstr)(const char *, const char *) = strstr;

#if USE_XLIB
int
main(int argc, char *argv[]) {
#elif USE_WINAPI
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {	
#endif
	Bool fast = False;
	int i;

#if USE_WINAPI
	char **argv;
	int argc = 0, targc = 0;
	char buffer[512];
	char *t;

	for(i=0; i < strlen(lpCmdLine); i++)
		if (lpCmdLine[i] == ' ')
			targc++;
	targc+=2;
	argv = malloc(targc);
	argv[argc] = "dmenu.exe";
	argc++;
	t = strtok(lpCmdLine, " "); 
	targc--;
	buffer[0] = NULL;
	
	fflush(stdout); /*strange bug*/
	while(t != NULL && targc)
	{
		if (t[0] != '-')
		{
			do
			{
				strncat(buffer, t, 512);
				t = strtok(NULL, " ");
				if (t != NULL && t[0] != '-')
					strncat(buffer, " ", 512);
				targc--;
			} while (t != NULL && t[0] != '-' && targc);
			argv[argc]=strdup(buffer);
			argc++;
			buffer[0] = NULL;
		}
		else
		{
			argv[argc]=strdup(t);
			argc++;
			buffer[0] = NULL;
			t = strtok(NULL, " ");
			targc--;
		}
	}
#endif

	for(i = 1; i < argc; i++) {
		/* these options take no arguments */
		if(!strcmp(argv[i], "-v")) {      /* prints version information */
			puts("dmenu-"VERSION", © 2006-2012 dmenu engineers, see LICENSE for details");
			exit(EXIT_SUCCESS);
		}
		else if(!strcmp(argv[i], "-b"))   /* appears at the bottom of the screen */
			topbar = False;
		else if(!strcmp(argv[i], "-f"))   /* grabs keyboard before reading stdin */
			fast = True;
		else if(!strcmp(argv[i], "-i")) { /* case-insensitive item matching */
			fstrncmp = strncasecmp;
			fstrstr = cistrstr;
		}
		else if(i+1 == argc)
			usage();
		/* these options take one argument */
		else if(!strcmp(argv[i], "-l"))   /* number of lines in vertical list */
			lines = atoi(argv[++i]);
		else if(!strcmp(argv[i], "-m"))
			mon = atoi(argv[++i]);
		else if(!strcmp(argv[i], "-p"))   /* adds prompt to left of input field */
			prompt = argv[++i];
		else if(!strcmp(argv[i], "-fn"))  /* font or font set */
			font = argv[++i];
		else if(!strcmp(argv[i], "-nb"))  /* normal background color */
			normbgcolor = argv[++i];
		else if(!strcmp(argv[i], "-nf"))  /* normal foreground color */
			normfgcolor = argv[++i];
		else if(!strcmp(argv[i], "-sb"))  /* selected background color */
			selbgcolor = argv[++i];
		else if(!strcmp(argv[i], "-sf"))  /* selected foreground color */
			selfgcolor = argv[++i];
		else
			usage();
	}
	dc = initdc();
#if USE_XLIB
	initfont(dc, font);
#endif

	if(fast) {
		grabkeyboard();
		readstdin();
	}
	else {
		readstdin();
		grabkeyboard();
	}
#if USE_XLIB
	setup();
#elif USE_WINAPI
	setup(hInstance, nShowCmd);
#endif
	run();

	return 1; /* unreachable */
}

#if USE_WINAPI

void
cleanup() {
	ReleaseDC(dmhwnd,dc->gc);
	
	SetSysColors(LENGTH(colorwinelements), colorwinelements, colors[0]); 

	DestroyWindow(dmhwnd);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void
die(const char *errstr, ...) {
	va_list ap;

	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);
	cleanup();
	exit(EXIT_FAILURE);
}

void
updategeom(void) {
	RECT wa;
	HWND hwnd = FindWindow("Shell_TrayWnd", NULL);
	/* check if the windows taskbar is visible and adjust
	 * the workspace accordingly.
	 */
	if (hwnd && IsWindowVisible(hwnd)) {	
		SystemParametersInfo(SPI_GETWORKAREA, 0, &wa, 0);
		sx = wa.left;
		sy = wa.top;
		sw = wa.right - wa.left;
		sh = wa.bottom - wa.top;
	} else {
		sx = GetSystemMetrics(SM_XVIRTUALSCREEN);
		sy = GetSystemMetrics(SM_YVIRTUALSCREEN);
		sw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
		sh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	}

	bh = 20; /* XXX: fixed value */

	/* window area geometry */
	wx = sx;
	
	/* wy = showbar && topbar ? sy + bh : sy; */
	wy = sy + bh;
	ww = sw;
	
	/* wh = showbar ? sh - bh : sh; */
	wh = sh - bh;
	
	/* bar position */
	/* by = showbar ? (topbar ? wy - bh : wy + wh) : -bh; */
	by = wy - bh;
	/* debug("updategeom: %d x %d\n", ww, wh); */
}
#endif

void
appenditem(Item *item, Item **list, Item **last) {
	if(*last)
		(*last)->right = item;
	else
		*list = item;

	item->left = *last;
	item->right = NULL;
	*last = item;
}

void
calcoffsets(void) {
	int i, n;

	if(lines > 0)
		n = lines * bh;
	else
		n = mw - (promptw + inputw + textw(dc, "<") + textw(dc, ">"));
	/* calculate which items will begin the next page and previous page */
	for(i = 0, next = curr; next; next = next->right)
		if((i += (lines > 0) ? bh : MIN(textw(dc, next->text), n)) > n)
			break;
	for(i = 0, prev = curr; prev && prev->left; prev = prev->left)
		if((i += (lines > 0) ? bh : MIN(textw(dc, prev->left->text), n)) > n)
			break;
}

char *
cistrstr(const char *s, const char *sub) {
	size_t len;

	for(len = strlen(sub); *s; s++)
		if(!strncasecmp(s, sub, len))
			return (char *)s;
	return NULL;
}

void
drawmenu(void) {
	int curpos;
	Item *item;

	dc->x = 0;
	dc->y = 0;
	dc->h = bh;
	drawrect(dc, 0, 0, mw, mh, True, BG(dc, normcol));

	if(prompt && *prompt) {
		dc->w = promptw;
		drawtext(dc, prompt, selcol);
		dc->x = dc->w;
	}
	/* draw input field */
	dc->w = (lines > 0 || !matches) ? mw - dc->x : inputw;
	drawtext(dc, text, normcol);
	if((curpos = textnw(dc, text, cursor) + dc->h/2 - 2) < dc->w)
		drawrect(dc, curpos, 2, 1, dc->h - 4, True, FG(dc, normcol));

	if(lines > 0) {
		/* draw vertical list */
		dc->w = mw - dc->x;
		for(item = curr; item != next; item = item->right) {
			dc->y += dc->h;
			drawtext(dc, item->text, (item == sel) ? selcol :
			                         (item->out)   ? outcol : normcol);
		}
	}
	else if(matches) {
		/* draw horizontal list */
		dc->x += inputw;
		dc->w = textw(dc, "<");
		if(curr->left)
			drawtext(dc, "<", normcol);
		for(item = curr; item != next; item = item->right) {
			dc->x += dc->w;
			dc->w = MIN(textw(dc, item->text), mw - dc->x - textw(dc, ">"));
			drawtext(dc, item->text, (item == sel) ? selcol :
			                         (item->out)   ? outcol : normcol);
		}
		dc->w = textw(dc, ">");
		dc->x = mw - dc->w;
		if(next)
			drawtext(dc, ">", normcol);
	}
	mapdc(dc, win, mw, mh);
}

void
grabkeyboard(void) {
	int i;

	/* try to grab keyboard, we may have to wait for another process to ungrab */
	for(i = 0; i < 1000; i++) {
#if USE_XLIB		
		if(XGrabKeyboard(dc->dpy, DefaultRootWindow(dc->dpy), True,
		                 GrabModeAsync, GrabModeAsync, CurrentTime) == GrabSuccess)
			return;
#elif USE_WINAPI
		return;
#endif	
		usleep(1000);
	}
	eprintf("cannot grab keyboard\n");
}

void
insert(const char *str, ssize_t n) {
	if(strlen(text) + n > sizeof text - 1)
		return;
	/* move existing text out of the way, insert new text, and update cursor */
	memmove(&text[cursor + n], &text[cursor], sizeof text - cursor - MAX(n, 0));
	if(n > 0)
		memcpy(&text[cursor], str, n);
	cursor += n;
	match();
}

#if USE_XLIB
void
keypress(XKeyEvent *ev) {
#elif USE_WINAPI
void
keypress(WPARAM ksym) {
	char c;
#endif
	char buf[32];
	int len;
	
#if USE_XLIB
	KeySym ksym = NoSymbol;
	Status status;

	len = XmbLookupString(xic, ev, buf, sizeof buf, &ksym, &status);
	if(status == XBufferOverflow)
		return;
#elif USE_WINAPI
	c = MapVirtualKey(ksym,MAPVK_VK_TO_CHAR);
	if (GetAsyncKeyState(VK_SHIFT) || (GetKeyState(VK_CAPITAL) & 0x0001) != 0)
		sprintf(buf,"%c",c);
	else
		sprintf(buf,"%c",tolower(c));
	len = 1;
#endif

#if USE_XLIB
	if(ev->state & ControlMask)
#elif USE_WINAPI
	if(GetAsyncKeyState(VK_CONTROL))
#endif
		switch(ksym) {
		case XK_a: ksym = XK_Home;      break;
		case XK_b: ksym = XK_Left;      break;
		case XK_c: ksym = XK_Escape;    break;
		case XK_d: ksym = XK_Delete;    break;
		case XK_e: ksym = XK_End;       break;
		case XK_f: ksym = XK_Right;     break;
		case XK_g: ksym = XK_Escape;    break;
		case XK_h: ksym = XK_BackSpace; break;
		case XK_i: ksym = XK_Tab;       break;
#if USE_XLIB
		case XK_j: /* fallthrough */
#endif
		case XK_J: ksym = XK_Return;    break;
#if USE_XLIB
		case XK_m: /* fallthrough */
#endif
		case XK_M: ksym = XK_Return;    break;
		case XK_n: ksym = XK_Down;      break;
		case XK_p: ksym = XK_Up;        break;

		case XK_k: /* delete right */
			text[cursor] = '\0';
			match();
			break;
		case XK_u: /* delete left */
			insert(NULL, 0 - cursor);
			break;
		case XK_w: /* delete word */
			while(cursor > 0 && text[nextrune(-1)] == ' ')
				insert(NULL, nextrune(-1) - cursor);
			while(cursor > 0 && text[nextrune(-1)] != ' ')
				insert(NULL, nextrune(-1) - cursor);
			break;
		case XK_y: /* paste selection */
#if USE_XLIB
			XConvertSelection(dc->dpy, (ev->state & ShiftMask) ? clip : XA_PRIMARY,
			                  utf8, utf8, win, CurrentTime);
#endif
			return;
		case XK_Return:
#if USE_XLIB
		case XK_KP_Enter:
#endif
			break;
		case XK_bracketleft:
			exit(EXIT_FAILURE);
		default:
			return;
		}
#if USE_XLIB
	else if(ev->state & Mod1Mask)
#elif USE_WINAPI
	else if(GetAsyncKeyState(VK_MENU))
#endif
		switch(ksym) {
#if USE_WINAPI
		case XK_g: 
			if (GetAsyncKeyState(VK_SHIFT) || (GetKeyState(VK_CAPITAL) & 0x0001) != 0)
				ksym = VK_END; /*XK_G*/
			else
				ksym = VK_HOME;  /*XK_g*/
		break;
#elif USE_XLIB
		case XK_g: ksym = XK_Home;  break;
		case XK_G: ksym = XK_End;   break;
#endif
		case XK_h: ksym = XK_Up;    break;
		case XK_j: ksym = XK_Next;  break;
		case XK_k: ksym = XK_Prior; break;
		case XK_l: ksym = XK_Down;  break;
		default:
			return;
		}
	switch(ksym) {
	default:
		if(!iscntrl(*buf))
			insert(buf, len);
		break;
	case XK_Delete:
		if(text[cursor] == '\0')
			return;
		cursor = nextrune(+1);
		/* fallthrough */
	case XK_BackSpace:
		if(cursor == 0)
			return;
		insert(NULL, nextrune(-1) - cursor);
		break;
	case XK_End:
		if(text[cursor] != '\0') {
			cursor = strlen(text);
			break;
		}
		if(next) {
			/* jump to end of list and position items in reverse */
			curr = matchend;
			calcoffsets();
			curr = prev;
			calcoffsets();
			while(next && (curr = curr->right))
				calcoffsets();
		}
		sel = matchend;
		break;
	case XK_Escape:
		exit(EXIT_FAILURE);
	case XK_Home:
		if(sel == matches) {
			cursor = 0;
			break;
		}
		sel = curr = matches;
		calcoffsets();
		break;
	case XK_Left:
		if(cursor > 0 && (!sel || !sel->left || lines > 0)) {
			cursor = nextrune(-1);
			break;
		}
		if(lines > 0)
			return;
		/* fallthrough */
	case XK_Up:
		if(sel && sel->left && (sel = sel->left)->right == curr) {
			curr = prev;
			calcoffsets();
		}
		break;
	case XK_Next:
		if(!next)
			return;
		sel = curr = next;
		calcoffsets();
		break;
	case XK_Prior:
		if(!prev)
			return;
		sel = curr = prev;
		calcoffsets();
		break;
	case XK_Return:
#if USE_XLIB
	case XK_KP_Enter:
#endif
#if USE_XLIB	
		puts((sel && !(ev->state & ShiftMask)) ? sel->text : text);
#elif USE_WINAPI
		puts((sel && !(GetAsyncKeyState(VK_SHIFT))) ? sel->text : text);
#endif
#if USE_XLIB
		if(!(ev->state & ControlMask))
#elif USE_WINAPI
		if(!(GetAsyncKeyState(VK_CONTROL)))
#endif
			exit(EXIT_SUCCESS);
		sel->out = True;
		break;
	case XK_Right:
		if(text[cursor] != '\0') {
			cursor = nextrune(+1);
			break;
		}
		if(lines > 0)
			return;
		/* fallthrough */
	case XK_Down:
		if(sel && sel->right && (sel = sel->right) == next) {
			curr = next;
			calcoffsets();
		}
		break;
	case XK_Tab:
		if(!sel)
			return;
		strncpy(text, sel->text, sizeof text - 1);
		text[sizeof text - 1] = '\0';
		cursor = strlen(text);
		match();
		break;
	}
	drawmenu();
}

void
match(void) {
	static char **tokv = NULL;
	static int tokn = 0;

	char buf[sizeof text], *s;
	int i, tokc = 0;
	size_t len;
	Item *item, *lprefix, *lsubstr, *prefixend, *substrend;

	strcpy(buf, text);
	/* separate input text into tokens to be matched individually */
	for(s = strtok(buf, " "); s; tokv[tokc-1] = s, s = strtok(NULL, " "))
		if(++tokc > tokn && !(tokv = realloc(tokv, ++tokn * sizeof *tokv)))
			eprintf("cannot realloc %u bytes\n", tokn * sizeof *tokv);
	len = tokc ? strlen(tokv[0]) : 0;

	matches = lprefix = lsubstr = matchend = prefixend = substrend = NULL;
	for(item = items; item && item->text; item++) {
		for(i = 0; i < tokc; i++)
			if(!fstrstr(item->text, tokv[i]))
				break;
		if(i != tokc) /* not all tokens match */
			continue;
		/* exact matches go first, then prefixes, then substrings */
		if(!tokc || !fstrncmp(tokv[0], item->text, len+1))
			appenditem(item, &matches, &matchend);
		else if(!fstrncmp(tokv[0], item->text, len))
			appenditem(item, &lprefix, &prefixend);
		else
			appenditem(item, &lsubstr, &substrend);
	}
	if(lprefix) {
		if(matches) {
			matchend->right = lprefix;
			lprefix->left = matchend;
		}
		else
			matches = lprefix;
		matchend = prefixend;
	}
	if(lsubstr) {
		if(matches) {
			matchend->right = lsubstr;
			lsubstr->left = matchend;
		}
		else
			matches = lsubstr;
		matchend = substrend;
	}
	curr = sel = matches;
	calcoffsets();
}

size_t
nextrune(int inc) {
	ssize_t n;

	/* return location of next utf8 rune in the given direction (+1 or -1) */
	for(n = cursor + inc; n + inc >= 0 && (text[n] & 0xc0) == 0x80; n += inc);
	return n;
}

void
paste(void) {
	char *p, *q;
#if USE_XLIB	
	int di;
	unsigned long dl;
	Atom da;

	/* we have been given the current selection, now insert it into input */
	XGetWindowProperty(dc->dpy, win, utf8, 0, (sizeof text / 4) + 1, False,
	                   utf8, &da, &di, &dl, &dl, (unsigned char **)&p);
	insert(p, (q = strchr(p, '\n')) ? q-p : (ssize_t)strlen(p));
	XFree(p);
#elif USE_WINAPI
	HGLOBAL h;
	if (!OpenClipboard(dc->hwnd))
		return;
	if ((h = GetClipboardData(CF_TEXT)))
	{
		p = GlobalLock(h);
		if (p != NULL)
		{
			insert(p, (q = strchr(p, '\n')) ? q-p : (ssize_t)strlen(p));
			GlobalUnlock(h);
		}
	}
	CloseClipboard();
#endif	
	drawmenu();
}

void
readstdin(void) {
	char buf[sizeof text], *p, *maxstr = NULL;
	size_t i, max = 0, size = 0;

	/* read each line from stdin and add it to the item list */
	for(i = 0; fgets(buf, sizeof buf, stdin); i++) {
		if(i+1 >= size / sizeof *items)
			if(!(items = realloc(items, (size += BUFSIZ))))
				eprintf("cannot realloc %u bytes:", size);
		if((p = strchr(buf, '\n')))
			*p = '\0';
		if(!(items[i].text = strdup(buf)))
			eprintf("cannot strdup %u bytes:", strlen(buf)+1);
		items[i].out = False;
		if(strlen(items[i].text) > max)
			max = strlen(maxstr = items[i].text);
	}
	if(items)
		items[i].text = NULL;
	inputw = maxstr ? textw(dc, maxstr) : 0;
	lines = MIN(lines, i);
}

void
run(void) {
#if USE_XLIB
	XEvent ev;

	while(!XNextEvent(dc->dpy, &ev)) {
		if(XFilterEvent(&ev, win))
			continue;
		switch(ev.type) {
		case Expose:
			if(ev.xexpose.count == 0)
				mapdc(dc, win, mw, mh);
			break;
		case KeyPress:
			keypress(&ev.xkey);
			break;
		case SelectionNotify:
			if(ev.xselection.property == utf8)
				paste();
			break;
		case VisibilityNotify:
			if(ev.xvisibility.state != VisibilityUnobscured)
				XRaiseWindow(dc->dpy, win);
			break;
		}
	}
#elif USE_WINAPI
	MSG msg;
	
	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
#endif	
}

#if USE_WINAPI
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_CREATE:
		{
			RECT r;
			GetClientRect(hwnd, &r);
			dc->hcanvas = CreateWindowEx(WS_EX_APPWINDOW, "STATIC", "",
			WS_CHILD|WS_VISIBLE,
			0, 0, 0, 0, 
			hwnd, 0, GetModuleHandle(NULL),
			NULL);
			dc->gc = GetDC(dc->hcanvas);
		}
			break;
		case WM_CLOSE:
			/* cleanup(); */
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		case WM_PAINT:
			if(dc->canvas)
				mapdc(dc, win, mw, mh);
			break;
		case WM_KEYDOWN:
			keypress(wParam);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)) 
			{ 
				case IDM_PASTE:
					if (IsClipboardFormatAvailable(CF_TEXT))
						paste();
				break;
			}
			break;
		default:
			return DefWindowProc(hwnd, msg, wParam, lParam); 
	}

	return 0;
}
#endif

#if USE_XLIB
void
setup(void) {
	int x, y, screen = DefaultScreen(dc->dpy);
	Window root = RootWindow(dc->dpy, screen);
	XSetWindowAttributes swa;
	XIM xim;
#ifdef XINERAMA
	int n;
	XineramaScreenInfo *info;
#endif
#elif USE_WINAPI
void
setup(HINSTANCE hInstance, int nCmdShow) {
	int i;
	RECT r;
#endif

	normcol[ColBG] = getcolor(dc, normbgcolor);
	normcol[ColFG] = getcolor(dc, normfgcolor);
	selcol[ColBG]  = getcolor(dc, selbgcolor);
	selcol[ColFG]  = getcolor(dc, selfgcolor);
	outcol[ColBG]  = getcolor(dc, outbgcolor);
	outcol[ColFG]  = getcolor(dc, outfgcolor);

#if USE_XLIB
	clip = XInternAtom(dc->dpy, "CLIPBOARD",   False);
	utf8 = XInternAtom(dc->dpy, "UTF8_STRING", False);
#endif

	/* calculate menu geometry */
	bh = dc->font.height + 2;
	lines = MAX(lines, 0);
	mh = (lines + 1) * bh;
#if USE_XLIB
#ifdef XINERAMA
	if((info = XineramaQueryScreens(dc->dpy, &n))) {
		int a, j, di, i = 0, area = 0;
		unsigned int du;
		Window w, pw, dw, *dws;
		XWindowAttributes wa;

		XGetInputFocus(dc->dpy, &w, &di);
		if(mon != -1 && mon < n)
			i = mon;
		if(!i && w != root && w != PointerRoot && w != None) {
			/* find top-level window containing current input focus */
			do {
				if(XQueryTree(dc->dpy, (pw = w), &dw, &w, &dws, &du) && dws)
					XFree(dws);
			} while(w != root && w != pw);
			/* find xinerama screen with which the window intersects most */
			if(XGetWindowAttributes(dc->dpy, pw, &wa))
				for(j = 0; j < n; j++)
					if((a = INTERSECT(wa.x, wa.y, wa.width, wa.height, info[j])) > area) {
						area = a;
						i = j;
					}
		}
		/* no focused window is on screen, so use pointer location instead */
		if(mon == -1 && !area && XQueryPointer(dc->dpy, root, &dw, &dw, &x, &y, &di, &di, &du))
			for(i = 0; i < n; i++)
				if(INTERSECT(x, y, 1, 1, info[i]))
					break;

		x = info[i].x_org;
		y = info[i].y_org + (topbar ? 0 : info[i].height - mh);
		mw = info[i].width;
		XFree(info);
	}
	else
#endif
	{
		x = 0;
		y = topbar ? 0 : DisplayHeight(dc->dpy, screen) - mh;
		mw = DisplayWidth(dc->dpy, screen);
	}
#endif
	/* see note1 */
	promptw = (prompt && *prompt) ? textw(dc, prompt) : 0;
	inputw = MIN(inputw, mw/3);
	match();

#if USE_XLIB
	/* create menu window */
	swa.override_redirect = True;
	swa.background_pixel = normcol[ColBG];
	swa.event_mask = ExposureMask | KeyPressMask | VisibilityChangeMask;
	win = XCreateWindow(dc->dpy, root, x, y, mw, mh, 0,
	                    DefaultDepth(dc->dpy, screen), CopyFromParent,
	                    DefaultVisual(dc->dpy, screen),
	                    CWOverrideRedirect | CWBackPixel | CWEventMask, &swa);

	/* open input methods */
	xim = XOpenIM(dc->dpy, NULL, NULL, NULL);
	xic = XCreateIC(xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
	                XNClientWindow, win, XNFocusWindow, win, NULL);

	XMapRaised(dc->dpy, win);
#elif USE_WINAPI
	/* save colors so we can restore them in cleanup */
	for (i = 0; i < LENGTH(colorwinelements); i++)
		colors[0][i] = GetSysColor(colorwinelements[i]);
	
	SetSysColors(LENGTH(colorwinelements), colorwinelements, colors[1]); 

	updategeom();

	/* message window */

	WNDCLASSEX winClass;

	winClass.cbSize = sizeof(WNDCLASSEX);
	winClass.style = 0;
	winClass.lpfnWndProc = WndProc;
	winClass.cbClsExtra = 0;
	winClass.cbWndExtra = 0;
	winClass.hInstance = hInstance;
	winClass.hIcon = NULL;
	winClass.hIconSm = NULL;
	winClass.hCursor = NULL;
	winClass.hbrBackground = CreateSolidBrush(BG(dc, normcol));
	winClass.lpszMenuName = NULL;
	winClass.lpszClassName = NAME;

	if (!RegisterClassEx(&winClass))
		die("Error registering window class");

	dmhwnd = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, NAME, NAME, 
		WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 
		0, 0, 0, 0, 0, NULL, hInstance, NULL);

	if (!dmhwnd)
		die("Error creating window");

	SetFocus(dmhwnd);

	ShowWindow(dmhwnd, nCmdShow);
    UpdateWindow(dmhwnd);

	dc->hwnd = dmhwnd;
	
	initfont(dc, font);
	bh = dc->font.height + 2;
	lines = MAX(lines, 0);
	mh = (lines + 1) * bh;
	SetWindowPos(dmhwnd, 0, 0, by, ww, bh, 0);
	
	GetClientRect(dmhwnd, &r);
	SetWindowPos(dc->hcanvas, 0, 0, 0, r.right, r.bottom, 0);

	/* note1 */
	{
		RECT rect;
		GetWindowRect(dmhwnd, &rect);
		/* x = rect.left; */
		/* y = rect.top; */
		mw = rect.right - rect.left;
	}
#endif
	resizedc(dc, mw, mh);
	drawmenu();
}

void
usage(void) {
	fputs("usage: dmenu [-b] [-f] [-i] [-l lines] [-p prompt] [-fn font] [-m monitor]\n"
	      "             [-nb color] [-nf color] [-sb color] [-sf color] [-v]\n", stderr);
	exit(EXIT_FAILURE);
}
