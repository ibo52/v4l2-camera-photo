#include<gtk/gtk.h>
#include <sys/types.h>
#include <unistd.h>//sleep
//#include <string.h>
#include "cam.h"
#include "gallery.h"
#include "filters.h"
#include "about-dialog.h"
#include "halocam__window.h"

//---CAMERA PAGE WİDGETS---
GtkWidget 		*window;
GtkWidget 		*rootLayout;	//MAİN LAYOUT OF WİNDOW
GtkWidget 		*captureButton;
GtkWidget 		*imageBoxLayout;
GtkWidget 		*imageBox;
GtkBuilder		*builder;
GtkWidget		*galleryFlowBox;

//---INFO PAGE WİDGETS---
GtkWidget 		*info__driver_label;
GtkWidget 		*info__card_label;
GtkWidget 		*info__bus_label;
GtkWidget 		*info__version_label;
GtkWidget 		*info__caps_label;

GtkWidget 		*info__buff_width;
GtkWidget 		*info__buff_height;
GtkWidget 		*info__buff_format;
GtkWidget		*info__device_path;
//---INFO PAGE WİDGETS---


int halocam__info_labels__activate(GtkBuilder		*builder){
	info__driver_label=GTK_WIDGET(gtk_builder_get_object(builder,"info__driver_label"));
	info__card_label=GTK_WIDGET(gtk_builder_get_object(builder,"info__card_label"));
	info__bus_label=GTK_WIDGET(gtk_builder_get_object(builder,"info__bus_label"));
	info__version_label=GTK_WIDGET(gtk_builder_get_object(builder,"info__version_label"));
	info__caps_label=GTK_WIDGET(gtk_builder_get_object(builder,"info__caps_label"));
	info__device_path=GTK_WIDGET(gtk_builder_get_object(builder,"info__device_path"));
	
	info__buff_width=GTK_WIDGET(gtk_builder_get_object(builder,"info__buff_width"));
	info__buff_height=GTK_WIDGET(gtk_builder_get_object(builder,"info__buff_height"));
	info__buff_format=GTK_WIDGET(gtk_builder_get_object(builder,"info__buff_format"));
	
	
	char temp[512]={'0'};
	gtk_label_set_text(GTK_LABEL(info__device_path), dev_name);
	
	sprintf(temp, "%s", caps.driver);
	gtk_label_set_text(GTK_LABEL(info__driver_label), temp);
	
	sprintf(temp, "%s", caps.card);
	gtk_label_set_text(GTK_LABEL(info__card_label), temp);
	
	sprintf(temp, "%s", caps.bus_info);
	gtk_label_set_text(GTK_LABEL(info__bus_label), temp);
	
	sprintf(temp, "%d.%d",(caps.version>>16)&&0xff, (caps.version>>24)&&0xff);
	gtk_label_set_text(GTK_LABEL(info__version_label), temp);
	
	sprintf(temp, "%08x", caps.capabilities);
	gtk_label_set_text(GTK_LABEL(info__caps_label), temp);
	//------------------------------
	sprintf(temp, "%d", fmt.fmt.pix.width);
	gtk_label_set_text(GTK_LABEL(info__buff_width), temp);
	
	sprintf(temp, "%d", fmt.fmt.pix.height);
	gtk_label_set_text(GTK_LABEL(info__buff_height), temp);
	
	sprintf(temp, "%c%c%c%c", fmt.fmt.pix.pixelformat&0xff,  (fmt.fmt.pix.pixelformat>>8)&0xff,  (fmt.fmt.pix.pixelformat>>16)&0xff,  (fmt.fmt.pix.pixelformat>>24)&0xff);
	gtk_label_set_text(GTK_LABEL(info__buff_format), temp);
	
	return 0;
}


/*
*	CONTROLLER SIGNALS
*/
//https://stackoverflow.com/questions/60949269/executing-gtk-functions-from-other-threads
static GMutex camera_access_mutex; //shared between display and capture button
/*
*
*
*/
gboolean updateImage(gpointer buffer){

	if (imageBox!=NULL || imageBoxLayout!=NULL){
		
		//1. move data to gdkpixbuff struct
		GdkPixbuf* pixbuff=gdk_pixbuf_new_from_data (
			  (uint8_t*)buffer,
			  GDK_COLORSPACE_RGB,
			  0,
			  8,//int bits_per_sample=
			  fmt.fmt.pix.width,
			  fmt.fmt.pix.height,
			  fmt.fmt.pix.width*3,
			  NULL,//GdkPixbufDestroyNotify destroy_fn=
			  NULL//gpointer destroy_fn_data
		);
		
		//2. scale gdkpixbuff iamge struct .ERROR-->Leads buffer overflow. Could not manage the struct
		GdkPixbuf* pixbuff_scaled=gdk_pixbuf_scale_simple (
			  pixbuff,
			  gtk_widget_get_allocated_width(imageBoxLayout),
			  gtk_widget_get_allocated_height(imageBoxLayout),
			  GDK_INTERP_BILINEAR
		);

		//3. set data to GtkImage object
		gtk_image_set_from_pixbuf(GTK_IMAGE(imageBox), pixbuff_scaled);//set method changes imageBox ton new image  and stimulates widget
		g_object_unref( pixbuff_scaled );  //free pixbuff memory

		}
		return G_SOURCE_REMOVE;
}

gboolean updateImageThread(gpointer args){
		
		g_mutex_lock (&camera_access_mutex);

		uint8_t* buff=get_RGB_buff();

		updateImage( buff);
		free(buff);
		
		g_mutex_unlock (&camera_access_mutex);
		
	return G_SOURCE_REMOVE;
}

gboolean captureImage(gpointer data){

	g_mutex_lock (&camera_access_mutex);
	
	char *text=data;
	text=dump_buffer_to_file(text);//get saved image name
	
	gallery__load_image(galleryFlowBox, text, 176, 176, 1);//adds image to galleryFlowBox
	
	g_mutex_unlock (&camera_access_mutex);
	
	return G_SOURCE_REMOVE;
}
void captureButtonClicked(GtkButton *b){
	//dumps camera buffer that comes from cam.h to file

	g_idle_add_full(
	G_PRIORITY_HIGH_IDLE,//function priority
	captureImage,
	"image_shot",
	NULL);//https://docs.gtk.org/glib/func.idle_add_full.html
	
}

void ShowAboutWindowButton_activate_cb(GtkMenuItem *item){
	aboutDialog__open_display_window();g_print("disp oopened\n");
}

void app_activate (GApplication *app, gpointer user_data) {
	
	//gtk_init(&argc, &argv);//init gtk
	
	builder=gtk_builder_new_from_file("../resources/ui/halocam.glade");
	
	window=GTK_WIDGET(gtk_builder_get_object(builder,"window"));
	rootLayout=GTK_WIDGET(gtk_builder_get_object(builder,"rootLayout"));
	imageBoxLayout=GTK_WIDGET(gtk_builder_get_object(builder,"imageBoxLayout"));
	imageBox=GTK_WIDGET(gtk_builder_get_object(builder,"imageBox"));
	captureButton=GTK_WIDGET(gtk_builder_get_object(builder,"captureButton"));

	halocam__info_labels__activate(builder);//set labels for info page
	
	galleryFlowBox=GTK_WIDGET(gtk_builder_get_object(builder,"galleryFlowBox"));
	
	g_signal_connect(window,"destroy",G_CALLBACK(gtk_main_quit),NULL);

	g_signal_connect(imageBox,"draw",G_CALLBACK(updateImageThread),NULL);
	gtk_builder_connect_signals(builder,NULL);
	//---
	gallery__load_all_images(galleryFlowBox, "../images/");
	gtk_window_set_application (GTK_WINDOW (window), GTK_APPLICATION (app));
	g_object_unref(builder);
	//---
	gtk_widget_show(window);
}
