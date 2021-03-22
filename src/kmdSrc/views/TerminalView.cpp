#include "TerminalView.h"
#include <iostream>

TerminalView::TerminalView(MainWindowView* const parent) : parent(parent) {
  scroll.set_size_request(500, 200);
  inputBox.set_size_request(500, 200);

  scroll.add(inputBox);
  pack_start(scroll, false, true);

  scroll.show();
  inputBox.show();
  show();
}

void TerminalView::setModel(TerminalModel* const val) {
  model = val;
}
