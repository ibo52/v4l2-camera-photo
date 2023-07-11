#include<gtk/gtk.h>
#include <sys/types.h>
#include <unistd.h>//sleep
#include <sys/mman.h>
#include <semaphore.h>
#include "../../include/cam.h"

GtkWidget 		*window;
GtkWidget 		*rootLayout;
GtkWidget 		*captureButton;
GtkWidget 		*imageBoxLayout;
GtkWidget 		*imageBox;
GtkBuilder		*builder;
GtkWidget 		*deviceInfo_textbuffer;

gpointer updateImageThread(gpointer);
gboolean updateImage(gpointer);

//https://stackoverflow.com/questions/60949269/executing-gtk-functions-from-other-threads
static GMutex camera_access_mutex; //shared between display and capture button

int main(int argc, char **argv){

	activate();
	
	gtk_init(&argc, &argv);//init gtk
	
	builder=gtk_builder_new_from_file("halocam.glade");
	
	window=GTK_WIDGET(gtk_builder_get_object(builder,"window"));
	rootLayout=GTK_WIDGET(gtk_builder_get_object(builder,"rootLayout"));
	imageBoxLayout=GTK_WIDGET(gtk_builder_get_object(builder,"imageBoxLayout"));
	imageBox=GTK_WIDGET(gtk_builder_get_object(builder,"imageBox"));
	captureButton=GTK_WIDGET(gtk_builder_get_object(builder,"captureButton"));
	deviceInfo_textbuffer=GTK_WIDGET(gtk_builder_get_object(builder,"deviceInfo_textbuffer"));
	
	gtk_text_buffer_set_text(GTK_TEXT_BUFFER(deviceInfo_textbuffer),device_specs.text,device_specs.length);
	
	g_signal_connect(window,"destroy",G_CALLBACK(gtk_main_quit),NULL);
	
	gtk_builder_connect_signals(builder,NULL);
	
	gtk_widget_show(window);
	
	g_thread_new("updateImageThread",updateImageThread,NULL); //thread to update display
	gtk_main();
	
	return 0;
}

gpointer updateImageThread(gpointer args){

	while(1){
		g_print("\n");//not using this causes unexpected behaviour. Interesting :D
		g_mutex_lock (&camera_access_mutex);
		

		uint8_t* buff=get_RGB_buff();
		g_idle_add ( G_SOURCE_FUNC(updateImage), buff);
		free(buff);
		
		g_mutex_unlock (&camera_access_mutex);
	}
	g_thread_exit(NULL);
}

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
		/*
		pixbuff=gdk_pixbuf_scale_simple (
			  pixbuff,
			  gtk_widget_get_allocated_width(imageBoxLayout),
			  gtk_widget_get_allocated_height(imageBoxLayout),
			  GDK_INTERP_BILINEAR
		);
		*/
		//3. set data to GtkImage object
		gtk_image_set_from_pixbuf(GTK_IMAGE(imageBox), pixbuff);//set method changes imageBox ton new image  and stimulates widget
		
		g_usleep(16667);//60 fps approx
		}
		return G_SOURCE_REMOVE;
}
gboolean captureImage(gpointer data){

	g_mutex_lock (&camera_access_mutex);
	
	char *text=data;
	dump_buffer_to_file(text);
	
	g_mutex_unlock (&camera_access_mutex);
	
	return G_SOURCE_REMOVE;
}
void captureButtonClicked(GtkButton *b){
	//dumps camera buffer that comes from cam.h to file
	//g_mutex_lock (&camera_access_mutex);
	//g_usleep(3*G_USEC_PER_SEC);
	g_idle_add_full(
	G_PRIORITY_HIGH_IDLE,//function priority
	captureImage,
	"image_shot",
	NULL);//https://docs.gtk.org/glib/func.idle_add_full.html
	//dump_buffer_to_file("image_shot");
	
	//g_mutex_unlock (&camera_access_mutex);
	/*
	uint8_t* buff=get_RGB_buff();
	save2jpeg(buff,1280*720*3,1280,720,"deneme",80);
	free(buff);
	*/
}
