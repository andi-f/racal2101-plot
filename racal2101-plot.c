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

#define BTN_AUTO_DEBUG_LEVEL0
#define READ_CSV_DEBUG_LEVEL0


GtkWidget *statusbar;			// gtk statusbar
GtkWidget *filesel;				// gtk filesel

racal2101_record record;
GArray *racal2101_data;

uint record_counter;
uint ud;
char ib_answer[16384];

int handler_id;

FILE * output_fd;;

static gboolean drawing_screen(GtkWidget *widget, GdkEventExpose *event, r2101_data *data);
static gboolean time_handler(GtkWidget *widget);
static void open_csv(GtkWidget *widget, r2101_data *data);

void toggle_bnt_auto_callback (GtkWidget *widget, r2101_data *data);
void about(GtkWidget *widget, r2101_data *data);	
void read_file(GtkWidget *widget, gpointer window);
void toogle_signal(GtkWidget *widget, gpointer window);
void on_window_closed (GtkWidget *widget, r2101_data *data);
void file_ok_proc(GtkWidget *widget, r2101_data *data);
void screen_refresh(GtkWidget *widget,r2101_data *data);
void create_png(GtkWidget *widget, r2101_data *data);
void create_plot(GtkWidget *widget, r2101_data *data);
int read_racal2101(void);
gdouble strip_answer(char *answer, char *para);
void clear_string(char *str) ;
void plot(cairo_t *cr, gint	width, gint height, r2101_data *data);

int main(int argc, char *argv[]) {
	GtkWidget *window;
	GtkWidget *vbox1;
	GtkWidget *vbox2;
	GtkWidget *hbox1;
	GtkWidget *darea;

	GtkWidget *menubar1;	

	GtkWidget *menu_file;
	GtkWidget *menuitem_file;

	GtkWidget *menu_start;
	GtkWidget *menuitem_start;

	GtkWidget *menu_png;
	GtkWidget *menuitem_png;
	GtkWidget *menu_about;
	GtkWidget *menuitem_about;
	GtkWidget *statusbar;
	GtkWidget *btn_auto;
	
	GtkWidget *label_min;
	GtkWidget *label_max;
	GtkWidget *label_maxtime;
	
	GtkWidget *widget_min;
	GtkWidget *widget_max;
	GtkWidget *widget_maxtime;
	
	GtkWidget *label_set_maxtime;	
	GtkWidget *label_set_mintime;
	GtkWidget *label_set_average_window;
	GtkWidget *label_set_min;
	GtkWidget *label_set_max;
	
	GtkWidget *widget_set_maxtime;	
	GtkWidget *widget_set_mintime;
	GtkWidget *widget_set_average_window;
	GtkWidget *widget_set_min;
	GtkWidget *widget_set_max;
	
	GtkAdjustment *adj_set_maxtime;	
	GtkAdjustment *adj_set_mintime;
	GtkAdjustment *adj_set_average_window;
	GtkAdjustment *adj_set_min;
	GtkAdjustment *adj_set_max;
	
	r2101_data wg_data;

	racal2101_data = g_array_new(FALSE, FALSE, sizeof(racal2101_record));

	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	vbox1 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox1);
	
	menubar1 = gtk_menu_bar_new();
	gtk_box_pack_start(GTK_BOX(vbox1), menubar1, FALSE, FALSE, 2);

	hbox1 = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(vbox1), hbox1);

	darea = gtk_drawing_area_new();
	gtk_box_pack_start (GTK_BOX(hbox1), darea, TRUE, TRUE, 0);

	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start (GTK_BOX(hbox1), vbox2, FALSE, TRUE, 10);
	
	g_signal_connect(darea, "expose-event", G_CALLBACK(drawing_screen),(gpointer*) &wg_data);
	gtk_widget_set_app_paintable(darea, TRUE);

	statusbar = gtk_statusbar_new();
	gtk_box_pack_start(GTK_BOX(vbox1), statusbar, FALSE, TRUE, 1);

	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

  	gtk_window_set_title(GTK_WINDOW(window), "Racal2101-plot");
	g_timeout_add(2000, (GSourceFunc) time_handler, (gpointer) window);

	//File
	menu_file = gtk_menu_new();

	menuitem_file = gtk_menu_item_new_with_label("Open");
	gtk_signal_connect(GTK_OBJECT(menuitem_file), "activate",
                     G_CALLBACK(open_csv),(gpointer*) &wg_data);
	gtk_menu_append(GTK_MENU(menu_file), menuitem_file);

	menuitem_file = gtk_menu_item_new_with_label("Quit");
	gtk_signal_connect(GTK_OBJECT(menuitem_file), "activate",
                     G_CALLBACK(on_window_closed), (gpointer*) &wg_data);
	gtk_menu_append(GTK_MENU(menu_file), menuitem_file);

	menuitem_file = gtk_menu_item_new_with_label("File");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem_file),
			    menu_file);
	gtk_menu_bar_append(GTK_MENU_BAR(menubar1), menuitem_file);

	//Start
	menu_start = gtk_menu_new();

	menuitem_start = gtk_menu_item_new_with_label("Start/Stop");
	gtk_signal_connect(GTK_OBJECT(menuitem_start), "activate",
                     G_CALLBACK(toogle_signal),darea);
	gtk_menu_append(GTK_MENU(menu_start), menuitem_start);

	menuitem_start = gtk_menu_item_new_with_label("Start");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem_start),
			    menu_start);
	gtk_menu_bar_append(GTK_MENU_BAR(menubar1), menuitem_start);

	//PNG
	menu_png = gtk_menu_new();

	menuitem_png = gtk_menu_item_new_with_label("Create PNG");
	gtk_signal_connect(GTK_OBJECT(menuitem_png), "activate",
                     G_CALLBACK(create_png), (gpointer*) &wg_data);
	gtk_menu_append(GTK_MENU(menu_png), menuitem_png);

	menuitem_png = gtk_menu_item_new_with_label("Create PNG");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem_png),
			    menu_png);
	gtk_menu_bar_append(GTK_MENU_BAR(menubar1), menuitem_png);

	//About
	menu_about = gtk_menu_new();
	menuitem_about = gtk_menu_item_new_with_label("Info");
	gtk_signal_connect(GTK_OBJECT(menuitem_about), "activate",
                     G_CALLBACK(about), "Info");
	gtk_menu_append(GTK_MENU(menu_about), menuitem_about);

	menuitem_about = gtk_menu_item_new_with_label("About");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem_about),
			    menu_about);
	gtk_menu_bar_append(GTK_MENU_BAR(menubar1), menuitem_about);

	//
	label_set_maxtime = gtk_label_new("Max Time/[s]");	
	label_set_mintime= gtk_label_new("Min Time/[s]");
	label_set_average_window= gtk_label_new("Average");
	label_set_max = gtk_label_new("Max /[Hz]]");
	label_set_min = gtk_label_new("Min /[Hz]");
	
	label_min = gtk_label_new("Record Max /[Hz]]");
	label_max = gtk_label_new("Record Min /[Hz]");
	label_maxtime = gtk_label_new("Record Max Time/[s]");	

	adj_set_maxtime = (GtkAdjustment *) gtk_adjustment_new (1,0,1000000,1,10,0);
	widget_set_maxtime = gtk_spin_button_new(adj_set_maxtime,0,0);
	gtk_signal_connect(GTK_OBJECT(widget_set_maxtime), "value-changed",G_CALLBACK(screen_refresh),(gpointer*) &wg_data);
		
	adj_set_mintime = (GtkAdjustment *) gtk_adjustment_new(0,00,100000,1,10,0);
	widget_set_mintime = gtk_spin_button_new(adj_set_mintime,0,0);
	gtk_signal_connect(GTK_OBJECT(widget_set_mintime), "value-changed",G_CALLBACK(screen_refresh),(gpointer*) &wg_data);
		
	adj_set_average_window = (GtkAdjustment *) gtk_adjustment_new(1,1,1000000,1,10,0);
	widget_set_average_window = gtk_spin_button_new(adj_set_average_window,0,0);
	gtk_signal_connect(GTK_OBJECT(widget_set_average_window), "value-changed",G_CALLBACK(screen_refresh),(gpointer*) &wg_data);
	
	adj_set_max = (GtkAdjustment *) gtk_adjustment_new(1.0,0.0,20000000000.0,0.1,1.0,0);
	widget_set_max = gtk_spin_button_new(adj_set_max,0,3);
	gtk_signal_connect(GTK_OBJECT(widget_set_max), "value-changed",G_CALLBACK(screen_refresh),(gpointer*) &wg_data);
	
	adj_set_min = (GtkAdjustment *) gtk_adjustment_new(1.0,0.0,20000000000.0,0.1,1.0,0);
	widget_set_min = gtk_spin_button_new(adj_set_min,0,3);
	gtk_signal_connect(GTK_OBJECT(widget_set_min), "value-changed",G_CALLBACK(screen_refresh),(gpointer*) &wg_data);

	gtk_entry_set_max_length(GTK_ENTRY(widget_set_maxtime),12);
	gtk_entry_set_max_length(GTK_ENTRY(widget_set_mintime),12);
	gtk_entry_set_max_length(GTK_ENTRY(widget_set_average_window),4);;
	gtk_entry_set_max_length(GTK_ENTRY(widget_set_max),13);
	gtk_entry_set_max_length(GTK_ENTRY(widget_set_min),13);
	
	widget_max = gtk_label_new("0.000000");
	widget_min = gtk_label_new("0.000000");
	widget_maxtime = gtk_label_new("0");
	
	gtk_label_set_text(GTK_LABEL(widget_max),"0.000"); 
	gtk_label_set_text(GTK_LABEL(widget_min),"0.000"); 
	gtk_label_set_text(GTK_LABEL(widget_maxtime),"0.000");
	
	gtk_box_pack_start(GTK_BOX(vbox2), label_set_maxtime , FALSE, TRUE, 1);
	gtk_box_pack_start(GTK_BOX(vbox2), widget_set_maxtime , FALSE, TRUE, 1);
	
	gtk_box_pack_start(GTK_BOX(vbox2), label_set_mintime , FALSE, TRUE, 1);
	gtk_box_pack_start(GTK_BOX(vbox2), widget_set_mintime , FALSE, TRUE, 1);
	
	gtk_box_pack_start(GTK_BOX(vbox2), label_set_average_window , FALSE, TRUE, 1);
	gtk_box_pack_start(GTK_BOX(vbox2), widget_set_average_window , FALSE, TRUE, 1);
	
	gtk_box_pack_start(GTK_BOX(vbox2), label_set_max , FALSE, TRUE, 1);
	gtk_box_pack_start(GTK_BOX(vbox2), widget_set_max , FALSE, TRUE, 1);
	
	gtk_box_pack_start(GTK_BOX(vbox2), label_set_min , FALSE, TRUE, 1);
	gtk_box_pack_start(GTK_BOX(vbox2), widget_set_min , FALSE, TRUE, 1);

	gtk_box_pack_start(GTK_BOX(vbox2), label_max , FALSE, TRUE, 1);
	gtk_box_pack_start(GTK_BOX(vbox2), widget_max , FALSE, TRUE, 1);
	
	gtk_box_pack_start(GTK_BOX(vbox2), label_min , FALSE, TRUE, 1);
	gtk_box_pack_start(GTK_BOX(vbox2), widget_min , FALSE, TRUE, 1);
	
	gtk_box_pack_start(GTK_BOX(vbox2), label_maxtime , FALSE, TRUE, 1);
	gtk_box_pack_start(GTK_BOX(vbox2), widget_maxtime , FALSE, TRUE, 1);
		
	btn_auto = gtk_button_new_with_label("Auto scale");
	gtk_signal_connect(GTK_OBJECT(btn_auto), "clicked",G_CALLBACK (toggle_bnt_auto_callback), (gpointer*) &wg_data);

	gtk_box_pack_start(GTK_BOX(vbox2), btn_auto , FALSE, TRUE, 1);		      

	wg_data.widget_set_maxtime = widget_set_maxtime;
	wg_data.widget_set_mintime = widget_set_mintime;
	wg_data.widget_set_average_window = widget_set_average_window;
	wg_data.widget_set_max = widget_set_max;
	wg_data.widget_set_min = widget_set_min;

	wg_data.window = window;
		
	wg_data.widget_max = widget_max;
	wg_data.widget_min = widget_min;
	wg_data.widget_maxtime = widget_maxtime;
	
	wg_data.adj_set_max = adj_set_max;
	wg_data.adj_set_min = adj_set_min;
	wg_data.widget_menuitem_start = menuitem_start;
	wg_data.filesel = filesel;

	wg_data.auto_man = TRUE;
	wg_data.btn_auto = btn_auto;
	
 	ud = set_device(0,12);
	if (ud)	{
		ib_write(ud,"FREQA");
		usleep(10000);	
    }
    else
		return -1;
    
	gtk_widget_show_all(window);
	time_handler(window);  
	gtk_main();

	return 0;  	
}

void screen_refresh(GtkWidget *widget, r2101_data *data) {
	gint width,	height;						// Window width & height
	
	cairo_t *cr;
	
	cr = gdk_cairo_create(widget->window);

	gdk_drawable_get_size(widget->window, &width, &height);
			
	plot(cr, width,height,data);
	
	cairo_destroy(cr);
}

static gboolean drawing_screen(GtkWidget *widget, GdkEventExpose *event, r2101_data *data) {
	gint width,	height;						// Window width & height
	
	cairo_t *cr;
	
	cr = gdk_cairo_create(widget->window);

	gdk_drawable_get_size(widget->window, &width, &height);
			
	plot(cr, width,height, data);
	
	cairo_destroy(cr);
  return TRUE;
}

static gboolean time_handler(GtkWidget *widget)	{
	if (widget->window == NULL) return FALSE;

	gtk_widget_queue_draw(widget);
	if (handler_id == 0) return TRUE;
	read_racal2101();
	return TRUE;
}

void toogle_signal(GtkWidget *widget, gpointer window)	{
	time_t      timeStamp;
	struct tm * timeInfo;
	char        buffer[80]="";
 
	clear_string(buffer);
	
	if (handler_id == 0) {
		handler_id = g_signal_connect(G_OBJECT(window), "expose-event", 
           G_CALLBACK(drawing_screen), NULL);
		g_fprintf(stderr,"Connect handle\n\r");           
		g_array_free(racal2101_data, TRUE);
		racal2101_data = g_array_new(FALSE, FALSE, sizeof(racal2101_record));
		record_counter = 0;
	
		time( &timeStamp );
		timeInfo = localtime( &timeStamp );
 
		strftime( buffer, 60, "%Y%m%d%H%M%S-racal2101-plot.csv", timeInfo );
		output_fd = fopen(buffer, "a");
		if(output_fd == NULL){
			g_fprintf(stderr,"no file handle\n\r");           
			return;		
		}
	} 
	else {
		g_signal_handler_disconnect(window, handler_id);
		handler_id = 0;	
		g_fprintf(stderr,"Disconnect handle\n\r");		
		fclose(output_fd);
  }
}

void on_window_closed (GtkWidget *widget, r2101_data *data)	{
    gtk_main_quit();
    g_fprintf(stderr,"main_quit");
 	
 	g_array_free(racal2101_data, TRUE);
 	g_fprintf(stderr,"widget_array");
 	
 	if (output_fd != 0)
		fclose(output_fd);
	g_fprintf(stderr,"handle fd");	
	return;
}

// Callback for about
void about(GtkWidget *widget, r2101_data *data) {
	GtkWidget *dialog = gtk_about_dialog_new();
	gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(dialog), PROGRAM);
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), VERSION); 
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog), COPYRIGHT);
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog), COMMENT);
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog), URI);
	gtk_dialog_run(GTK_DIALOG (dialog));
	gtk_widget_destroy(dialog);
 }

 // File select menu
static void open_csv(GtkWidget *widget, r2101_data *data) {
	GtkWidget *filesel;
	/* Create the selector */
	filesel = gtk_file_selection_new ("Please select a file for editing.");

	data->filesel = filesel;

	gtk_signal_connect(GTK_OBJECT(filesel), "delete_event",
                     G_CALLBACK(gtk_widget_hide),&filesel);
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(filesel)->ok_button),"clicked", 
                     G_CALLBACK(file_ok_proc), data);
	gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(filesel)->cancel_button),"clicked", 
							G_CALLBACK(gtk_widget_hide),GTK_OBJECT(filesel));
	/* Display that dialog */
	gtk_widget_show (filesel);
}

void file_ok_proc(GtkWidget *widget, r2101_data *data)	{
	FILE	*fp;					// filehandle
	const	gchar	*file;			// filename
	char	buf[MAXLEN];			// line buffer
	char	 delimiter[] = ";";
	char 	*ptr;
	gdouble max_value = 0,min_value = 0,maxtime= 0;
	time_t T0;		

	file =  gtk_file_selection_get_filename( GTK_FILE_SELECTION(data->filesel));
	
	gtk_widget_hide(data->filesel);
	g_array_free(racal2101_data, TRUE);
	racal2101_data = g_array_new(FALSE, FALSE, sizeof(racal2101_record));
	record_counter = 0;

	if ( (fp = fopen(file,"r")) == NULL )
	{
		g_fprintf(stderr,"can't read file %s \n\r",file);
		return;
	}

	while ( fgets(buf,MAXLEN,fp) != NULL)
	{
			if ((buf[0] !=0xa) && (buf[0] !=0xa))	{
			ptr = strtok(buf, delimiter);
			if (ptr != NULL)
				sscanf(ptr,"%ld", &record.timestamp);
			ptr = strtok(NULL, delimiter);
			if (ptr != NULL)		
				sscanf(ptr,"%lf", &record.value );

			if (record_counter == 0){
				T0 = record.timestamp;
				max_value = min_value = record.value;
			}	
			
			record.timestamp = difftime(record.timestamp,T0);
	
			if(record.value > max_value) 
				max_value = record.value;
			else
			if(record.value < min_value)
				min_value = record.value;
#ifdef READ_CSV_DEBUG_LEVEL1
			g_fprintf(stderr,"%s: time %ld value %.6f \n\r", buf, record.timestamp, record.value );
#endif		
			g_array_append_vals(racal2101_data, &record,1);
			record_counter ++;
		}
	}
	fclose(fp);
	
	maxtime = record.timestamp;
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(data->widget_set_maxtime), maxtime);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(data->widget_set_mintime), 0.0);
	
	g_snprintf(buf,MAXLEN,"%12.0f",maxtime);
	gtk_label_set_text(GTK_LABEL(data->widget_maxtime),buf);

	if (max_value >= 1.0E9)
		g_snprintf(buf,MAXLEN,"%12.0f",max_value);
	else
	if (max_value >= 1.0E6 )
		g_snprintf(buf,MAXLEN,"%12.3f",max_value);
	else
	if (max_value >= 1.0E3)
		g_snprintf(buf,MAXLEN,"%12.6f",max_value);
	else
		
	gtk_label_set_text(GTK_LABEL(data->widget_max),buf);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(data->widget_set_max),max_value);
		
	if (min_value >= 1.0E9)
		g_snprintf(buf,MAXLEN,"%12.0f",min_value);
	else
	if (min_value >= 1.0E6)
		g_snprintf(buf,MAXLEN,"%12.3f",min_value);
	else
	if (min_value >= 1.0E3)
		g_snprintf(buf,MAXLEN,"%12.6f",min_value);

	gtk_label_set_text(GTK_LABEL(data->widget_min),buf);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(data->widget_set_min),min_value);

	return;
}
	
void toggle_bnt_auto_callback (GtkWidget *widget, r2101_data *data)	{
	if (data->auto_man == TRUE)	{
		data->auto_man = FALSE;
		gtk_button_set_label (GTK_BUTTON (data->btn_auto),"Manuell scale");
#ifdef BTN_AUTO_DEBUG_LEVEL1		
		g_fprintf(stderr,"MAN\n\r");

#endif
	}	
	else {
		data->auto_man = TRUE;
		gtk_button_set_label (GTK_BUTTON (data->btn_auto),"Auto scale");		
#ifdef BTN_AUTO_DEBUG_LEVEL1				
		g_fprintf(stderr,"AUTO\n\r");
#endif		
	}
}

