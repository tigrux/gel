#include <string.h>
#include <math.h>
#include <gtk/gtk.h>
#include <gel.h>


typedef struct
{
    GObject *window;
    GObject *drawingarea;
    GObject *textview;
    GObject *label;
    GelParser *parser;
    GelContext *context;
    cairo_t *cairo;
} App;


void color_(GClosure *closure, GValue *return_value,
            guint n_values, const GValue *values,
            GelContext *context, App *self)
{
    GList *tmp_list = NULL;
    gdouble r,g,b;
    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "FFF", &r, &g, &b))
    {
        cairo_set_source_rgb(self->cairo, r/100, g/100, b/100);
    }
    gel_list_free(tmp_list);
}

void move_(GClosure *closure, GValue *return_value,
           guint n_values, const GValue *values,
           GelContext *context, App *self)
{
    GList *tmp_list = NULL;
    gdouble x,y;
    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "FF", &x, &y))
    {
        cairo_move_to(self->cairo, x, y);
        cairo_translate(self->cairo, x, y);
    }
    gel_list_free(tmp_list);
}


void line_(GClosure *closure, GValue *return_value,
           guint n_values, const GValue *values,
           GelContext *context, App *self)
{
    GList *tmp_list = NULL;
    gdouble d;
    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "F", &d))
    {
        cairo_line_to(self->cairo, d, 0);
        cairo_stroke(self->cairo);
        cairo_translate(self->cairo, d, 0);
        cairo_move_to(self->cairo, 0, 0);
    }
    gel_list_free(tmp_list);
}


void turn_(GClosure *closure, GValue *return_value,
           guint n_values, const GValue *values,
           GelContext *context, App *self)
{
    GList *tmp_list = NULL;
    gdouble a;
    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "F", &a))
    {
        cairo_rotate(self->cairo, a*M_PI/180);
    }
    gel_list_free(tmp_list);
}


gboolean on_draw(GtkWidget *widget, cairo_t *cairo, App *self)
{
    GtkTextBuffer *textbuffer = NULL;
    g_object_get(self->textview, "buffer", &textbuffer, NULL);

    gchar *text = NULL;
    g_object_get(textbuffer, "text", &text, NULL);
    gel_parser_input_text(self->parser, text, strlen(text));

    g_object_set(self->label, "label", "", NULL);
    self->cairo = cairo;

    GelContext *draw_context = gel_context_new_with_outer(self->context);

    GelParserIter iter;
    gel_parser_iter_init(&iter, self->parser);
    GError *error = NULL;
    while(gel_parser_iter_next(&iter, &error))
    {
        GValue *parsed_value = gel_parser_iter_get(&iter);
        GValue tmp_value = {0};
        gel_context_eval(draw_context, parsed_value, &tmp_value, &error);
        if(error != NULL)
            break;
        if(G_IS_VALUE(&tmp_value))
            g_value_unset(&tmp_value);
    }

    if(error != NULL)
    {
        g_object_set(self->label, "label", error->message, NULL);
        g_error_free(error);
    }

    gel_context_free(draw_context);
    return TRUE;
}


int main(int argc, gchar *argv[])
{
    gtk_init(&argc, &argv);

    GtkBuilder *builder = gtk_builder_new();
    GError *error = NULL;
    if(gtk_builder_add_from_file(builder, "gel-draw.ui", &error) == 0)
    {
        g_warning("%s\n", error->message);
        g_error_free(error);
        g_object_unref(builder);
        return 1;
    }

    App self =
    {
        .window = gtk_builder_get_object(builder, "window"),
        .drawingarea = gtk_builder_get_object(builder, "drawingarea"),
        .textview = gtk_builder_get_object(builder, "textview"),
        .label = gtk_builder_get_object(builder, "label"),
        .parser = gel_parser_new(),
        .context = gel_context_new()
    };

    gel_context_define_object(self.context, "window", self.window);
    gel_context_define_object(self.context, "drawingarea", self.drawingarea);
    gel_context_define_object(self.context, "textview", self.textview);
    gel_context_define_object(self.context, "label", self.label);

    gel_context_define_function(self.context, "color", (GelFunction)color_, &self);
    gel_context_define_function(self.context, "move", (GelFunction)move_, &self);
    gel_context_define_function(self.context, "line", (GelFunction)line_, &self);
    gel_context_define_function(self.context, "turn", (GelFunction)turn_, &self);

    gtk_widget_show_all(GTK_WIDGET(self.window));
    gtk_builder_connect_signals(builder, &self);

    gtk_main();

    gel_parser_free(self.parser);
    gel_context_free(self.context);

    return 0;
}

