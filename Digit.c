/*
 *
 *
 * 
 */

/********************************************************
 * Digit.c: The Digit Widget Methods
 *********************************************************/
#include <stdio.h>
#include <math.h>
#include <X11/IntrinsicP.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/CoreP.h>
#include "DigitP.h"
#include "Digit.h"

static void    Initialize();
static void    Redisplay();
static void    Resize();
static void    Destroy();
static Boolean SetValues();

static void DrawValue(  XdDigitWidget w, int new, int old );
static void DrawSegment(  XdDigitWidget w, int id );
static void _drawSegment( XdDigitWidget w, int ox, int oy, int dir ); 
static void _drawDecimalPoint( XdDigitWidget w, int ox, int oy );

static char defaultTranslations[] =  "";

static XtActionsRec actionsList[] = { };

static XtResource resources[] = { 
  {XtNsegmentMargin, XtCMargin, XtRDimension, sizeof (Dimension),
   XtOffset(XdDigitWidget, digit.segment_margin), 
   XtRString, "1" },
  {XtNsegmentDelta, XtCDelta, XtRDimension, sizeof (int),
   XtOffset(XdDigitWidget, digit.segment_delta), 
   XtRString, "5" },
  {XtNvalue, XtCValue, XtRInt, sizeof (int),
   XtOffset(XdDigitWidget, digit.value), 
   XtRString, "0" },
  {XtNshowDecimalPoint, XtCBoolean, XtRBoolean, sizeof (Boolean),
   XtOffset(XdDigitWidget, digit.show_decimalpoint), 
   XtRString, "False" },
  {XtNforeground, XtCForeground, XtRPixel, sizeof (Pixel),
   XtOffset(XdDigitWidget, digit.foreground), 
   XtRString, "Black"                                    },

};

XdDigitClassRec  XddigitClassRec = {
     /* CoreClassPart */
  {
   (WidgetClass) &widgetClassRec,  /* superclass            */
   "Digit",                        /* class_name            */
   sizeof(XdDigitRec),              /* widget_size           */
   NULL,                           /* class_initialize      */
   NULL,                           /* class_part_initialize */
   FALSE,                          /* class_inited          */
   Initialize,                     /* initialize            */
   NULL,                           /* initialize_hook       */
   XtInheritRealize,               /* realize               */
   actionsList,                    /* actions               */
   XtNumber(actionsList),          /* num_actions           */
   resources,                      /* resources             */
   XtNumber(resources),            /* num_resources         */
   NULLQUARK,                      /* xrm_class             */
   TRUE,                           /* compress_motion       */
   TRUE,                           /* compress_exposure     */

   TRUE,                           /* compress_enterleave   */
   TRUE,                           /* visible_interest      */
   Destroy,                        /* destroy               */
   Resize,                         /* resize                */
   Redisplay,                      /* expose                */
   SetValues,                      /* set_values            */
   NULL,                           /* set_values_hook       */
   XtInheritSetValuesAlmost,       /* set_values_almost     */
   NULL,                           /* get_values_hook       */
   NULL,                           /* accept_focus          */
   XtVersion,                      /* version               */
   NULL,                           /* callback private      */
   defaultTranslations,            /* tm_table              */
   NULL,                           /* query_geometry        */
   NULL,                           /* display_accelerator   */
   NULL,                            /* extension             */
   },
      /* Digit class fields */
  {
   0,                              /* ignore                */
   }
};

WidgetClass XddigitWidgetClass = (WidgetClass) &XddigitClassRec;

/* major drawing direction for a segment */
#define DIR_HORIZONTAL 0
#define DIR_VERTICAL   1

/* unique value */
#define NUM_UNKNOWN -99

static void Initialize (request, new)
  XdDigitWidget request, new;
{
  XGCValues values;
  XtGCMask  valueMask;

  /*
   * Make sure the window size is not zero. The Core 
   * Initialize() method doesn't do this.
   */
  if (request->core.width == 0)
    new->core.width = 50;
  if (request->core.height == 0)
    new->core.height = 100;

  if (request->digit.segment_delta<0) {
	XtWarning ("Segment delta must be non-negative" );
	new->digit.segment_delta=0;
  }
  if (request->digit.value<-3 || request->digit.value>9) {
	XtWarning ("Digit value out of range" );
	request->digit.value=0;
  }

  /*
   * Create the graphics context
   */
  valueMask = GCForeground | GCBackground | GCFillStyle ;
  values.foreground = new->digit.foreground;
  values.background = new->core.background_pixel;
  values.fill_style = FillSolid;
  new->digit.segment_GC = XtGetGC ( (Widget)new, valueMask, &values);  
  Resize (new);

}

static void Destroy (w)
  XdDigitWidget w;
{
  XtDestroyGC (w->digit.segment_GC);
}

static void Resize (w)
     XdDigitWidget w;
{
  /*
   * recalculate height, width and delta for a segment 
   */
  w->digit.segment_height = 10*w->core.height/25;
  if (w->digit.segment_height == 0) w->digit.segment_height=1;
  w->digit.segment_width = w->digit.segment_height/5;
  if (w->digit.segment_width == 0) w->digit.segment_width=1;
  w->digit.segment_delta = w->digit.segment_width/2;
  if (w->digit.segment_delta == 0) w->digit.segment_delta = 1;
}

static void Redisplay (w, event, region)
  XdDigitWidget  w;
  XEvent       *event;
  Region        region;
{
  if(w->core.visible){
    /*
     * Set the clip masks in all graphics contexts.
     */
    XSetRegion(XtDisplay(w), w->digit.segment_GC, region);

    DrawValue(w,w->digit.value, NUM_UNKNOWN );
  }
}

static Boolean SetValues (current, request, new)
  XdDigitWidget current, request, new;
{
	XGCValues  values;
	XtGCMask   valueMask;
	Boolean    redraw = FALSE;

  /*
   * Make sure the new dial values are reasonable.
   */
  if (new->digit.value<-3 || new->digit.value>9 ) {
		XtWarning("Digit value out of range" );
		new->digit.value=0;
  }

  /*
   * If the segment color has changed, generate the GC
   */
  if (new->digit.foreground != current->digit.foreground) {
    valueMask = GCForeground | GCBackground | GCFillStyle ;
    values.foreground = new->digit.foreground;
    values.background = new->core.background_pixel;
    values.fill_style = FillSolid;
    new->digit.segment_GC = XtGetGC ( (Widget)new, valueMask, &values);   
    redraw = TRUE;     
  }

  if (new->digit.value != current->digit.value ) {
    /*
     * ok, we could do a good job here in speed up a redraw 
     * by deleting the old bogus segments and just drawing 
     * the new ones.
     * But at first, just a complete redraw is enforced .
     */
    redraw = TRUE ;
  }

  return redraw ;
}

static void DrawValue(  w, new, old )
  XdDigitWidget w;
  int new, old;
{ 
  int i, s[11] ;

  for (i=1;i<11;i++)
    s[i]=0;

  switch (new) {
      case 0: s[1]=s[2]=s[3]=s[5]=s[6]=s[7]=1; break;
      case 1: s[3]=s[6]=1; break;
      case 2: s[1]=s[3]=s[4]=s[5]=s[7]=1; break;
      case 3: s[1]=s[3]=s[4]=s[6]=s[7]=1; break;
      case 4: s[2]=s[3]=s[4]=s[6]=1; break;
      case 5: s[1]=s[2]=s[4]=s[6]=s[7]=1; break;
      case 6: s[1]=s[2]=s[4]=s[5]=s[6]=s[7]=1; break;
      case 7: s[1]=s[3]=s[6]=1; break;
      case 8: s[1]=s[2]=s[3]=s[4]=s[5]=s[6]=s[7]=1; break;
      case 9: s[1]=s[2]=s[3]=s[4]=s[6]=s[7]=1; break; 
       /* the more special ones - "value" is negative" */
      case MINUS_VALUE      : s[4]=1; break;
      case DECPOINT_VALUE   : break;
      case DOUBLEPOINT_VALUE: s[9]=s[10]=1; break; 
    }
    for (i=1;i<11;i++) 
      if (s[i])
        DrawSegment(w,i);
    if (w->digit.show_decimalpoint)
      DrawSegment(w,8);
}

static void DrawSegment( XdDigitWidget w, int id )
{
  int sw = w->digit.segment_width, sh = w->digit.segment_height;
  int dir = DIR_VERTICAL;
  int x=0,y=0;

  switch (id) {
    case 1: x=sw/2; y=0; dir=DIR_HORIZONTAL; break;
    case 2: x=0; y=sw/2; break;
    case 3: x=sh; y=sw/2; break;
    case 4: x=sw/2; y=sh +sw/2 ; dir=DIR_HORIZONTAL; break;
    case 5: x=0; y=sh+sw/2 +sw/2 ; break;
    case 6: x=sh; y=sh+sw/2 +sw/2 ; break;
    case 7: x=sw/2; y=2*sh +2*sw/2 ; dir=DIR_HORIZONTAL; break;
    case 8: x=sh+2*w->digit.segment_delta; y=2*sh +sw ; break;
    case 9: x=sh/2-sw/2; y = sh/2-sw/2; break;
    case 10: x=sh/2-sw/2; y = sh + sh/2-sw/2 ; break;
  }
  if (id==8 || id==9 || id==10)
    _drawDecimalPoint(w,x,y);
  else
    _drawSegment(w, x, y, dir );
}

static void _drawSegment( XdDigitWidget w, int ox, int oy, int dir ) 
{
  XPoint p[6];
  int sm = w->digit.segment_margin ;
  int sw = w->digit.segment_width ;
  int sd = w->digit.segment_delta ;

  if (dir==DIR_VERTICAL) {
    p[0].x = ox + sw/2; p[0].y = oy + sm ;
    p[1].x = (sw/2-sm); p[1].y = sd ;
    p[2].x = 0; p[2].y =  w->digit.segment_height - 2*sm - sd ;
    p[3].x = -p[1].x ; p[3].y = sd ;
    p[4].x = p[3].x ; p[4].y = -sd;
    p[5].x = 0; p[5].y = -p[2].y;
  }
  else {
    p[0].x = ox + sm; p[0].y = oy + sw/2 ;
    p[1].x = sd; p[1].y = -(sw/2-sm);
    p[2].x = w->digit.segment_height - 2*sm - 2*sd ; p[2].y =  0 ;
    p[3].x = sd ; p[3].y = -p[1].y ;
    p[4].x = -sd ; p[4].y = p[3].x ;
    p[5].x = -p[2].x ; p[5].y = 0 ;
  }
  XFillPolygon( XtDisplay(w), XtWindow(w),
    w->digit.segment_GC, p, 6, Convex, CoordModePrevious );
}

static void _drawDecimalPoint( w, ox, oy ) 
  XdDigitWidget w;
  int ox, oy;
{
  int x,y,ww,h;
  int sm = w->digit.segment_margin ;
  int sw = w->digit.segment_width ;

  x = ox + sm ; y = oy + sm ;
  ww = sw - 2*sm; h = sw - 2*sm ;
  XFillRectangle( XtDisplay(w), XtWindow(w),
    w->digit.segment_GC, x, y, ww, h );
}
