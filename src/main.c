#include <gtk/gtk.h>
#include <libsecret/secret.h>

// Function declarations for button actions
void on_add_key_button_clicked(GtkButton *button, gpointer user_data);
void on_remove_key_button_clicked(GtkButton *button, gpointer user_data);

int main(int argc, char *argv[]) {
    GtkApplication *app = gtk_application_new("com.nostr.signer", G_APPLICATION_FLAGS_NONE);

    // Initialize actions
    g_signal_connect(app, "startup", G_CALLBACK(create_actions), NULL);
    g_signal_connect(app, "startup", G_CALLBACK(create_permission_actions), NULL);

    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);

    return g_application_run(G_APPLICATION(app), argc, argv);
}

void on_activate(GtkApplication *app, gpointer user_data) {
    GtkBuilder *builder = gtk_builder_new_from_file("nostr_signer.ui");
    GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));

    gtk_window_set_application(GTK_WINDOW(window), app);
    gtk_widget_show(window);
}
