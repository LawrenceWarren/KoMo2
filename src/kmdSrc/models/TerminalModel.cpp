#include "TerminalModel.h"
#include <iostream>
#include "../views/TerminalView.h"

// TODO: look at the function start_gtk && callback_console_update

TerminalModel::TerminalModel(TerminalView* const view, KoMo2Model* const parent)
    : Model(parent), view(view) {
  view->setModel(this);
}

void TerminalModel::changeJimulatorState(const JimulatorState newState) {}

/**
 * @brief Handles any key press events.
 * @param e The key press event.
 * @return bool true if the key press was handled.
 */
const bool TerminalModel::handleKeyPress(const GdkEventKey* const e) {
  return false;
}

TerminalView* const TerminalModel::getView() {
  return view;
}
