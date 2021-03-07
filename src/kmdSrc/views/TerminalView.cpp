#include "TerminalView.h"

TerminalView::TerminalView(MainWindowView* const parent) : parent(parent) {}

void TerminalView::setModel(TerminalModel* const val) {
  model = val;
}
