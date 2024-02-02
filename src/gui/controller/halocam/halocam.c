/*
*	MAIN METHOD TO RUN APPLICATION
*/
#include<gtk/gtk.h>
#include "Camera.h"
#include "halocam__window.h"

Camera* CameraDevice;		//global defined object, accessible by all

gpointer app_shutdown_cb(){

	camera__destroy(&CameraDevice);	//free the object

	return 0;
}

int main(int argc, char **argv){

	CameraDevice=camera__new("/dev/video0");
	camera__activate(CameraDevice); 		//initialize camera

	GtkApplication *app;
    int stat;

    app = gtk_application_new ("org.halosoft.halocam", G_APPLICATION_HANDLES_OPEN);
    //g_signal_connect (app, "startup", G_CALLBACK (castart), NULL);
    g_signal_connect (app, "activate", G_CALLBACK (app_activate), CameraDevice);
    g_signal_connect (app, "shutdown", G_CALLBACK (app_shutdown_cb), NULL);
    stat = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);
    
	return stat;
}
