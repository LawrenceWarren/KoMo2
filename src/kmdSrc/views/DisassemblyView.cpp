#include "DisassemblyView.h"

DisassemblyView::DisassemblyView(MainWindowView* const parent)
    : parent(parent) {
  initDisassemblyContainer();
}

void DisassemblyView::initDisassemblyContainer() {}

// !!!!!!!!!!!!!!!!!!!!!!!
// ! Getters and setters !
// !!!!!!!!!!!!!!!!!!!!!!!

void DisassemblyView::setModel(DisassemblyModel* const val) {
  model = val;
}
