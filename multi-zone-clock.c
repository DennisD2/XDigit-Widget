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

// widget geometries; these values should be calculated from screen dimensions; but for now, we offer
// two geometries, small (_B) and large (_A), for small screens (<1500x1000) and large screens
// Also font size is handled in this way, but should be calculated (font size) from actual geometries
#define DEFAULT_DIGIT_WIDTH_A 60
#define DEFAULT_DIGIT_HEIGHT_A 100
#define DEFAULT_TEXTAREA_WIDTH_A 200
#define LABEL_X_OFFSET_A 15
#define LABEL_Y_OFFSET_A 20
#define DEFAULT_FONT_HEIGHT_A DEFAULT_DIGIT_HEIGHT_A/4

#define DEFAULT_DIGIT_WIDTH_B 30
#define DEFAULT_DIGIT_HEIGHT_B 50
#define DEFAULT_TEXTAREA_WIDTH_B 100
#define LABEL_X_OFFSET_B 15
#define LABEL_Y_OFFSET_B 12
#define DEFAULT_FONT_HEIGHT_B DEFAULT_DIGIT_HEIGHT_B/4

#define LARGEFONT_1 "-adobe-courier-bold-r-normal--"
#define LARGEFONT_2 "-240-75-75-m-150-iso8859-1"
#define SMALLFONT_1 "-*-helvetica-bold-r-*-*-"
#define SMALLFONT_2 "-*-*-*-*-*-iso8859-1"

static void setDateLabel(Widget date, int day, int month, int year);

/*---------------------------*/
/* App Types definitions     */
/*---------------------------*/

// Clock struct
typedef struct {
	String label; // Label for this clock
	String zone; // timezone for this clock
	Widget digit[8]; //  5 or 8 digits per clock
	Widget labelWidget;
	Widget dateWidget;
} ClockStruct;

// all clocks
typedef struct {
	int numClocks;
	ClockStruct *clocks;

	int numDigits; // calculated in main(), from showSeconds
	int timeout; // calculated in setTimeoutValue()
	int screenWidth;
	int screenHeight;
	int digitWidth;
	int digitHeight;
	int textAreaWidth;
	int label_x_offset;
	int label_y_offset;
	int fontHeight;
	XmFontList titleFontList;
	XmFontList dateFontList;

} ClocksStruct;

// global static variable for all clocks
static ClocksStruct clocksStruct;

typedef struct {
	int h;
	int m;
	int s;
	int day;
	int month;
	int year;
} DigitStruct;

/*---------------------------*/
/* App Resources definitions */
/*---------------------------*/

typedef struct {
	Pixel foreground;
	Pixel background;
	String labels;
	Boolean showSeconds;
	XFontStruct *titleFont;
	XFontStruct *dateFont;
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
	{ "titleFont", XtCFont, XtRFontStruct, sizeof(XFontStruct *),
	XtOffsetOf(Resources, titleFont),
	XtRString, "XtDefaultFont"},
	{ "dateFont", XtCFont, XtRFontStruct, sizeof(XFontStruct *),
	XtOffsetOf(Resources, dateFont),
	XtRString, "XtDefaultFont"},
};

/*---------------------------*/
/* App functions             */
/*---------------------------*/

// Set timeout value either to default or a new value
void setTimeoutValue(int newValue) {
	if (newValue == TIMEOUT_DEFAULT) {
		if (theResources.showSeconds) {
			clocksStruct.timeout = TIMEOUT_WITH_SECONDS;
		} else {
			clocksStruct.timeout = TIMEOUT_NOSECONDS;
		}
	} else {
		clocksStruct.timeout = newValue;
	}
}

/*
 * Get time and convert to values suitable for the Digit widgets.
 * Can return arbitrary remote "local" times. This feature is reached by manipulating TZ variable.
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
	digits->day = tt->tm_mday;
	digits->month = tt->tm_mon;
	digits->year = tt->tm_year +1900L;
	//printf("h:m = %d:%d:%d\n", digits->h, digits->m, digits->s);
}

/*
 * Set all widgets value resources to current time/date value. Includes digits and date widget.
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

	setDateLabel( clock->dateWidget, digits.day, digits.month, digits.year);

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
	XtAddTimeOut( clocksStruct.timeout, TimeoutCB, clockStruct );
}

/**
 * Create all required digit widgets fopr a single clock row
 * @param compo parent
 * @param clockDigits in/out parameter containinga ll created widgets
 * @param row clock row for we are creating new widgets
 */
static void createClockWidgets(Widget compo, ClockStruct *clockDigits, int row) {
	Arg args[8];
	for ( int i=0; i<clocksStruct.numDigits; i++ ) {
		int n=0;
		XtSetArg( args[n], XtNx, (Position)i*clocksStruct.digitWidth ); n++;
		XtSetArg( args[n], XtNy, (Position)row*clocksStruct.digitHeight ); n++;
		XtSetArg( args[n], XtNwidth, (Dimension)clocksStruct.digitWidth ); n++;
		XtSetArg( args[n], XtNheight, (Dimension)clocksStruct.digitHeight ); n++;
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
 * Create a label/title widget and a date widget for a clock
 * @param compo parent widget
 * @param numClock clock row
 * @param title text to display
 * @param labelWidget returns created widget
 * @param dateWidget returns created widget
 */
static void createClockLabelWidgets(Widget compo, int numClock, char* title, Widget *labelWidget, Widget *dateWidget) {
	Arg wargs[7];
	int n=0;

	XmString xmstr = XmStringCreate(title, XmSTRING_DEFAULT_CHARSET);
	XtSetArg( wargs[n], XmNlabelString, xmstr ); n++;
	XtSetArg( wargs[n], XtNx, (Position)clocksStruct.label_x_offset + clocksStruct.numDigits*clocksStruct.digitWidth); n++;
	XtSetArg( wargs[n], XtNy, (Position)numClock*clocksStruct.digitHeight + clocksStruct.digitHeight/2 - clocksStruct.label_y_offset); n++;
	XtSetArg( wargs[n], XmNfontList, clocksStruct.titleFontList ); n++;
	*labelWidget = XtCreateManagedWidget("clockTitle", xmLabelWidgetClass, compo, wargs, n);
	XmStringFree( xmstr );

	n=0;
	xmstr = XmStringCreate("hehe", XmSTRING_DEFAULT_CHARSET);
	XtSetArg( wargs[n], XmNlabelString, xmstr ); n++;
	XtSetArg( wargs[n], XtNx, (Position)clocksStruct.label_x_offset + clocksStruct.numDigits*clocksStruct.digitWidth); n++;
	XtSetArg( wargs[n], XtNy, (Position)numClock*clocksStruct.digitHeight + clocksStruct.digitHeight/2 + clocksStruct.label_y_offset/2 ); n++;
	XtSetArg( wargs[n], XmNfontList, clocksStruct.dateFontList ); n++;
	*dateWidget = XtCreateManagedWidget("clockDate", xmLabelWidgetClass, compo, wargs, n);
	XmStringFree( xmstr );
}

/**
 * Set date string for a dateWidget
 * @param dateWidget widget to use
 * @param day date part
 * @param month date part
 * @param year date part
 */
static void setDateLabel(Widget dateWidget, int day, int month, int year) {
	Arg args[1];
	char buf[32];
	sprintf(buf, "%d.%d.%d", day, month, year);
	XmString xmstr = XmStringCreate(buf, XmSTRING_DEFAULT_CHARSET);
	XtSetArg( args[0], XmNlabelString, xmstr );
	XtSetValues( dateWidget, args, 1 );
	XmStringFree( xmstr );
}

/**
 * Parse "clocks" string from resources
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
 * Splits clock title and clock timezone string parts
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

//#define LABEL_X_OFFSET 10+5
//#define LABEL_Y_OFFSET 20


void calculateWidgetDimensions(ClocksStruct *clocksStruct) {
	if (clocksStruct->screenWidth > 1500 && clocksStruct->screenHeight > 1000) {
		clocksStruct->digitWidth = DEFAULT_DIGIT_WIDTH_A; // screen w / 64
		clocksStruct->digitHeight = DEFAULT_DIGIT_HEIGHT_A;
		clocksStruct->textAreaWidth = DEFAULT_TEXTAREA_WIDTH_A;
		clocksStruct->label_x_offset = LABEL_X_OFFSET_A;
		clocksStruct->label_y_offset = LABEL_Y_OFFSET_A;
		clocksStruct->fontHeight = DEFAULT_FONT_HEIGHT_A - DEFAULT_FONT_HEIGHT_A%2;

	} else {
		clocksStruct->digitWidth = DEFAULT_DIGIT_WIDTH_B; // screen w / 64
		clocksStruct->digitHeight = DEFAULT_DIGIT_HEIGHT_B;
		clocksStruct->textAreaWidth = DEFAULT_TEXTAREA_WIDTH_B;
		clocksStruct->label_x_offset = LABEL_X_OFFSET_B;
		clocksStruct->label_y_offset = LABEL_Y_OFFSET_B;
		clocksStruct->fontHeight = DEFAULT_FONT_HEIGHT_B + DEFAULT_FONT_HEIGHT_B%2;
	}
}

/**
 * Dump all font names and other info from a XmFontList object
 *
 * @param display
 * @param fontList
 */
void dumpFontList(Display *display, XmFontList fontList) {
	XmFontContext context;
	if (XmFontListInitFontContext(&context, fontList) == 0) {
		printf("Cannot create XmFontListContext");
	}
	XmFontListEntry entry = XmFontListNextEntry(context);
	while (entry != NULL) {
		XmFontType fontType;
		XPointer ret = XmFontListEntryGetFont(entry, &fontType);
		char *tag = XmFontListEntryGetTag(entry);

		char whatisit[256];
		XFontStruct *font;
		unsigned long value;
		char *fontName;
		switch (fontType) {
			case XmFONT_IS_FONT:
				font = (XFontStruct *)ret;
				int fontHeight = font->ascent + font->descent;
				int fontWidth  = font->max_bounds.width;
				if (XGetFontProperty(font, XA_FONT, &value)) {
					fontName = XGetAtomName(display, (Atom)value);
					if (fontName != NULL) {
						sprintf(whatisit, "Font, name: %s, tag: %s. char dimension hxw=%dx%d\n", fontName, tag,
							fontHeight, fontWidth);
					}
				}
				break;
			case XmFONT_IS_FONTSET:
				sprintf(whatisit, "FontSet %X", (unsigned int)ret);
				break;
			case XmFONT_IS_XFT:
				sprintf(whatisit, "XFT(?) %X", (unsigned int)ret);
				break;
		}
		printf("%s\n", whatisit);
		free(tag);

		entry = XmFontListNextEntry(context);
	}
	XmFontListFreeFontContext(context);
}

/**
 * Load fonts for clock title and date string
 * @param display
 * @param clocks_struct
 */
void loadFonts(Display *display, ClocksStruct * clocks_struct) {
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
	Motif uses an own resource named XmNfontList, so the solution looks different, see below
	*/

	/* Solution for Motif */
	// Next line: simplest approach just use font loaded via resource
	//XFontStruct *font_info = theResources.titleFont;

	// We try another approach: try to calculate good fitting font
	// clocksStruct.fontHeight was adjusted accoring to digit height, and now we use it
	// to construct a font name with this height value
	char targetFont[256];
	if (clocksStruct.fontHeight == DEFAULT_FONT_HEIGHT_A) {
		 sprintf(targetFont,"%s%d%s", LARGEFONT_1, clocksStruct.fontHeight, LARGEFONT_2);
	} else {
		sprintf(targetFont, "%s%d%s", SMALLFONT_1, clocksStruct.fontHeight, SMALLFONT_2);
	}
	printf("Target font: %s\n", targetFont);
	// Try to load a fitting font
	XFontStruct *font_info = XLoadQueryFont(display, targetFont);
	if (font_info == NULL) {
		printf("No fonts\n");
	}
	/* create a motif font list and store in global var for later use */
	XmFontList fontList = XmFontListCreate(font_info, XmFONTLIST_DEFAULT_TAG);
	clocksStruct.titleFontList = fontList;
	dumpFontList(display, fontList);

	//font_info = theResources.dateFont;
	if (clocksStruct.fontHeight == DEFAULT_FONT_HEIGHT_A) {
		sprintf(targetFont,"%s%d%s", LARGEFONT_1, clocksStruct.fontHeight, LARGEFONT_2);
	} else {
		sprintf(targetFont, "%s%d%s", SMALLFONT_1, clocksStruct.fontHeight, SMALLFONT_2);
	}
	font_info = XLoadQueryFont(display, targetFont);
	if (font_info == NULL) {
		printf("No fonts\n");
	}
	/* create a motif font list and store in global var for later use */
	fontList = XmFontListCreate(font_info, XmFONTLIST_DEFAULT_TAG);
	clocksStruct.dateFontList = fontList;
	dumpFontList(display, fontList);
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
	clocksStruct.screenWidth = XDisplayWidth(display, 0);
	clocksStruct.screenHeight = XDisplayHeight(display, 0);

	// to force small size
	//clocksStruct.screenWidth = 1024;
	//clocksStruct.screenHeight = 768;
	printf("Screen dimensions %dx%d\n", clocksStruct.screenWidth, clocksStruct.screenHeight);

	calculateWidgetDimensions(&clocksStruct);

	loadFonts(display, &clocksStruct);

	//theResources.showSeconds=1;
	if (theResources.showSeconds) {
		clocksStruct.numDigits = DIGIT_WIDGETS_NUM_WITH_SECONDS;
	} else {
		clocksStruct.numDigits = DIGIT_WIDGETS_NUM_NOSECONDS;
	}
	setTimeoutValue(TIMEOUT_DEFAULT);

    /*
     * Create a container widget for all the digits
     */
    int n = 0;
    XtSetArg( args[n], XtNwidth, (Dimension)clocksStruct.numDigits*clocksStruct.digitWidth + clocksStruct.textAreaWidth ); n++;
    XtSetArg( args[n], XtNheight, (Dimension)numClocks*clocksStruct.digitHeight ); n++;
    Widget compo = XtCreateManagedWidget("clockPanel", compositeWidgetClass,
                                         toplevel, args, n);

	/*
     * Create all digit widgets and title+date widgets per clock
     */
	for ( i=0; i<numClocks; i++ ) {
		createClockLabelWidgets(compo,i, labels[i], &(clocksStruct.clocks[i].labelWidget), &(clocksStruct.clocks[i].dateWidget));
		createClockWidgets(compo, &clocksStruct.clocks[i], i);
	}

    XtRealizeWidget(toplevel);

    /* init clock display */
	TimeoutCB( (XtPointer)&clocksStruct, NULL );

	/* add time out */
	XtAddTimeOut( clocksStruct.timeout, TimeoutCB, &clocksStruct );

    XtMainLoop();
}
