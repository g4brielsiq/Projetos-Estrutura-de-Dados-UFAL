#include <gtk/gtk.h>

int main(int argc, char *argv[]) {
    GtkWidget *janela;
    GtkWidget *grid;
    GtkWidget *content;
    GtkWidget *button1, *button2;

    gtk_init(&argc, &argv);

    janela = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(janela), "Biblioteca Virtual");
    gtk_window_set_default_size(GTK_WINDOW(janela), 1280, 720);
    gtk_window_set_position(GTK_WINDOW(janela), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(janela), FALSE);

    GtkWidget *fixed = gtk_fixed_new();
    gtk_container_add(GTK_CONTAINER(janela), fixed);

    GtkWidget *logo = gtk_image_new_from_file("assets/image/logo.png");
    gtk_fixed_put(GTK_FIXED(fixed), logo, 0, 0);

    GtkWidget *menuBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(menuBox, 245, 544);
    gtk_fixed_put(GTK_FIXED(fixed), menuBox, 0, 176);

    GtkWidget *contentbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(contentbox, 1035, 720);
    gtk_fixed_put(GTK_FIXED(fixed), contentbox, 245, 0);

    GtkWidget *contentScrollArea = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(contentScrollArea), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_set_border_width(GTK_CONTAINER(contentScrollArea), 10);
    gtk_box_pack_start(GTK_BOX(contentbox), contentScrollArea, TRUE, TRUE, 0);

    GtkWidget *contentboxScrollArea = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(contentboxScrollArea, 1035, 720);
    gtk_container_add(GTK_CONTAINER(contentScrollArea), contentboxScrollArea);

    GtkWidget *label = gtk_label_new("This is my label");
    GtkWidget *label2 = gtk_label_new("This is my label");
    GtkWidget *label3 = gtk_label_new("This is my label");
    GtkWidget *label4 = gtk_label_new("This is my label");

    gtk_box_pack_start(GTK_BOX(contentboxScrollArea), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(contentboxScrollArea), label2, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(contentboxScrollArea), label3, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(contentboxScrollArea), label4, FALSE, FALSE, 0);

    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css,
    "#menu { background-color: #72818F; box-shadow: inset 0px 4px 10px rgba(0, 0, 0, 0.4);}"
    "#contentboxScrollArea > * { background-color: #695dff; box-shadow: inset 0px 4px 10px rgba(0, 0, 0, 0.4); margin: 20px;}"
    "#content { background-color: #d9d9d9; box-shadow: inset 0px 4px 10px rgba(0, 0, 0, 0.4);}"
    , -1, NULL);

    gtk_widget_set_name(menuBox, "menu");
    gtk_widget_set_name(contentbox, "content");
    gtk_widget_set_name(contentboxScrollArea, "contentboxScrollArea");

    GtkStyleContext *ctxMenu = gtk_widget_get_style_context(menuBox);
    gtk_style_context_add_provider(ctxMenu, GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_USER);
    GtkStyleContext *ctxConent = gtk_widget_get_style_context(contentbox);
    gtk_style_context_add_provider(ctxConent, GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_USER);
    GtkStyleContext *ctxContentboxScrollArea = gtk_widget_get_style_context(contentboxScrollArea);
    gtk_style_context_add_provider(ctxContentboxScrollArea, GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_USER);


    g_signal_connect(janela, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all(janela);
    gtk_main();

    return 0;
}
