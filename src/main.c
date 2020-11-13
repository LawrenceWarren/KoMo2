/**
 * @file main.c
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief The entry point of the KoMo2 program.
 * @version 0.1
 * @date 2020-11-13
 * @copyright Copyright (c) 2020
 */

#include <gtk/gtk.h>

#include "views.h"

/**
 * @brief Creates a GtkApplication and runs it.
 * @param argc The number of arguments.
 * @param argv The list of arguments.
 * @return int status code.
 */
int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("uon.komodo",
                                              G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(createMainWindow),
                     NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
