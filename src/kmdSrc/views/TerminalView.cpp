/**
 * @file TerminalView.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief Defines the TerminalView class, which is an element of the KoMo2 GUI
 * which represents the input & output text boxes at the bottom of the screen.
 * Any data, and functions related to the manipulation of data, can be found in
 * the accompanying TerminalModel class.
 * @version 1.0.0
 * @date 10-04-2021
 */

#include "TerminalView.h"
#include <iostream>

/**
 * @brief Construct a new TerminalView::TerminalView object.
 * @param parent A constant pointer to the parent view.
 */
TerminalView::TerminalView(MainWindowView* const parent) : parent(parent) {
  initScrollView();
  initClearButton();
  initTextView();
  initInputBox();
  packChildren();
}

/**
 * @brief Initialises the scroll view.
 */
void TerminalView::initScrollView() {
  scroll.set_size_request(948, 150);
  scroll.get_accessible()->set_name("Terminal output");
}

/**
 * @brief Initialises the clear button.
 */
void TerminalView::initClearButton() {
  clearButton.set_size_request(100, 20);
  clearButton.set_label("Clear");
  clearButton.get_style_context()->add_class("compButtons");
  clearButton.get_accessible()->set_name("Clear");
  clearButton.get_accessible()->set_description(
      "Clears the terminal of any logged information.");
  clearButton.set_tooltip_text("Clear the terminal output pane");
}

/**
 * @brief Initialises the output box.
 */
void TerminalView::initTextView() {
  textView.set_size_request(948, 150);
  textView.get_style_context()->add_class("terminalDisplay");
  textView.set_wrap_mode(Gtk::WRAP_WORD_CHAR);
  textView.set_can_focus(true);
  textView.set_editable(false);
  textView.get_accessible()->set_name("Terminal output window");
  textView.set_tooltip_text("Emulator output is logged here");
}

/**
 * @brief Initialises the input box.
 */
void TerminalView::initInputBox() {
  inputBox.set_size_request(848, 20);
  inputBox.get_style_context()->add_class("terminalInput");
  inputBox.get_accessible()->set_name("Terminal input window");
  inputBox.set_tooltip_text("Text input here is sent to the emulator");
}

/**
 * @brief Pack all of the child elements together and show them.
 */
void TerminalView::packChildren() {
  layout.pack_start(inputBox, true, true);
  layout.pack_start(clearButton, true, true);
  scroll.add(textView);
  pack_start(scroll, true, true);
  pack_start(layout, true, true);

  show();
  show_all_children();
}

// !!!!!!!!!!!!!!!!!!!!!
// ! Getters & Setters !
// !!!!!!!!!!!!!!!!!!!!!

/**
 * @brief Sets the model member.
 * @param val The value to set the model member to.
 */
void TerminalView::setModel(TerminalModel* const val) {
  model = val;
}
/**
 * @brief Returns a constant pointer to the clear button.
 * @return Gtk::Button* const A constant pointer to the clear button.
 */
Gtk::Button* const TerminalView::getClearButton() {
  return &clearButton;
}
/**
 * @brief Returns a constant pointer to the output box.
 * @return Gtk::TextView* const A constant pointer to the output box.
 */
Gtk::TextView* const TerminalView::getTextView() {
  return &textView;
}
/**
 * @brief Returns a constant pointer to the input box.
 * @return Gtk::Entry* const A constant pointer to the input box.
 */
Gtk::Entry* const TerminalView::getInputBox() {
  return &inputBox;
}
