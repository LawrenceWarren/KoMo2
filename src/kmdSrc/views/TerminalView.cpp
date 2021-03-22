#include "TerminalView.h"
#include <iostream>

TerminalView::TerminalView(MainWindowView* const parent) : parent(parent) {
  scroll.set_size_request(500, 150);
  inputBox.set_size_request(500, 150);

  scroll.add(inputBox);
  pack_start(scroll, false, true);

  scroll.show();
  inputBox.show();
  show();

  // inputBox.signal_add().connect(
  //    sigc::mem_fun(*this, &TerminalView::handleTextChange));
}

void TerminalView::setModel(TerminalModel* const val) {
  model = val;
}

const bool TerminalView::isFocused() {
  return inputBox.is_focus();
}

Glib::RefPtr<Gtk::TextBuffer> TerminalView::getCurrentText() {
  return inputBox.get_buffer();
}
