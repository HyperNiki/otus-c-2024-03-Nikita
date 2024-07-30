#include <gtk/gtk.h>

#include <gtk/gtk.h>
#include <dirent.h>
#include <sys/stat.h>

static void populate_tree_store(GtkTreeStore *store, GtkTreeIter *parent, const gchar *path) {
    GDir *dir;
    const gchar *filename;
    GtkTreeIter iter;
    gchar *fullpath;
    struct stat s;

    dir = g_dir_open(path, 0, NULL);
    if (!dir)
        return;

    while ((filename = g_dir_read_name(dir)) != NULL) {
        fullpath = g_build_filename(path, filename, NULL);
        if (stat(fullpath, &s) == 0) {
            gtk_tree_store_append(store, &iter, parent);
            gtk_tree_store_set(store, &iter, 0, filename, -1);

            if (S_ISDIR(s.st_mode)) {
                populate_tree_store(store, &iter, fullpath);
            }
        }
        g_free(fullpath);
    }
    g_dir_close(dir);
}

static GtkTreeStore* create_and_fill_model(void) {
    GtkTreeStore *store;
    GtkTreeIter iter;
    gchar *current_dir;

    store = gtk_tree_store_new(1, G_TYPE_STRING);

    current_dir = g_get_current_dir();
    populate_tree_store(store, NULL, current_dir);
    g_free(current_dir);

    return store;
}

static GtkWidget* create_view_and_model(void) {
    GtkTreeViewColumn *col;
    GtkCellRenderer *renderer;
    GtkWidget *view;
    GtkTreeStore *store;

    view = gtk_tree_view_new();

    col = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(col, "Files");
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(col, renderer, TRUE);
    gtk_tree_view_column_add_attribute(col, renderer, "text", 0);

    store = create_and_fill_model();
    gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(store));

    g_object_unref(store);

    return view;
}

static void on_activate(GtkApplication *app) {
    GtkWidget *window;
    GtkWidget *treeview;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "File Browser");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);

    treeview = create_view_and_model();
    gtk_window_set_child(GTK_WINDOW(window), treeview);

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {

    if (argc != 1) {
        printf("Использование: %s\n", argv[0]);
        return 1;
    }

    GtkApplication *app;
    int status;

    app = gtk_application_new("com.example.GtkApplication", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
