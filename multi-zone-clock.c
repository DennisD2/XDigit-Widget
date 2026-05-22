/*************************************************************
 * multi-zone-clock.c : multi-zone digital clock
 * Currently, "multi" means 2 zones: local time and GMT time
 *************************************************************/

#define NUM_CLOCKS 3

#define TIME_LOCAL 0
#define TIME_GMT 1
#define TIME_NEW_YORK 2

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

char *clockTitle[NUM_CLOCKS] = { "Frankfurt", "GMT", "New York"};

/*
 * Set all widgets value resouce to digit values defined by values array
 */
void setClocksValue(Widget *digit[5], int values[4]) {
	Arg args[1];

	XtSetArg(args[0], XtNvalue, values[0]);
	XtSetValues( *digit[0], args, 1 );
	XtSetArg(args[0], XtNvalue, values[1]);
	XtSetValues( *digit[1], args, 1 );
	XtSetArg( args[0], XtNvalue, values[2]);
	XtSetValues( *digit[3], args, 1 );
	XtSetArg( args[0], XtNvalue, values[3]);
	XtSetValues( *digit[4], args, 1 );
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
 * Timeout callback
 */
void TimeoutCB( XtPointer client_data, XtIntervalId* id ) {
	Widget **digit = (Widget**)client_data;

	int num_values[4];

	getCurrentTime((int*)num_values, TIME_LOCAL);
	setClocksValue(digit, num_values);

	getCurrentTime((int*)num_values, TIME_GMT);
	setClocksValue(&digit[5], num_values);

	getCurrentTime((int*)num_values, TIME_GMT);
	num_values[1] -= 4 ;
	if (num_values[1] < 0) num_values[1] += 12;
	setClocksValue(&digit[10], num_values);

	/*
	 * start time out from the beginning 
	 */
	XtAddTimeOut( TIMEOUT, TimeoutCB, digit );
}

void createClockWidgets(Widget compo, Widget digit[], int row) {
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
		digit[i] = XtCreateManagedWidget("digit", XddigitWidgetClass,
		                                    compo, args, n);
	}
}

typedef struct _Resources {
	Pixel foreground;
	Pixel background;
} Resources;

static Resources theResources;

static XtResource resourceSpec[] = {
	{ XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
	  XtOffsetOf(Resources, foreground),
	  XtRString, "XtDefaultForeground"},
	{ XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel),
	  XtOffsetOf(Resources, background),
	  XtRString, "XtDefaultBackground"},
};

void createClockLabel(Widget compo, int numClock, char* title) {
	Arg wargs[5];
	int n;
	n=0;
	XmString xmstr = XmStringCreate(title, XmSTRING_DEFAULT_CHARSET);
	XtSetArg( wargs[n], XmNlabelString, xmstr ); n++;
	XtSetArg( wargs[n], XtNx, (Position)10 + 5*60 + 5); n++;
	XtSetArg( wargs[n], XtNy, (Position)numClock*100 + 100/2 ); n++;
	XtSetArg( wargs[n], XtNforeground, theResources.foreground /*XtDefaultForeground*/ ); n++;
	XtSetArg( wargs[n], XtNbackground, theResources.background ); n++;
	XtCreateManagedWidget("clockTitle", xmLabelWidgetClass, compo, wargs, n);
}

int main(int argc, char **argv) {
    Widget toplevel, compo, digit[NUM_CLOCKS][5];
    Arg args[8]; int n, i;

    /*
     * Initialize the Intrinsics.
     */   
    toplevel = XtInitialize(argv[0], "XDigit", NULL, 
                            0, &argc, argv);
	/* get apps resources for use in createClockLabel() */
	XtGetApplicationResources(toplevel, &theResources,
						   resourceSpec, XtNumber(resourceSpec), NULL, 0);

    /*
     * Create a container widget for all the digits
     */
    n=0;
    XtSetArg( args[n], XtNwidth, (Dimension)400 ); n++;
    XtSetArg( args[n], XtNheight, (Dimension)NUM_CLOCKS*100 ); n++;
    compo = XtCreateManagedWidget("panel", compositeWidgetClass,
	toplevel, args, n);
    /*
     * Create some digit widgets 
     */


	for ( i=0; i<NUM_CLOCKS; i++ ) {
		createClockLabel(compo,i, clockTitle[i]);
		createClockWidgets(compo, digit[i], i);
	}
    XtRealizeWidget(toplevel);

    /* init clock display */
	TimeoutCB( (XtPointer)digit, NULL );

	/* add time out */
	XtAddTimeOut( TIMEOUT, TimeoutCB, digit );

    XtMainLoop();
}
