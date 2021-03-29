#include "TerminalView.h"
#include <iostream>

TerminalView::TerminalView(MainWindowView* const parent) : parent(parent) {
  scroll.set_size_request(700, 200);
  textView.set_size_request(700, 200);
  inputBox.set_size_request(600, 20);
  clearButton.set_size_request(100, 20);
  clearButton.set_label("Clear");
  clearButton.get_style_context()->add_class("compButtons");

  clearButton.get_accessible()->set_name("Clear");
  clearButton.get_accessible()->set_description(
      "Clears the terminal of any logged information.");

  // Sets the colours of the terminal input box
  inputBox.get_style_context()->add_class("terminalInput");
  textView.get_style_context()->add_class("terminalDisplay");
  textView.set_wrap_mode(Gtk::WRAP_WORD_CHAR);

  textView.set_can_focus(true);
  textView.set_editable(false);

  // textView.set_border_width(2);

  layout.pack_start(inputBox, false, false);
  layout.pack_start(clearButton, false, true);
  scroll.add(textView);
  pack_start(scroll, false, true);
  pack_start(layout, false, true);

  scroll.show();
  textView.show();
  inputBox.show();
  show();
}

const bool TerminalView::isFocused() {
  return inputBox.is_focus();
}

void TerminalView::clearInputBox() {
  inputBox.delete_text(0, -1);
}

void TerminalView::clearTextView() {
  textView.get_buffer()->set_text("");
}

// !!!!!!!!!!!!!!!!!!!!!
// ! Getters & Setters !
// !!!!!!!!!!!!!!!!!!!!!

Gtk::Button* const TerminalView::getClearButton() {
  return &clearButton;
}
Gtk::TextView* const TerminalView::getTextView() {
  return &textView;
}
const std::string TerminalView::getCurrentText() {
  return inputBox.get_buffer()->get_text();
}
void TerminalView::setModel(TerminalModel* const val) {
  model = val;
}
