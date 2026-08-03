/*************************************************************
 * listfonts.c : list fonts (like a poor xlsfonts)
 *************************************************************/

#define DEFAULT_FONT_NAME     "*"

//#include <X11/Xlib.h>
#include <X11/Intrinsic.h> 

#include <stdio.h>
#include <limits.h>

int main(int argc, char **argv) {
	Widget toplevel;
	int i;
	char *base_font_name = DEFAULT_FONT_NAME;
	int available;

	/*
     * Initialize the Intrinsics, only to get a display
     */
	toplevel = XtInitialize(argv[0], "listfonts", NULL, 0, &argc, argv);
	Display *display = XtDisplay(toplevel);

	/* there is also a XListFontsWithInfo() */
	char **fonts = XListFonts(display, base_font_name, INT_MAX, &available);
	for (i=0; i<available; i++) {
		printf("%i	%s\n", i, fonts[i]);
	}
}
