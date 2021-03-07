#include "DisassemblyModel.h"
#include <iostream>
#include "../views/DisassemblyView.h"

DisassemblyModel::DisassemblyModel(DisassemblyView* const view,
                                   KoMo2Model* const parent)
    : Model(parent), view(view) {
  view->setModel(this);
  std::cout << "Disassembly model created" << std::endl;
}

void DisassemblyModel::changeJimulatorState(const JimulatorState newState) {}

/**
 * @brief Handles any key press events.
 * @param e The key press event.
 * @return bool true if the key press was handled.
 */
const bool DisassemblyModel::handleKeyPress(const GdkEventKey* const e) {
  return false;
}

DisassemblyView* const DisassemblyModel::getView() {
  return view;
}