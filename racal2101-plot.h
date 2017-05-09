#include <stdlib.h>
#include <gtk/gtk.h>
#include <cairo/cairo.h>

#define NODEBUG
#define MAXLEN 200

#ifndef PROGRAM
#define PROGRAM "Racal2101-plot"
#endif

#ifndef VERSION
#define VERSION  "v 1.0 GTK"
#endif

#ifndef COPYRIGHT
#define COPYRIGHT "GPL 2017"
#endif

#ifndef COMMENT
#define COMMENT "Datalooger for Racal 2101 microwave counter"
#endif

#ifndef URL
#define URI	"http://waklam.de/amateurfunk/racal2010-plot.html"
#endif

typedef struct	{
  time_t	timestamp;
  gdouble    value;	
}racal2101_record;

typedef struct {
	GtkWidget *window;
	GtkWidget *widget_set_maxtime;
	GtkWidget *widget_set_mintime;
	GtkWidget *widget_set_average_window;
	GtkWidget *widget_set_max;
	GtkWidget *widget_set_min;	
	GtkWidget *widget_max;
	GtkWidget *widget_min;	
	GtkWidget *widget_maxtime;
	GtkWidget *widget_menuitem_start;
	GtkAdjustment *adj_set_max;
	GtkAdjustment *adj_set_min;
	GtkWidget *filesel;
	GtkWidget *btn_auto;
	int auto_man;
} r2101_data;
