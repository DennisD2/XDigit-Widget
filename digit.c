/*
 *
 */

/*************************************************************
 * digit.c : test the Digit widget class
 *************************************************************/

#include <X11/Intrinsic.h> 
#include "Digit.h"

#include <stdio.h>
void KeyEventHandler( Widget w, XtPointer client_data, XEvent *event, 
	Boolean *continue_to_dispatch )
{
/*
	XKeyEvent *key = (XKeyEvent *)event ;
	printf( "key={%c}\n", key->keycode );
*/
  static int v = 0;
  Arg args[1]; int n=0;

	XtSetArg( args[n], XtNvalue, v ); n++ ;
	XtSetValues( w, args, n );
	v++; if (v==10) v=-3;
}

void main(argc, argv)
    int   argc;
    char *argv[];
  {
    Widget toplevel, digit;

    /*
     * Initialize the Intrinsics.
     */   
    toplevel = XtInitialize(argv[0], "DigitTest", NULL, 
                            0, &argc, argv);
    /*
     * Create a digit widget 
     */
    digit = XtCreateManagedWidget("digit", XddigitWidgetClass, 
                                 toplevel, NULL, 0);
    XtAddEventHandler( digit, KeyPressMask, FALSE,
    	KeyEventHandler, NULL );
    XtRealizeWidget(toplevel);
    XtMainLoop();
}


