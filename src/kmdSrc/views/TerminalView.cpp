#include "TerminalView.h"
#include <iostream>

TerminalView::TerminalView(MainWindowView* const parent) : parent(parent) {
  scroll.set_size_request(700, 150);
  textView.set_size_request(700, 150);

  // Sets the colours of the terminal input box
  // TODO: try to get this to work in CSS?
  textView.get_style_context()->add_class("terminalDisplay");
  Gdk::RGBA bgColour("#444d56");
  Gdk::RGBA textColour("#ffffff");
  textView.override_color(textColour);
  textView.override_background_color(bgColour);
  textView.set_wrap_mode(Gtk::WRAP_WORD);

  textView.set_can_focus(false);
  textView.set_editable(false);

  scroll.add(textView);
  pack_start(scroll, false, true);

  scroll.show();
  textView.show();
  show();

  // textView.signal_add().connect(
  //    sigc::mem_fun(*this, &TerminalView::handleTextChange));
}

void TerminalView::setModel(TerminalModel* const val) {
  model = val;
}

const bool TerminalView::isFocused() {
  return textView.is_focus();
}

Glib::RefPtr<Gtk::TextBuffer> TerminalView::getCurrentText() {
  return textView.get_buffer();
}

Gtk::TextView* const TerminalView::getTextView() {
  return &textView;
}