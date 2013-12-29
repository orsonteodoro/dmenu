/* See LICENSE file for copyright and license details. */
/* vim: expandtab
 */
/* Default settings; can be overrided by command line. */

#if USE_XLIB
static Bool topbar = True;                  /* -b  option; if False, dmenu appears at bottom */
#elif USE_WINAPI
static BOOL topbar = TRUE;                  /* -b  option; if False, dmenu appears at bottom */
#endif
static const char *font = NULL;             /* -fn option; default X11 font or font set      */
static const char *prompt = NULL;           /* -p  option; prompt to the elft of input field */
static const char *normbgcolor = "#222222"; /* -nb option; normal background                 */
static const char *normfgcolor = "#bbbbbb"; /* -nf option; normal foreground                 */
static const char *selbgcolor  = "#005577"; /* -sb option; selected background               */
static const char *selfgcolor  = "#eeeeee"; /* -sf option; selected foreground               */
static const char *outbgcolor  = "#00ffff";
static const char *outfgcolor  = "#000000";
/* -l option; if nonzero, dmenu uses vertical list with given number of lines */
static unsigned int lines = 0;
#if USE_WINAPI
#define normbordercolor 0x00cccccc /* 0x00bbggrr */
#define selbordercolor  0x00ff6600
#endif
