/*************************************************************
 * digit3.c : another digital clock
 *************************************************************/

#define NUM_CLOCKS 2

#include <X11/Intrinsic.h> 
#include <X11/Composite.h>
#include "Digit.h"

#include <time.h>
#define TIMEOUT 10000L
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

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

void TimeoutCB( XtPointer client_data, XtIntervalId* id ) {
	Widget **digit = (Widget**)client_data;
	time_t t;
	char *buf, *p, *values[4];
	int i, num_values[4];

	for (i=0;i<4;i++) {
	   values[i] = (char*)malloc( 2 );
	   values[i][0] = '0'; values[i][1]='\0';
	}

    /*
	 * Get time and convert to values suitable for the Digit widgets
	 */
	time( &t );
	buf = ctime(&t);
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
	/*
	 * set value-resource of the digits
	 */

	setClocksValue(digit, num_values);
	setClocksValue(&digit[5], num_values);

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

int main(int argc, char **argv) {
    Widget toplevel, compo, digit[NUM_CLOCKS][5];
    Arg args[8]; int n, i;

    /*
     * Initialize the Intrinsics.
     */   
    toplevel = XtInitialize(argv[0], "XDigit", NULL, 
                            0, &argc, argv);

    /*
     * Create a container widget for all the digits
     */
    n=0;
    XtSetArg( args[n], XtNwidth, (Dimension)300 ); n++;
    XtSetArg( args[n], XtNheight, (Dimension)NUM_CLOCKS*100 ); n++;
    compo = XtCreateManagedWidget("panel", compositeWidgetClass,
	toplevel, args, n);
    /*
     * Create some digit widgets 
     */
	for ( i=0; i<NUM_CLOCKS; i++ ) {
		createClockWidgets(compo, digit[i], i);
	}
    XtRealizeWidget(toplevel);

    /* init clock display */
	TimeoutCB( (XtPointer)digit, NULL );

	/* add time out */
	XtAddTimeOut( TIMEOUT, TimeoutCB, digit );

    XtMainLoop();
}
