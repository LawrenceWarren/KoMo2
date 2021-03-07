#include "DisassemblyView.h"
#include <iostream>

DisassemblyView::DisassemblyView(MainWindowView* const parent)
    : parent(parent) {
      std::cout << "Disassembly View inflated" << std::endl;
    }

void DisassemblyView::setModel(DisassemblyModel* const val) {
  model = val;
}