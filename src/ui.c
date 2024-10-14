#include <gtk/gtk.h>
#include <gio/gio.h>
#include <libsecret/secret.h>

#define SCHEMA secret_schema_new("com.gnostr.Signer", SECRET_SCHEMA_NONE, "user", SECRET_SCHEMA_ATTRIBUTE_STRING, NULL)

// Callback for the Add Key action
static void add_key_action(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK_CANCEL, "Enter the private key:");
    gtk_dialog_run(GTK_DIALOG(dialog));

    // Retrieve the entered key and store it
    const gchar *private_key = gtk_entry_get_text(GTK_ENTRY(dialog));
    secret_password_store_sync(SCHEMA, SECRET_COLLECTION_DEFAULT, "Private Key", private_key, NULL, NULL, "user", "default_user", NULL);

    gtk_widget_destroy(dialog);
}

// Callback for the Remove Key action
static void remove_key_action(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    secret_password_clear_sync(SCHEMA, NULL, NULL, "user", "default_user", NULL);
    g_print("Key removed successfully\n");
}

// Creating and adding actions to the action map
static void create_actions(GtkApplication *app) {
    // Add Key action
    GSimpleAction *add_key = g_simple_action_new("add_key", NULL);
    g_signal_connect(add_key, "activate", G_CALLBACK(add_key_action), NULL);
    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(add_key));

    // Remove Key action
    GSimpleAction *remove_key = g_simple_action_new("remove_key", NULL);
    g_signal_connect(remove_key, "activate", G_CALLBACK(remove_key_action), NULL);
    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(remove_key));
}

// Register actions for managing permissions
static void create_permission_actions(GtkApplication *app) {
    // Add Permission action
    GSimpleAction *add_permission = g_simple_action_new("add_permission", NULL);
    g_signal_connect(add_permission, "activate", G_CALLBACK(add_permission_action), NULL);
    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(add_permission));

    // Remove Permission action
    GSimpleAction *remove_permission = g_simple_action_new("remove_permission", NULL);
    g_signal_connect(remove_permission, "activate", G_CALLBACK(remove_permission_action), NULL);
    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(remove_permission));
}


// Callback for adding permissions
static void add_permission_action(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK_CANCEL, "Enter the App ID and Permissions (comma-separated):");
    gtk_dialog_run(GTK_DIALOG(dialog));

    // Retrieve the entered app ID and permissions
    const gchar *input = gtk_entry_get_text(GTK_ENTRY(dialog));
    gchar **parts = g_strsplit(input, ":", 2);
    if (parts[0] && parts[1]) {
        const gchar *app_id = parts[0];
        gchar **new_permissions = g_strsplit(parts[1], ",", -1);

        // Set new permissions for the app
        set_permissions_for_app(app_id, new_permissions);

        g_strfreev(new_permissions);
    }

    g_strfreev(parts);
    gtk_widget_destroy(dialog);

    // Reload permissions into the TreeView
    load_permissions_into_treeview(GTK_TREE_VIEW(gtk_builder_get_object(builder, "permissions_list")));
}

// Callback for removing permissions
static void remove_permission_action(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK_CANCEL, "Enter the App ID to remove permissions:");

    gtk_dialog_run(GTK_DIALOG(dialog));

    // Retrieve the entered app ID
    const gchar *app_id = gtk_entry_get_text(GTK_ENTRY(dialog));

    // Remove permissions for the app by setting an empty list
    set_permissions_for_app(app_id, g_strv_new(""));

    gtk_widget_destroy(dialog);

    // Reload permissions into the TreeView
    load_permissions_into_treeview(GTK_TREE_VIEW(gtk_builder_get_object(builder, "permissions_list")));
}

// Callback for confirming the addition of permissions
static void add_permission_confirm_action(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    GtkEntry *app_id_entry = GTK_ENTRY(gtk_builder_get_object(builder, "app_id_entry"));
    const gchar *app_id = gtk_entry_get_text(app_id_entry);

    // Collect selected permissions
    GList *permissions = NULL;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "sign_event_check")))) {
        permissions = g_list_append(permissions, "sign_event");
    }
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "nip04_encrypt_check")))) {
        permissions = g_list_append(permissions, "nip04_encrypt");
    }
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "nip04_decrypt_check")))) {
        permissions = g_list_append(permissions, "nip04_decrypt");
    }
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "nip44_encrypt_check")))) {
        permissions = g_list_append(permissions, "nip44_encrypt");
    }
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "nip44_decrypt_check")))) {
        permissions = g_list_append(permissions, "nip44_decrypt");
    }

    // Convert the list of permissions to an array of strings
    gchar **permissions_array = g_malloc0_n(g_list_length(permissions) + 1, sizeof(gchar *));
    int i = 0;
    for (GList *l = permissions; l != NULL; l = l->next, i++) {
        permissions_array[i] = g_strdup(l->data);
    }
    permissions_array[i] = NULL;

    // Set the permissions for the app
    set_permissions_for_app(app_id, permissions_array);

    g_strfreev(permissions_array);
    g_list_free(permissions);

    // Reload the permissions in the TreeView
    load_permissions_into_treeview(GTK_TREE_VIEW(gtk_builder_get_object(builder, "permissions_list")));
    
    // Close the dialog
    gtk_widget_destroy(GTK_WIDGET(gtk_builder_get_object(builder, "add_permission_dialog")));
}


// Callback for confirming the removal of permissions
static void remove_permission_confirm_action(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    GtkEntry *app_id_entry = GTK_ENTRY(gtk_builder_get_object(builder, "remove_app_id_entry"));
    const gchar *app_id = gtk_entry_get_text(app_id_entry);

    // Remove permissions for the app by setting an empty list
    set_permissions_for_app(app_id, g_strv_new(""));

    // Reload the permissions in the TreeView
    load_permissions_into_treeview(GTK_TREE_VIEW(gtk_builder_get_object(builder, "permissions_list")));
    
    // Close the dialog
    gtk_widget_destroy(GTK_WIDGET(gtk_builder_get_object(builder, "remove_permission_dialog")));
}

void load_permissions_into_treeview(GtkTreeView *treeview) {
    GtkListStore *list_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
    GVariant *permissions_variant = g_settings_get_value(settings, "app-permissions");
    GVariantDict *permissions_dict = g_variant_dict_new(permissions_variant);

    GVariantIter iter;
    const gchar *app_id;
    GVariant *permissions;
    g_variant_iter_init(&iter, permissions_variant);

    while (g_variant_iter_next(&iter, "{sv}", &app_id, &permissions)) {
        GtkTreeIter tree_iter;
        gchar *permissions_str = g_variant_print(permissions, TRUE);  // Convert permissions to a readable string
        permissions_str = g_strdelimit(permissions_str, "[]", ' ');  // Remove square brackets for better readability

        gtk_list_store_append(list_store, &tree_iter);
        gtk_list_store_set(list_store, &tree_iter, 0, app_id, 1, permissions_str, -1);
        g_free(permissions_str);
    }

    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(list_store));

    g_variant_dict_unref(permissions_dict);
    g_variant_unref(permissions_variant);
}


// Show feedback after adding permissions
static void show_feedback(const gchar *message) {
    GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, message);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}


// Setting tooltips for buttons
static void setup_tooltips(GtkBuilder *builder) {
    GtkWidget *add_permission_button = GTK_WIDGET(gtk_builder_get_object(builder, "add_permission_button"));
    GtkWidget *remove_permission_button = GTK_WIDGET(gtk_builder_get_object(builder, "remove_permission_button"));

    gtk_widget_set_tooltip_text(add_permission_button, "Click to add new permissions for an application.");
    gtk_widget_set_tooltip_text(remove_permission_button, "Click to remove all permissions for an application.");
}
