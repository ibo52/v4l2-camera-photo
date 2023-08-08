#include<gtk/gtk.h>
#include<stdint.h>
#include <sys/ioctl.h>
#include "cam.h"

GtkBuilder		*cameraSettingsDialog__builder;
GtkWidget		*cameraSettingsDialog__display_window;
GtkWidget		*rootBox;

#define CLEAR(x) memset(&(x), 0, sizeof(x)) //write zero to the struct space

void value_changed_cb(GtkRange* self, gpointer ctrl_id){
	
	//forward control id and value to camera
	camera__control__set((uintptr_t)ctrl_id ,(int)gtk_range_get_value(self) );
}

static void cameraSettingsDialog__append_controls(){
	/*
	*	Check and append all supported controls to menu
	*/
	PangoAttrList *Attrs = pango_attr_list_new();//text formatting
	pango_attr_list_insert(Attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
	pango_attr_list_insert(Attrs, pango_attr_foreground_new(0,16384,32768));
	
	CLEAR(queryctrl);
	queryctrl.id = V4L2_CID_BASE;//V4L2_CID_USER_CLASS;//V4L2_CID_BASE;
	
	while (0 == camera__control__get_ctrl() ) {

			if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
				continue; //if control not supported
			
			//box properties
			GtkWidget* box=gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
			gtk_box_set_homogeneous(GTK_BOX(box), 1);
			
			//label properties
			GtkWidget* label=gtk_label_new( (char *)queryctrl.name );
			gtk_label_set_attributes(GTK_LABEL(label),Attrs);
			gtk_widget_set_halign(label, GTK_ALIGN_START);			
			gtk_widget_set_margin_start(label, 5);
			
			//if reached V4L2_CID_CAMERA_CLASS or V4L2_CID_USER_CLASS do not add slider.
			//These are not controls, but explanation strings for next control segments
			if( ((queryctrl.id^V4L2_CTRL_FLAG_NEXT_CTRL)==V4L2_CID_CAMERA_CLASS) | ((queryctrl.id^V4L2_CTRL_FLAG_NEXT_CTRL)==V4L2_CID_USER_CLASS) ){
				gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
				
				gtk_box_pack_start(GTK_BOX(box), label, 1,1,5);
				gtk_box_pack_start(GTK_BOX(rootBox), box, 1,1,5);
				gtk_widget_show(label);	
				gtk_widget_show(box);
				pango_attr_list_unref(Attrs);
				continue;
			}
			//slider properties
			GtkWidget* slider;
			if(queryctrl.type == V4L2_CTRL_TYPE_MENU){ //if it has menu type controls
			
				slider=gtk_combo_box_text_new();
				queryctrl.id=(queryctrl.id^V4L2_CTRL_FLAG_NEXT_CTRL);//reverse effect of flag V4L2_CTRL_FLAG_NEXT_CTRL
				
				for (querymenu.index = queryctrl.minimum; querymenu.index <= queryctrl.maximum; querymenu.index++) {
         			
         			camera__control__enumerate_menu();
         			
         			char temp[20];
         			sprintf(temp, "%i",querymenu.index);
         			
         			gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(slider), temp, (char*)querymenu.name);
         			
				}
				gtk_combo_box_set_active(GTK_COMBO_BOX(slider), queryctrl.default_value);
				queryctrl.id=(queryctrl.id|V4L2_CTRL_FLAG_NEXT_CTRL);//de-reverse effect of flag V4L2_CTRL_FLAG_NEXT_CTRL;	
			}
			else{
			slider=gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,
												queryctrl.minimum, queryctrl.maximum, 1);				
			gtk_range_set_value(GTK_RANGE(slider), queryctrl.default_value);
			gtk_scale_add_mark(GTK_SCALE(slider), (float)queryctrl.default_value, GTK_POS_BOTTOM, "|");
			gtk_widget_set_halign(slider, GTK_ALIGN_FILL);
			gtk_widget_set_margin_end(slider, 50);
			}
			//pack and show widgets
			gtk_box_pack_start(GTK_BOX(box), label, 1,1,0);
			gtk_box_pack_start(GTK_BOX(box), slider, 1,1,0);
			gtk_box_pack_start(GTK_BOX(rootBox), box, 1,1,5);
			
			gtk_widget_show(label);
			gtk_widget_show(slider);
			gtk_widget_show(box);
			
			if (queryctrl.flags & V4L2_CTRL_FLAG_INACTIVE)
				gtk_widget_set_sensitive(slider, 0);//disable if flag set
				
			else if( GTK_IS_SCALE(slider))
				g_signal_connect(slider, "value-changed", G_CALLBACK(value_changed_cb),(void*)(uintptr_t)(queryctrl.id^V4L2_CTRL_FLAG_NEXT_CTRL));//XOR to get control id
			
			pango_attr_list_unref(Attrs);

	}
}

int cameraSettingsDialog__open_display_window(){

	if (cameraSettingsDialog__display_window!=NULL) {

		gtk_window_close (GTK_WINDOW(cameraSettingsDialog__display_window));
	}
		
		cameraSettingsDialog__builder=gtk_builder_new_from_file("../resources/ui/camera_settings.glade");
	
		cameraSettingsDialog__display_window=GTK_WIDGET(gtk_builder_get_object(cameraSettingsDialog__builder,"cameraSettingsDialog"));	
		rootBox=GTK_WIDGET(gtk_builder_get_object(cameraSettingsDialog__builder,"rootBox"));	
		
		g_signal_connect(cameraSettingsDialog__display_window,"destroy",G_CALLBACK(gtk_main_quit),NULL);
		
		cameraSettingsDialog__append_controls();
		
		gtk_widget_show_all (cameraSettingsDialog__display_window);
		
	return 0;
}
