/*************************************************************
 * multi-zone-clock.c : multi-zone digital clock
 *************************************************************/

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/Composite.h>

#include <Xm/Xm.h>
#include <Xm/Label.h>

#include "Digit.h"

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

/*---------------------------*/
/* App defines               */
/*---------------------------*/
// Maximum number of clocks
#define MAX_CLOCKS 10

#define TIMEOUT_NOSECONDS 10000L /* 10s */
#define TIMEOUT_WITH_SECONDS 1000L /* 1s */
#define TIMEOUT_DEFAULT -1

#define DIGIT_WIDGETS_NUM_NOSECONDS 5
#define DIGIT_WIDGETS_NUM_WITH_SECONDS DIGIT_WIDGETS_NUM_NOSECONDS+3

/*---------------------------*/
/* App Types definitions     */
/*---------------------------*/

// Clock struct
typedef struct {
	String label; // Label for this clock
	String zone; // timezone for this clock
	Widget digit[8]; //  5 or 8 digits per clock
} ClockStruct;

// all clocks, additional number of clocks
typedef struct {
	int numClocks;
	ClockStruct *clocks;
} ClocksStruct;

// global static variable for all clocks
static ClocksStruct clocksStruct;

typedef struct {
	int h;
	int m;
	int s;
} DigitStruct;

// global static variable for all fonts
static XmFontList the_font_list;

/*---------------------------*/
/* App Resources definitions */
/*---------------------------*/

typedef struct {
	Pixel foreground;
	Pixel background;
	String labels;
	Boolean showSeconds;
	int numDigits; // calculated in main(), from showSeconds
	int timeout; // calculated in main(), from showSeconds
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
	{ "clocks", "Clocks", XtRString, sizeof(String),
	XtOffsetOf(Resources, labels),
	XtRString, "local"},
	{ "showSeconds", XtCBoolean, XtRBoolean, sizeof(Boolean),
	XtOffsetOf(Resources, showSeconds),
	XtRString, "false"},
	/*{ XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct *),
		XtOffsetOf(Resources, fontStruct),
		XtRString, "XtDefaultFont"},*/
};

/*---------------------------*/
/* App functions             */
/*---------------------------*/

// Set timeout value either to default or a new value
void setTimeoutValue(int newValue) {
	if (newValue == TIMEOUT_DEFAULT) {
		if (theResources.showSeconds) {
			theResources.timeout = TIMEOUT_WITH_SECONDS;
		} else {
			theResources.timeout = TIMEOUT_NOSECONDS;
		}
	} else {
		theResources.timeout = newValue;
	}
}

/*
 * Get time and convert to values suitable for the Digit widgets.
 * Can return local time of GMT time, depending on time_zone value.
 */
static void getCurrentTime(DigitStruct *digits, String zone) {
	if (strcmp(zone, "Local") == 0) {
		// No TZ required for our own local time
		unsetenv("TZ");
	} else {
		// Manipulate TZ variable for reading local time for different time zones
		char zoneEnv[64];
		sprintf(zoneEnv, "TZ=%s", zone);
		putenv(zoneEnv);
	}

	time_t t;
	time( &t );
	struct tm * tt;
	tt = localtime(&t);
	//printf("time: %s\n", asctime(tt));

	digits->h = tt->tm_hour;
	digits->m = tt->tm_min;
	digits->s = tt->tm_sec;
	//printf("h:m = %d:%d:%d\n", digits->h, digits->m, digits->s);
}

/*
 * Set all widgets value resources to digit values defined by values array
 */
static void setClockValue(const ClockStruct *clock) {
	Arg args[1];
	DigitStruct digits;

	getCurrentTime(&digits, clock->zone);

	if (digits.h/10 == 0) {
		XtSetArg(args[0], XtNvalue, NO_VALUE);
		XtSetValues( clock->digit[0], args, 1 );
	} else {
		XtSetArg(args[0], XtNvalue, digits.h/10);
		XtSetValues( clock->digit[0], args, 1 );
	}

	XtSetArg(args[0], XtNvalue, digits.h%10);
	XtSetValues( clock->digit[1], args, 1 );
	XtSetArg( args[0], XtNvalue, digits.m/10);
	XtSetValues( clock->digit[3], args, 1 );
	XtSetArg( args[0], XtNvalue, digits.m%10);
	XtSetValues( clock->digit[4], args, 1 );
	if (theResources.showSeconds) {
		XtSetArg(args[0], XtNvalue, digits.s/10);
		XtSetValues( clock->digit[6], args, 1 );
		XtSetArg(args[0], XtNvalue, digits.s%10);
		XtSetValues( clock->digit[7], args, 1 );
	}

	// Optimize timeout value to match as good as possible the zero crossing of seconds value
	// Not required if we have timeout every second:
	if (theResources.showSeconds)
		return;
	// optimize timeout value
	int glitch = digits.s % 10;
	//printf("glitch=%d\n", glitch);
	if (glitch == 0) {
		setTimeoutValue(TIMEOUT_DEFAULT);
	} else {
		setTimeoutValue(10 - glitch);
	}
}

/*
 * Timeout callback
 */
static void TimeoutCB( XtPointer client_data, XtIntervalId* id ) {
	ClocksStruct *clockStruct  = (ClocksStruct *)client_data;

	for (int i=0; i<clockStruct->numClocks; i++) {
		setClockValue(&clockStruct->clocks[i]);
	}

	/*
	 * start time out from the beginning 
	 */
	XtAddTimeOut( theResources.timeout, TimeoutCB, clockStruct );
}

//
/**
 * Create all required digit widgets fopr a single clock row
 * @param compo parent
 * @param clockDigits in/out parameter containinga ll created widgets
 * @param row clock row for we are creating new widgets
 */
static void createClockWidgets(Widget compo, ClockStruct *clockDigits, int row) {
	Arg args[8];
	for ( int i=0; i<theResources.numDigits; i++ ) {
		int n=0;
		XtSetArg( args[n], XtNx, (Position)i*60 ); n++;
		XtSetArg( args[n], XtNy, (Position)row*100 ); n++;
		XtSetArg( args[n], XtNwidth, (Dimension)60 ); n++;
		if (i==2 || i==5 )
			XtSetArg( args[n], XtNvalue, DOUBLEPOINT_VALUE );
		else
			XtSetArg( args[n], XtNvalue, i );
		n++;
		clockDigits->digit[i] = XtCreateManagedWidget("digit", XddigitWidgetClass,
		                                    compo, args, n);
	}
}

/**
 * Create a label/title for a clock row
 * @param compo parent widget
 * @param numClock clock row
 * @param title text to display
 */
static void createClockLabelWidget(Widget compo, int numClock, char* title) {
	Arg wargs[7];
	int n=0;

	XmString xmstr = XmStringCreate(title, XmSTRING_DEFAULT_CHARSET);
	XtSetArg( wargs[n], XmNlabelString, xmstr ); n++;
	XtSetArg( wargs[n], XtNx, (Position)10 + theResources.numDigits*60 + 5); n++;
	XtSetArg( wargs[n], XtNy, (Position)numClock*100 + 100/2 ); n++;
	XtSetArg( wargs[n], XmNfontList, the_font_list ); n++;
	XtCreateManagedWidget("clockTitle", xmLabelWidgetClass, compo, wargs, n);
}

/**
 *
 * @param labelString string like "Frankfurt=Local,GMT,New York=America/New_York"
 * @param labels Array of strings created from labelString by splitting at delimiter ','
 * @return number of strings read
 */
static int readClockInfos(String labelString, String *labels) {
	int i=0;
	String token = strtok(theResources.labels,",");
	labels[i] = token;
	while (token != NULL) {
		labels[i++] = token;
		token = strtok(NULL, ",");
	}
	return i;
}

/**
 *
 * @param info input string like "Yolo=Europe/Berlin"
 * @param parts Array of strings created from labelString by splitting at delimiter '='
 * @return number of strings read (correct is 1 or 2)
 */
static int splitInfo(String info, String *parts) {
	int i=0;
	String token = strtok(info,"=");
	parts[i] = token;
	while (token != NULL) {
		parts[i++] = token;
		token = strtok(NULL, "=");
	}
	return i;
}

int main(int argc, char **argv) {
	Arg args[8]; int i;

    /* Initialize the Intrinsics */
    Widget toplevel = XtInitialize(argv[0], "MultiZoneClock", NULL,
                                   0, &argc, argv);
	/* Read app resources */
	XtGetApplicationResources(toplevel, &theResources,
						   resourceSpec, XtNumber(resourceSpec), NULL, 0);

	String labels[MAX_CLOCKS];
	int numClocks = readClockInfos(theResources.labels, labels);
	if (numClocks > MAX_CLOCKS) {
		printf("Too many clocks defined!\n");
		numClocks = MAX_CLOCKS;
	} else {
		printf("%d clocks defined\n", numClocks);
	}
	printf("showSeconds = %d\n", theResources.showSeconds);

    clocksStruct.numClocks = numClocks;
    clocksStruct.clocks = malloc(sizeof(ClockStruct)*numClocks);
	for (i=0; i<numClocks; i++) {
		String infoParts[2];
		infoParts[0] = "?"; infoParts[1]="?";
		int n = splitInfo(labels[i], infoParts);
		if (n > 2) {
			printf("Strange clock info (%s)!\n", labels[i]);
		}
		clocksStruct.clocks[i].label = infoParts[0];
		clocksStruct.clocks[i].zone = infoParts[1];
		printf("Clock %d, label='%s', zone='%s'\n", i, clocksStruct.clocks[i].label, clocksStruct.clocks[i].zone);
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

	//theResources.showSeconds=1;
	if (theResources.showSeconds) {
		theResources.numDigits = DIGIT_WIDGETS_NUM_WITH_SECONDS;
	} else {
		theResources.numDigits = DIGIT_WIDGETS_NUM_NOSECONDS;
	}
	setTimeoutValue(TIMEOUT_DEFAULT);

    /*
     * Create a container widget for all the digits
     */

    int n = 0;
    XtSetArg( args[n], XtNwidth, (Dimension)theResources.numDigits*60 + 200 ); n++;
    XtSetArg( args[n], XtNheight, (Dimension)numClocks*100 ); n++;
    Widget compo = XtCreateManagedWidget("clockPanel", compositeWidgetClass,
                                         toplevel, args, n);

	/*
     * Create all digit widgets
     */
	for ( i=0; i<numClocks; i++ ) {
		createClockLabelWidget(compo,i, labels[i]);
		createClockWidgets(compo, &clocksStruct.clocks[i], i);
	}

    XtRealizeWidget(toplevel);

    /* init clock display */
	TimeoutCB( (XtPointer)&clocksStruct, NULL );

	/* add time out */
	XtAddTimeOut( theResources.timeout, TimeoutCB, &clocksStruct );

    XtMainLoop();
}
