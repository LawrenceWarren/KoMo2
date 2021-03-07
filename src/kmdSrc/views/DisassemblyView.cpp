#include "DisassemblyView.h"
#include <iostream>

DisassemblyView::DisassemblyView(MainWindowView* const parent)
    : parent(parent) {}

void DisassemblyView::setModel(DisassemblyModel* const val) {
  model = val;
}