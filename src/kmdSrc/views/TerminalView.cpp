#include "TerminalView.h"
#include <iostream>

TerminalView::TerminalView(MainWindowView* const parent) : parent(parent) {
  scroll.set_size_request(700, 200);
  textView.set_size_request(700, 200);
  inputBox.set_size_request(700, 20);

  // Sets the colours of the terminal input box
  // TODO: try to get this to work in CSS?
  inputBox.get_style_context()->add_class("terminalInput");
  textView.get_style_context()->add_class("terminalDisplay");
  Gdk::RGBA bgColour("#24292e");
  Gdk::RGBA textColour("#ffffff");
  textView.override_color(textColour);
  textView.override_background_color(bgColour);
  textView.set_wrap_mode(Gtk::WRAP_WORD_CHAR);

  textView.set_can_focus(false);
  textView.set_editable(false);

  scroll.add(textView);
  pack_start(scroll, false, true);
  pack_start(inputBox, false, true);

  scroll.show();
  textView.show();
  inputBox.show();
  show();
}

void TerminalView::setModel(TerminalModel* const val) {
  model = val;
}

const bool TerminalView::isFocused() {
  return inputBox.is_focus();
}

const std::string TerminalView::getCurrentText() {
  return inputBox.get_buffer()->get_text();
}

Gtk::TextView* const TerminalView::getTextView() {
  return &textView;
}

void TerminalView::clearInputBox() {
  inputBox.delete_text(0, -1);
}
