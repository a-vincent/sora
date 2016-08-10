
#include <ui/gtk-ui.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <ui/widget.h>

GtkWidget *gtk_gui_main_window;

static gboolean gtk_gui_key_press_event(GtkWidget *, GdkEventKey *, gpointer);

int
gtk_gui_setup(int *argcp, char ***argvp) {

    gtk_set_locale();
    gtk_init(argcp, argvp);
    gtk_gui_main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    if (gtk_gui_main_window == NULL)
	return -1;

    g_signal_connect(G_OBJECT(gtk_gui_main_window), "key-press-event",
	G_CALLBACK(gtk_gui_key_press_event), NULL);

    return 0;
}

void
gtk_gui_run_main(void) {
    gtk_widget_show_all(gtk_gui_main_window);

    gtk_main();
}

void
gtk_gui_add_widget(struct widget *widget) {
    gtk_container_add(GTK_CONTAINER(gtk_gui_main_window), widget->gtk_widget);
}

static gboolean
gtk_gui_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer aux) {
    if (event->keyval == GDK_KEY_q)
	gtk_main_quit();

    return 0;
}
