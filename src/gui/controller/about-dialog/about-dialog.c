#include<gtk/gtk.h>
#include<stdint.h>

typedef struct __about_dialog{
	GtkWidget		*window;
}View;

static View 		aboutDialog;
static GtkBuilder	*builder;

/*static function that opens window about application*/
int aboutDialog__open_display_window(void){

	if (aboutDialog.window!=NULL) {

		gtk_window_close (GTK_WINDOW(aboutDialog.window));
	}
		
		builder=gtk_builder_new_from_file("../resources/view/about-dialog.glade");
	
		aboutDialog.window=GTK_WIDGET(gtk_builder_get_object(builder,"window__aboutDialog"));	
		
		g_signal_connect(aboutDialog.window,"destroy",G_CALLBACK(gtk_window_close),NULL);
	
		gtk_widget_show_all (aboutDialog.window);
		
	return 0;
}
