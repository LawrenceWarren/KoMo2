/**
 * @file views.c
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief Defines any views and view related functions.
 * @version 0.1
 * @date 2020-11-13
 * @copyright Copyright (c) 2020
 */

#include <gtk/gtk.h>
#include <stdio.h>

#include "callbacks.h"

/**
 * @brief Adds a ButtonBox object to a window.
 * @param window The window to add the buttonBox to.
 * @param buttonBox The buttonbox to add the window to.
 */
void AddButtonBoxToWindow(GtkWidget *window, GtkWidget *buttonBox) {
    gtk_button_box_set_layout(GTK_BUTTON_BOX(buttonBox), GTK_BUTTONBOX_CENTER);
    gtk_container_add(GTK_CONTAINER(window), buttonBox);
}

/**
 * @brief Adds a Button object to a layout container structure, optionally 
 * providing any signal information, callback functions and callback 
 * function data as necessary.
 * @param container The layout container structure to hold the buttons.
 * @param button The button to be added to the layout structure.
 * @param signal If any callbacks are provided, the signal to link those 
 * callbacks to.
 * @param clickCallback A function pointer used to handle the signal event. For
 * example, if the signal is "clicked," this function will handle the click 
 * event. Calls `g_signal_connect`.
 * @param callbackData The data passed to the clickCallback function.
 * @param signalSwapped A function pointer used to handle the signal event. For
 * example, if the signal is "clicked," this function will handle the click
 * event. Calls `g_signal_connect_swapped`.
 * @param sigSwapData The data passed to the signalSwapped function.
 */
void addButtonToContainer(GtkWidget *container, GtkWidget *button, char *signal,
                        void (*clickCallback)(), gpointer callbackData,
                        void (*signalSwapped)(), gpointer sigSwapData) {
    if (clickCallback && signal) {
        g_signal_connect(button, signal, G_CALLBACK(clickCallback),
                         callbackData);
    }

    if (signalSwapped && signal) {
        g_signal_connect_swapped(button, signal, G_CALLBACK(signalSwapped),
                                 sigSwapData);
    }

    gtk_container_add(GTK_CONTAINER(container), button);
}

/**
 * @brief Creates the programs main window.
 * @param app The pointer to the GTK application.
 * @param user_data Any data passed to the createMainWindow function.
 */
void createMainWindow(GtkApplication *app, gpointer user_data) {
    // ! Sets up the main window.

    // The programs main window.
    GtkWidget *mainWindow = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(mainWindow), "KoMo2");
    gtk_window_set_default_size(GTK_WINDOW(mainWindow), 1156, 727);

    // ! Sets up the button layout box.

    // The layout box for the buttons.
    GtkWidget *buttonBox = gtk_button_box_new(GTK_ORIENTATION_VERTICAL);
    AddButtonBoxToWindow(mainWindow, buttonBox);

    // ! Sets up the hello world button to display.

    // The hello world button.
    GtkWidget *helloWorld = gtk_button_new_with_label("Print info!");
    addButtonToContainer(buttonBox, helloWorld, "clicked", printHello,
                       NULL, NULL, NULL);

    // ! Sets up the Custom button.

    // The custom button.
    GtkWidget *newButton = gtk_button_new_with_label("Quit");
    addButtonToContainer(buttonBox, newButton, "clicked", NULL, NULL,
                       gtk_widget_destroy, mainWindow);

    gtk_widget_show_all(mainWindow);
}
