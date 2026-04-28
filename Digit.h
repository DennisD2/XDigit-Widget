
/*****************************************************
 * Digit.h: Public header file for Digit Widget Class
 *****************************************************/
#ifndef  DIGIT_H
#define  DIGIT_H

#include <X11/StringDefs.h>

extern WidgetClass XddigitWidgetClass;
typedef struct _XdDigitClassRec * XdDigitWidgetClass;
typedef struct _XdDigitRec      * XdDigitWidget;

#define XtNsegmentMargin	"segmentMargin"
#define XtNsegmentDelta		"segmentDelta"
#define XtNshowDecimalPoint	"showDecimalPoint"

#define XtCDelta		"Delta"

extern Widget XtCreateDigit( Widget parent, char *name, ArgList args,
        int ArgCount );

/*
 * special values 
 */
#define MINUS_VALUE -1
#define DECPOINT_VALUE -2
#define DOUBLEPOINT_VALUE -3

#endif /*DIGIT_H*/

