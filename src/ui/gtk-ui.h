#ifndef GTK_GUI_H_
#define GTK_GUI_H_

struct widget;

int gtk_gui_setup(int *, char ***);
void gtk_gui_add_widget(struct widget *);
void gtk_gui_run_main(void);

#endif /* GTK_GUI_H_ */
