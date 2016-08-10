
#include <ui/widget-fft.h>

#include <complex.h>
#include <math.h>
#include <fftw3.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <radio/radio.h>
#include <ui/widget.h>
#include <util/memory.h>

#define DEFAULT_FFT_SIZE 1024
#define REFRESH_TIME_MS 20		// 20 ms = 50Hz

struct widget_fft {
    struct widget widget;
    struct radio *radio;
    struct sample *in_buf;
    double complex *out_buf;
    double max_buf[DEFAULT_FFT_SIZE];
    double scale;
    fftw_plan fft_plan;
    t_frequency tick_first;
    t_frequency tick_step;
    unsigned int tick_number;
    t_frequency freq_min;
    t_frequency freq_max;
    int cumulate;
};

static gint widget_fft_timeout(gpointer);
static void widget_fft_draw_event(GtkWidget *, GdkEventExpose *, gpointer);
static gboolean widget_fft_configure_event(GtkWidget *, GdkEvent *, gpointer);
static gboolean widget_fft_key_press_event(GtkWidget *, GdkEventKey *,
	gpointer);

struct widget *
widget_fft_new(struct radio *radio) {
    struct widget_fft *w = memory_alloc(sizeof *w);
    int i;

    w->widget.gtk_widget = NULL;
    w->radio = radio;
    w->in_buf = NULL;
    w->out_buf = NULL;
    w->fft_plan = NULL;

    w->scale = 1.0;

    w->tick_first = 0;
    w->tick_step = 0;

    w->cumulate = 0;

    w->widget.gtk_widget = gtk_drawing_area_new();

    if (w->widget.gtk_widget == NULL)
	goto err;

    w->in_buf = fftw_malloc(sizeof *w->in_buf * DEFAULT_FFT_SIZE);
    if (w->in_buf == NULL)
	goto err;
    w->out_buf = fftw_malloc(sizeof *w->out_buf * DEFAULT_FFT_SIZE);
    if (w->out_buf == NULL)
	goto err;

    w->fft_plan = fftw_plan_dft_1d(DEFAULT_FFT_SIZE,
	(complex double *) w->in_buf, w->out_buf, FFTW_FORWARD, FFTW_ESTIMATE);
    if (w->fft_plan == NULL)
	goto err;

    for (i = 0; i < DEFAULT_FFT_SIZE; i++)
	w->max_buf[i] = 0;

    g_signal_connect(G_OBJECT(w->widget.gtk_widget), "configure-event",
	G_CALLBACK(widget_fft_configure_event), w);
    g_signal_connect(G_OBJECT(w->widget.gtk_widget), "expose-event",
	G_CALLBACK(widget_fft_draw_event), w);
    g_signal_connect(G_OBJECT(w->widget.gtk_widget), "key-press-event",
	G_CALLBACK(widget_fft_key_press_event), w);

    gtk_widget_set_size_request(w->widget.gtk_widget, DEFAULT_FFT_SIZE, 300);
    gtk_widget_set_can_focus(w->widget.gtk_widget, TRUE);
    gtk_widget_set_sensitive(w->widget.gtk_widget, TRUE);
    gtk_widget_add_events(w->widget.gtk_widget, GDK_KEY_PRESS_MASK);
    gtk_widget_grab_focus(w->widget.gtk_widget);
    g_timeout_add(REFRESH_TIME_MS, widget_fft_timeout, w->widget.gtk_widget);

    if (0) {
err:
	if (w->fft_plan != NULL)
	    fftw_destroy_plan(w->fft_plan);
	if (w->out_buf != NULL)
	    fftw_free(w->out_buf);
	if (w->in_buf != NULL)
	    fftw_free(w->in_buf);
	if (w->widget.gtk_widget != NULL)
	    gtk_widget_destroy(w->widget.gtk_widget);
	return NULL;
    }

    return &w->widget;
}

static gint
widget_fft_timeout(gpointer aux) {
    GtkWidget *w = (GtkWidget *) aux;

    gtk_widget_queue_draw(w);

    return 1;
}

static void
widget_fft_compute_auto_scale(struct widget_fft *w) {
    double max = w->max_buf[0];
    size_t i;
    for (i = 1; i < sizeof w->max_buf / sizeof w->max_buf[0]; i++) {
	if (max < w->max_buf[i])
	    max = w->max_buf[i];
    }
    if (max == 0)
	max = 1;

    w->scale = (w->widget.gtk_widget->allocation.height - 20) / max;
}

static gboolean
widget_fft_configure_event(GtkWidget *widget, GdkEvent *event, gpointer aux) {
    struct widget_fft *w = (struct widget_fft *) aux;
    t_frequency fcenter = 0;
    unsigned long srate = 0;
    cairo_t *cr = NULL;
    cairo_text_extents_t freq_extents;
    unsigned long per_freq_room;
    unsigned long widget_width = widget->allocation.width;
    unsigned int nb_ticks;
    unsigned long prec;

    if (w->radio->m->get_frequency(w->radio, &fcenter) == -1)
	fcenter = 0;

    if (w->radio->m->get_sample_rate(w->radio, &srate) == -1)
	goto err;

    w->freq_min = fcenter - srate / 2;
    w->freq_max = fcenter + srate / 2;

    cr = gdk_cairo_create(widget->window);
    cairo_text_extents(cr, "9999999999", &freq_extents);
    per_freq_room = freq_extents.width * 2;
    if (per_freq_room == 0)
	goto err;

    nb_ticks = widget_width / per_freq_room;
    if (nb_ticks == 0)
	goto err;

    prec = pow(10, (int) round(log10(srate / nb_ticks)));
    w->tick_first = (fcenter - srate / 2 + prec - 1) / prec * prec;
    w->tick_step = prec;
    w->tick_number = (w->freq_max - w->tick_first) / w->tick_step + 1;

    if (0) {
err:
	w->tick_first = fcenter;
	w->tick_step = 0;
	w->tick_number = 1;
	w->freq_min = fcenter - srate / 2;
	w->freq_max = fcenter + srate / 2;
    }

    if (cr != NULL)
	cairo_destroy(cr);
    return 0;
}

static void
widget_fft_draw_freq_tick(struct widget_fft *w, cairo_t *cr,
	unsigned int bottom, t_frequency f) {
    unsigned int width = w->widget.gtk_widget->allocation.width;
    cairo_text_extents_t extent;
    char *text = frequency_human_print(f);
    int x;
    int i;

    for (i = 0; text[i] != '\0' && text[i] != 'H'; i++)
	;
    text[i] = '\0';

    cairo_text_extents(cr, text, &extent);

    if (w->freq_max == w->freq_min)
	x = width / 2;
    else
	x = (f - w->freq_min) * width / (w->freq_max - w->freq_min);
    cairo_move_to(cr, x, bottom);
    cairo_line_to(cr, x, bottom + 4);
    cairo_move_to(cr, x - extent.width / 2, bottom + 6 + extent.height);
    cairo_show_text(cr, text);

    memory_free(text);
}

static int
widget_fft_read_data(struct radio *r, struct sample *in_buf, size_t size) {
    size_t to_read = size;
    double ignored_size_d;
    size_t ignored_size;
    size_t to_ignore;
    unsigned long srate;
    double size_in_ms;
    ssize_t ret;

    if (r->m->get_sample_rate(r, &srate) == -1)
	return -1;

    size_in_ms = size * 1000. / srate;
    ignored_size_d = (REFRESH_TIME_MS - size_in_ms) * srate / 1000.;
    ignored_size = ignored_size_d > 0? ignored_size_d : 0;

    for (to_ignore = ignored_size; to_ignore != 0; to_ignore -= ret) {
	/*
	 * Ignore read() return value because some radios (rtlsdr at
	 * least) refuse short reads.
	 */
	r->m->read(r, in_buf, to_ignore > size? size : to_ignore);
    }

    for (to_read = size; to_read != 0; to_read -= ret) {
	ret = r->m->read(r, in_buf + size - to_read, to_read);
	if (ret == -1)
	    return -1;
    }
    
    return 0;
}

static void
widget_fft_draw_event(GtkWidget *widget, GdkEventExpose *event, gpointer aux) {
    struct widget_fft *w = (struct widget_fft *) aux;
    cairo_t *cr = NULL;
    unsigned int x;
    unsigned int height = widget->allocation.height;
    cairo_text_extents_t extent;
    unsigned int bottom;

    if (widget_fft_read_data(w->radio, w->in_buf, DEFAULT_FFT_SIZE) == -1) {
	printf("EOF or error\n");
	goto err;
    }

    fftw_execute(w->fft_plan);

    if (!w->cumulate)
	w->out_buf[0] = 0;

    cr = gdk_cairo_create(widget->window);
    cairo_set_line_width(cr, 0.5);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_text_extents(cr, ".0123456789KMG", &extent);
    bottom = height - extent.height * 2 - 10 + 0.5;
    cairo_move_to(cr, 0, bottom);
    cairo_line_to(cr, widget->allocation.width, bottom);

    for (unsigned int i = 0; i < w->tick_number; i++)
	widget_fft_draw_freq_tick(w, cr, bottom,
		w->tick_first + i * w->tick_step);

    cairo_stroke(cr);

    cairo_set_line_width(cr, 0.5);
    cairo_set_source_rgb(cr, 0, 0, 1);

    for (x = 0; x < DEFAULT_FFT_SIZE; x++) {
	int i = (x + (DEFAULT_FFT_SIZE / 2)) % DEFAULT_FFT_SIZE;
	double new_y = cabs(w->out_buf[i]);

	if (w->cumulate)
	    w->max_buf[i] += new_y;
	else if (new_y > w->max_buf[i])
	    w->max_buf[i] = new_y;

	new_y *= w->scale;

	if (x == 0)
	    cairo_move_to(cr, x, bottom - new_y - 1);
	else
	    cairo_line_to(cr, x, bottom - new_y - 1);
    }

    if (w->cumulate) {
	double min = w->max_buf[0];
	for (x = 0; x < DEFAULT_FFT_SIZE; x++) {
	    int i = (x + (DEFAULT_FFT_SIZE / 2)) % DEFAULT_FFT_SIZE;
	    if (w->max_buf[i] < min)
		min = w->max_buf[i];
	}
	for (x = 0; x < DEFAULT_FFT_SIZE; x++) {
	    int i = (x + (DEFAULT_FFT_SIZE / 2)) % DEFAULT_FFT_SIZE;
	    w->max_buf[i] -= min;
	}
    }

    cairo_stroke(cr);

    cairo_set_source_rgb(cr, 1, 0, 0);

    for (x = 0; x < DEFAULT_FFT_SIZE; x++) {
	int i = (x + (DEFAULT_FFT_SIZE / 2)) % DEFAULT_FFT_SIZE;
	if (x == 0)
	    cairo_move_to(cr, x, bottom - w->max_buf[i] * w->scale - 1);
	else
	    cairo_line_to(cr, x, bottom - w->max_buf[i] * w->scale - 1);
    }

    cairo_stroke(cr);
err:
    if (cr != NULL)
	cairo_destroy(cr);
}

static gboolean
widget_fft_key_press_event(GtkWidget *widget, GdkEventKey *event,
							gpointer aux) {
    struct widget_fft *w = (struct widget_fft *) aux;
    t_frequency fcenter;
    unsigned long srate;
    long offset;
    int need_refresh = 1;
    int i;

    switch (event->keyval) {
    case GDK_KEY_a:
	widget_fft_compute_auto_scale(w);
	need_refresh = 0;
	break;

    case GDK_KEY_Left: case GDK_KEY_Right:
	if (w->radio->m->get_frequency(w->radio, &fcenter) == -1)
	    goto err;

	if (w->radio->m->get_sample_rate(w->radio, &srate) == -1)
	    goto err;

	offset = srate / 4;

	if (event->keyval == GDK_KEY_Left)
	    offset = -offset;

	w->radio->m->set_frequency(w->radio, fcenter + offset);
	break;

    case GDK_KEY_r:
	break;

    case GDK_KEY_c:
	w->cumulate = !w->cumulate;
	break;

    default:
	need_refresh = 0;
	break;
    }

    if (need_refresh) {
	for (i = 0; i < DEFAULT_FFT_SIZE; i++)
	    w->max_buf[i] = 0;

	gtk_widget_queue_resize(w->widget.gtk_widget);
    }

err:
    return 0;
}
