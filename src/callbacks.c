/**
 * @file callbacks.c
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief This file defines button click callback functions. 
 * @version 0.1
 * @date 2020-11-13
 * @copyright Copyright (c) 2020
 */

#include <gtk/gtk.h>

/**
 * @brief A handler for the button press - logs on the terminal.
 * @param widget The widget which called this function.
 * @param data A pointer to data sent to the function.
 */
void printHello(GtkWidget* widget, gpointer data) {
  static int counter = 0;
  g_print("Hello World %d\n", counter);
  counter++;
}