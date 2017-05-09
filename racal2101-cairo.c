#include "racal2101-plot.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/time.h>
#include <math.h>

#include "gpib-functions.h"

#include <gtk/gtk.h>
#include <cairo/cairo.h>
#include <cairo/cairo-pdf.h>
#include <cairo/cairo-ps.h>

#include <glib.h>
#include <glib/gprintf.h>

#define STRIP_DEBUG_LEVEL0
#define PLOT_DEBUG_LEVEL0
#define READ_RACAL_DEBUG_LEVEL1

racal2101_record record;

extern GArray *racal2101_data;

extern uint record_counter;
extern uint ud;
extern char ib_answer[16384];
extern FILE * output_fd;;
extern int handler_id;

void clear_string(char *str) {
    char *w= str;
    do {
		*w = *str++;
		w += *w && !isspace(*w);
	 } while (*w);
}

gdouble strip_answer(char *answer) {
	gdouble value;
	char channel;

#ifdef STRIP_DEBUG_LEVEL1
	g_fprintf(stderr,"Answer %s", answer);
#endif

	sscanf(answer, "F%c%lg", &channel, &value);
	
	if ((channel =='A') || (channel =='B') || (channel =='C')) 
		return value;
	return 0;	
}

void plot(cairo_t *cr, gint	width, gint height, r2101_data *data) {
	gdouble		Y=0;						// actual value
	gdouble 	T=0;
	gdouble		T0=0;

	gdouble		X_DIV = 10.0;			// 10 div/x
	gdouble		Y_DIV = 8.0;			// 10 div/y

	gdouble		MAX_Y = 1E9;			// max Y scale
	gdouble		MIN_Y =  0;				// min Y scale
	gdouble		MAX_X = record_counter;	// max X scale
	gdouble		MIN_X = 0.0;			// min X scale

	gdouble 	XMAX = 0;				// max dots x-scale
	gdouble 	YMAX= 0;				// max dots y-scale
	gdouble 	XOFFSET= 0;				// start of diagramm
	gdouble		YOFFSET= 0;				// start of diagramm
	gdouble		avg= 0;
	gdouble		x= 0, y= 0;				// actual plot postion
	gdouble		x_alt= 0,y_alt= 0;		// last plot postion
	gdouble		x_scale= 0,y_scale= 0;	// scale for axis
	gdouble		x_zero= 0,y_zero= 0;	// point of origin

	char		string_buf[MAXLEN];		// line buffer

	int			m = 0;
	int  		n = 0;					// counter Cario

	clear_string(&string_buf[0]);
#ifdef PLOT_DEBUG_LEVEL1						
	g_fprintf(stderr,"plot %f %f %f %f \n\r", MAX_X, MIN_X, MAX_Y, MIN_Y );
	
#endif	
	cairo_rectangle(cr, 0.0, 0.0, width, height);
	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	cairo_fill(cr);

	cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

	cairo_set_font_size(cr, 10.0);
	cairo_set_line_width(cr, 1.0);

	XMAX = 0.8 * width;
	YMAX = 0.8 * height;

	XOFFSET = 0.75 * (width-XMAX);
	YOFFSET = 0.5 * (height-YMAX);
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);

	g_snprintf(string_buf,MAXLEN,"%s %s",PROGRAM,VERSION);
	cairo_move_to (cr,10, 0.2* YOFFSET);
	cairo_show_text(cr, string_buf);
	cairo_stroke(cr);

	if (data->auto_man == FALSE)	{
		MAX_X = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->widget_set_maxtime));
		MIN_X = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->widget_set_mintime));
		MAX_Y = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->widget_set_max));
		MIN_Y = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->widget_set_min));
		avg =  gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->widget_set_average_window));
		if(MAX_Y < MIN_Y)
			MIN_Y = MAX_Y - 0.01 * MAX_Y;
	}
	else 
	if (data->auto_man == TRUE)	{
		if (record_counter != 0)	{
			record = g_array_index(racal2101_data, racal2101_record, 0);
			MAX_Y = MIN_Y = record.value;
			MAX_X = MIN_X = record.timestamp;
			for(m = 1; m <= record_counter-1; m ++)	{
				record = g_array_index(racal2101_data, racal2101_record, m);
					if (record.value > MAX_Y)
						MAX_Y  = record.value;
					else
					if (record.value < MIN_Y)
						MIN_Y = record.value;

					if (record.value > MAX_X)
						MAX_X  = record.timestamp;
					else
					if (record.value < MIN_X)
						MIN_X = record.timestamp;
				}
		}
		else	{
			MAX_Y = 10;
			MIN_Y = 0;			
			MAX_X = 10;
			MIN_X = 0;			
		}	
		avg = 1;	
	}

	if (MAX_Y >= 999999)	{
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(data->widget_set_max),2);
		gtk_adjustment_set_step_increment( (GtkAdjustment *) data->adj_set_max,0.01);
	}
	else
	if (MAX_Y >= 999)	{
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(data->widget_set_max),5);
		gtk_adjustment_set_step_increment( (GtkAdjustment *) data->adj_set_max,0.00001);
	}
	else	{
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(data->widget_set_max),11);
		gtk_adjustment_set_step_increment( (GtkAdjustment *) data->adj_set_max,0.00000001);
	}

	if (MIN_Y >= 999999)	{
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(data->widget_set_min),2);
		gtk_adjustment_set_step_increment( (GtkAdjustment *) data->adj_set_min,0.01);
	}
	else
	if (MIN_Y >= 999)	{
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(data->widget_set_min),5);
		gtk_adjustment_set_step_increment( (GtkAdjustment *) data->adj_set_min,0.00001);
	}	
	else	{
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(data->widget_set_min),11);
		gtk_adjustment_set_step_increment( (GtkAdjustment *) data->adj_set_min,0.00000001);
	}

	x_scale = XMAX / (MAX_X-MIN_X);
	y_scale = YMAX / (MIN_Y-MAX_Y);

	x_zero = - MIN_X * x_scale + XOFFSET;
	y_zero = YMAX - MIN_Y * y_scale + YOFFSET;

	for(m = 0; m < X_DIV+1;m ++)	{
		x =  m * XMAX / X_DIV + XOFFSET;
		if( (x >= XOFFSET) && (x <= XOFFSET+XMAX) )	{
			cairo_move_to(cr,x,YOFFSET);
			cairo_line_to(cr,x,YMAX+YOFFSET);
		}
		g_snprintf(string_buf,MAXLEN,"% .0f",MIN_X + m / X_DIV * (MAX_X-MIN_X) );

		cairo_move_to(cr,x-10,YOFFSET+YMAX+10);
		cairo_show_text(cr, string_buf);
	}

	for(m = 0; m < Y_DIV+1;m ++)	{
		y = m * YMAX / Y_DIV + YOFFSET;

		cairo_move_to(cr,XOFFSET,y);
		cairo_line_to (cr,XMAX+XOFFSET,y);

		if (MAX_Y >= 999999)
			g_snprintf(string_buf,MAXLEN,"% .2f",MAX_Y - m / Y_DIV * (MAX_Y-MIN_Y) );	
		else
		if (MAX_Y >= 999)
			g_snprintf(string_buf,MAXLEN,"% .5f",MAX_Y - m / Y_DIV * (MAX_Y-MIN_Y) );
		else	
			g_snprintf(string_buf,MAXLEN,"% .8f",MAX_Y - m / Y_DIV * (MAX_Y-MIN_Y) );
		cairo_move_to(cr,20,y);
		cairo_show_text(cr, string_buf);
	}
	cairo_stroke(cr);

	cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);

	if (record_counter != 0)	{

#ifdef PLOT_DEBUG_LEVEL2		
		g_fprintf(stderr,"%lf %lf %lf %lf \n\r",x_scale,y_scale,x_zero,y_zero);
#endif	
		m = 0;
		record = g_array_index(racal2101_data, racal2101_record, m);
		Y = record.value;
		T0 =  record.timestamp;

		if(Y > MAX_Y)	{
			Y = MAX_Y;
		}
		else
		if(Y < MIN_Y)	{
			Y = MIN_Y;
		}
			
		x_alt =  x_zero;
		y_alt =  y_zero + Y * y_scale;
		
		for(m = 1; m <= (record_counter-avg); m ++)	{
			Y = 0;
			for(n = -1; n <= avg-2; n ++)	{
				record = g_array_index(racal2101_data, racal2101_record, m+n);
				Y = Y + record.value;
#ifdef PLOT_DEBUG_LEVEL3						
				//g_fprintf(stderr,"Y=%f\n\r",Y);
#endif					
			}
			Y = Y / avg;

			record = g_array_index(racal2101_data, racal2101_record, m);				
			T = (gdouble) record.timestamp - T0;
#ifdef PLOT_DEBUG_LEVEL2						
			g_fprintf(stderr,"T=%f Y=%f avg=%f m=%d\n\r",T,Y,avg,m);
#endif	
			if(Y > MAX_Y)	{
				Y = MAX_Y;
			}
			else
			if(Y < MIN_Y)	{
				Y = MIN_Y;
			}

			x =  x_zero + T * x_scale;
			y =  y_zero + Y * y_scale;
#ifdef PLOT_DEBUG_LEVEL2						
			g_fprintf(stderr,"T=%f V=%f x=%f y=%f %d\n\r",T,Y,x,y,m);
#endif			
			if( (x_alt >= XOFFSET) && (x <= XOFFSET+XMAX) )	{
				cairo_move_to(cr,x_alt,y_alt);
				cairo_line_to (cr,x,y);
			}				
			x_alt = x;
			y_alt = y;
		}
		cairo_stroke(cr);
	}

	g_snprintf(string_buf,MAXLEN,"%12.0f",T);
	gtk_label_set_text(GTK_LABEL(data->widget_maxtime),string_buf);

	if (MAX_Y >= 1.0E9)
		g_snprintf(string_buf,MAXLEN,"%12.0f",MAX_Y);
	else
	if (MAX_Y >= 1.0E6 )
		g_snprintf(string_buf,MAXLEN,"%12.3f",MAX_Y);
	else
	if (MAX_Y >= 1.0E3)
		g_snprintf(string_buf,MAXLEN,"%12.6f",MAX_Y);

	gtk_label_set_text(GTK_LABEL(data->widget_max),string_buf);
		
	if (MIN_Y >= 1.0E9)
		g_snprintf(string_buf,MAXLEN,"%12.0f",MIN_Y);
	else
	if (MIN_Y >= 1.0E6)
		g_snprintf(string_buf,MAXLEN,"%12.3f",MIN_Y);
	else
	if (MIN_Y >= 1.0E3)
		g_snprintf(string_buf,MAXLEN,"%12.6f",MIN_Y);
		gtk_label_set_text(GTK_LABEL(data->widget_min),string_buf);
	return;
}

void create_png(GtkWidget *widget, r2101_data *data) {

	gint width = 800, height = 600;					// Window width & height
	
	cairo_surface_t *surface;
	cairo_t *cr;

	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	cr = cairo_create(surface);

	plot(cr, width,height, data);

	time_t      timeStamp;
	struct tm * timeInfo;
	char        buffer[80];
 
	time( &timeStamp );
	timeInfo = localtime( &timeStamp );
 
	strftime( buffer, 60, "%Y%m%d%H%M%S_image.png", timeInfo );
    	
	cairo_surface_write_to_png(surface, buffer);
	
	cairo_surface_destroy(surface);

	cairo_destroy(cr);

  return;
}

int read_racal2101(void) {
	char record_buffer[80]="";
	static time_t starttime;
	
	clear_string(&record_buffer[0]);
	clear_string(&ib_answer[0]);
	if(!ud) {
		g_fprintf(stderr, "Unable to open device\n");
		return -1;
	}

	if (ib_query(ud, 22,"MEAS?", ib_answer) > 0)	{
		record.value = strip_answer(ib_answer);
	}	
	
	if (record_counter == 0)	{
		starttime = time(NULL);
		record.timestamp = 0;
	}
	else
		record.timestamp = time(NULL)- starttime;

#ifdef READ_RACAL_DEBUG_LEVEL1
	g_fprintf(stderr, "Reading %3.12lg  %ld \n\r", record.value, record.timestamp);
#endif	

	g_sprintf(record_buffer, "%ld;%3.12lG\n\r", record.timestamp,record.value);
	if(output_fd)
		fputs(record_buffer, output_fd);
	else
		g_fprintf(stderr, "Could not write record to files\n\r");	
		
	g_array_append_vals(racal2101_data, &record,1);  
	record_counter ++;
	return 0;
}	
	
