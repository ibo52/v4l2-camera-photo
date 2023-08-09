#include<gtk/gtk.h>
#include<stdint.h>

GtkBuilder		*aboutDialog__builder;
GtkWidget		*aboutDialog__display_window;

int aboutDialog__open_display_window(){

	if (aboutDialog__display_window!=NULL) {

		gtk_window_close (GTK_WINDOW(aboutDialog__display_window));
	}
		
		aboutDialog__builder=gtk_builder_new_from_file("../resources/ui/about-dialog.glade");
	
		aboutDialog__display_window=GTK_WIDGET(gtk_builder_get_object(aboutDialog__builder,"window__aboutDialog"));	
		
		g_signal_connect(aboutDialog__display_window,"destroy",G_CALLBACK(gtk_window_close),NULL);
	
		gtk_widget_show_all (aboutDialog__display_window);
		
	return 0;
}
