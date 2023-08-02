/*
*	MAIN METHOD TO RUN APPLICATION
*/
#include<gtk/gtk.h>
#include "cam.h"
#include "halocam__window.h"

int main(int argc, char **argv){
	camera__activate(); 		//initialize camera

	GtkApplication *app;
    int stat;

    app = gtk_application_new ("org.halosoft.halocam", G_APPLICATION_HANDLES_OPEN);
    //g_signal_connect (app, "startup", G_CALLBACK (castart), NULL);
    g_signal_connect (app, "activate", G_CALLBACK (app_activate), NULL);
    stat = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);
    
	return stat;
}