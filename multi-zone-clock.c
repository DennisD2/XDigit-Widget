/*************************************************************
 * multi-zone-clock.c : multi-zone digital clock
 *************************************************************/

#define MAX_CLOCKS 10

#define TIME_LOCAL 0
#define TIME_GMT 1
#define TIME_NEW_YORK 2

#include <X11/Xlib.h>
#include <X11/Intrinsic.h> 
#include <X11/Composite.h>

#include <Xm/Xm.h>
#include <Xm/Label.h>

#include "Digit.h"

#include <time.h>
#define TIMEOUT 10000L
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

// 5 digits per clock
typedef struct _ClockDigitsStruct {
	Widget digit[5];
} ClockDigitsStruct;

// all clocks, additional number of clocks
typedef struct _DigitStruct {
	int numClocks;
	ClockDigitsStruct *clockDigits;
} ClocksStruct;

ClocksStruct clocksStruct;

/*
 * Set all widgets value resouce to digit values defined by values array
 */
void setClocksValue(Widget digit[], int values[4]) {
	Arg args[1];

	printf("%d %d %d %d\n", values[0], values[1], values[2], values[3]);
	XtSetArg(args[0], XtNvalue, values[0]);
	XtSetValues( digit[0], args, 1 );
	XtSetArg(args[0], XtNvalue, values[1]);
	XtSetValues( digit[1], args, 1 );
	XtSetArg( args[0], XtNvalue, values[2]);
	XtSetValues( digit[3], args, 1 );
	XtSetArg( args[0], XtNvalue, values[3]);
	XtSetValues( digit[4], args, 1 );
}

/*
 * Get time and convert to values suitable for the Digit widgets.
 * Can return local time of GMT time, depending on time_zone value.
 */
void getCurrentTime(int num_values[4], int time_zone) {
	char *p, values[4][2];
	int i;
	time_t t;

	for (i=0;i<4;i++) {
		values[i][0] = '0'; values[i][1]='\0';
	}

	time( &t );
	struct tm * tt;
	if (time_zone == TIME_LOCAL) {
		tt = localtime(&t);
	} else {
		tt = gmtime(&t);
	}
	char *buf = asctime(tt);
	//printf("time: %s\n", buf);

	p=buf;
	while (!isdigit(*p)) p++;
	while (isdigit(*p)) p++;
	while (!isdigit(*p)) p++;
	values[0][0] = *p ; p++;
	values[1][0] = *p ; p++; p++;
	values[2][0] = *p ; p++;
	values[3][0] = *p ;
	for ( i=0; i<4; i++ ) {
		num_values[i] = atoi(values[i]);
	}
}

/*
 * Remove/add some integer offset to a 24 hour value in v[] array
 */
void change_hours(int * v, int i) {
	int h = v[0]*10 + v[1];
	h += i % 24;
	v[0] = h/10;
	v[1] = h%10;
}

/*
 * Timeout callback
 */
void TimeoutCB( XtPointer client_data, XtIntervalId* id ) {
	ClocksStruct *clockStruct  = (ClocksStruct *)client_data;
	int num_values[4];

	int i;
	for (i=0; i<clockStruct->numClocks; i++) {
		getCurrentTime((int*)num_values, TIME_LOCAL);
		setClocksValue(clockStruct->clockDigits[i].digit, num_values);
	}
	/*
	getCurrentTime((int*)num_values, TIME_LOCAL);
	setClocksValue(digit, num_values);

	getCurrentTime((int*)num_values, TIME_GMT);
	setClocksValue(&digit[5], num_values);

	getCurrentTime((int*)num_values, TIME_GMT);
	change_hours( num_values, -4);
	setClocksValue(&digit[10], num_values);
	*/

	/*
	 * start time out from the beginning 
	 */
	XtAddTimeOut( TIMEOUT, TimeoutCB, clockStruct );
}

void createClockWidgets(Widget compo, ClockDigitsStruct *clockDigits, int row) {
	int n,i;
	Arg args[8];
	for ( i=0; i<5; i++ ) {
		n=0;
		XtSetArg( args[n], XtNx, (Position)i*60 ); n++;
		XtSetArg( args[n], XtNy, (Position)row*100 ); n++;
		XtSetArg( args[n], XtNwidth, (Dimension)60 ); n++;
		if (i==2 )
			XtSetArg( args[n], XtNvalue, DOUBLEPOINT_VALUE );
		else
			XtSetArg( args[n], XtNvalue, i );
		n++;
		clockDigits->digit[i] = XtCreateManagedWidget("digit", XddigitWidgetClass,
		                                    compo, args, n);
	}
}

XmFontList the_font_list;

typedef struct _Resources {
	Pixel foreground;
	Pixel background;
	String labels;
	/*XFontStruct *fontStruct; */
} Resources;

static Resources theResources;

static XtResource resourceSpec[] = {
	{ XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
	  XtOffsetOf(Resources, foreground),
	  XtRString, "XtDefaultForeground"},
	{ XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel),
	  XtOffsetOf(Resources, background),
	  XtRString, "XtDefaultBackground"},
	{ "labels", "Labels", XtRString, sizeof(String),
	XtOffsetOf(Resources, labels),
	XtRString, "local"},
	/*{ XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct *),
		XtOffsetOf(Resources, fontStruct),
		XtRString, "XtDefaultFont"},*/
};

void createClockLabel(Widget compo, int numClock, char* title) {
	Arg wargs[7];
	int n=0;

	XmString xmstr = XmStringCreate(title, XmSTRING_DEFAULT_CHARSET);
	XtSetArg( wargs[n], XmNlabelString, xmstr ); n++;
	XtSetArg( wargs[n], XtNx, (Position)10 + 5*60 + 5); n++;
	XtSetArg( wargs[n], XtNy, (Position)numClock*100 + 100/2 ); n++;
	XtSetArg( wargs[n], XmNfontList, the_font_list ); n++;
	XtCreateManagedWidget("clockTitle", xmLabelWidgetClass, compo, wargs, n);
}

/**
 *
 * @param labelString string like "Frankfurt,GMT,New York"
 * @param labels Array of strings created from labelString by splitting at delimiter ','
 * @return number of strings read
 */
int readClockLabels(String labelString, String *labels) {
	//printf("%s\n", theResources.labels );
	int i=0;
	String token = strtok(theResources.labels,",");
	labels[i] = token;
	while (token != NULL) {
		labels[i++] = token;
		token = strtok(NULL, ",");
	}
	//int n=i;
	//for (i=0; i<n; i++) {
	//	printf("%s\n", labels[i] );
	//}
	return i;
}

int main(int argc, char **argv) {
    Widget toplevel, compo;
    Arg args[8]; int n, i;

    /*
     * Initialize the Intrinsics.
     */   
    toplevel = XtInitialize(argv[0], "MultiZoneClock", NULL,
                            0, &argc, argv);
	/* get apps resources for use in createClockLabel() */
	XtGetApplicationResources(toplevel, &theResources,
						   resourceSpec, XtNumber(resourceSpec), NULL, 0);

	String labels[MAX_CLOCKS];
	int numClocks = readClockLabels(theResources.labels, labels);
	if (numClocks > MAX_CLOCKS) {
		printf("Too many clocks defined!\n");
	} else {
		printf("%d clocks defined\n", numClocks);
	}

	Display *display = XtDisplay(toplevel);
/*
    For pure (non-Motif) we would do it like this:

    #define DEFAULT_FONT_NAME     "*-*-*-*-*-*--*-*, -*"
    char *base_font_name = DEFAULT_FONT_NAME;
	char **missing_list;
	int missing_count;
	char *def_string;
	XFontSet font_set = XCreateFontSet(display, base_font_name, &missing_list,
							   &missing_count, &def_string);
	if (missing_count > 0) {
		fprintf(stderr, "The following charsets are missing: \n");
		for (i=0; i<missing_count; i++)
			fprintf(stderr, "%s \n", missing_list[i]);
		XFreeStringList(missing_list);
	}
    XFontStruct **fonts;
    char **names;
	int num_fonts = XFontsOfFontSet( font_set, &fonts, &names);
	for (i=0; i<num_fonts; i++) {
		printf("Font[%d]: %s\n", i, names[i]);
	}

	Then we have a fontset and a font and may use that in some non-Motif widgets.
	Motif uses an own ressource named XmNfontList, so the solution looks different, see below
	*/

	/* Solution for Motif */
#define SOMEFONT "-adobe-courier-bold-r-normal--24-240-75-75-m-150-iso8859-1"
	char *someFont = SOMEFONT;
	/* load font */
	XFontStruct *font_info = XLoadQueryFont(display, someFont);
	if (font_info == NULL) {
		printf("No fonts\n");
	}
	/* create a motif font list and store in global var for later use */
	XmFontList fontList = XmFontListCreate(font_info, XmFONTLIST_DEFAULT_TAG);
	the_font_list = fontList;

    /*
     * Create a container widget for all the digits
     */
    n=0;
    XtSetArg( args[n], XtNwidth, (Dimension)500 ); n++;
    XtSetArg( args[n], XtNheight, (Dimension)numClocks*100 ); n++;
    compo = XtCreateManagedWidget("clockPanel", compositeWidgetClass,
	toplevel, args, n);

	/*
     * Create all digit widgets
     */
	ClockDigitsStruct *clockDigits = malloc (sizeof(ClockDigitsStruct)*numClocks);
	for ( i=0; i<numClocks; i++ ) {
		createClockLabel(compo,i, labels[i]);
		createClockWidgets(compo, &clockDigits[i], i);
	}
    clocksStruct.numClocks = numClocks;
	clocksStruct.clockDigits = malloc(sizeof(ClockDigitsStruct)*numClocks);
    for (i=0; i<numClocks; i++) {
		clocksStruct.clockDigits[i] = clockDigits[i];
	}

    XtRealizeWidget(toplevel);

    /* init clock display */
	TimeoutCB( (XtPointer)&clocksStruct, NULL );

	/* add time out */
	XtAddTimeOut( TIMEOUT, TimeoutCB, &clocksStruct );

    XtMainLoop();
}
