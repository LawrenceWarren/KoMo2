#include <iostream>
#include "../views/TerminalView.h"
#include "KoMo2Model.h"

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
  // If enter is pressed
  if (e->keyval == GDK_KEY_Return) {
    auto buff = getView()->getCurrentText();

    auto lastLine = buff->get_text(
        buff->get_iter_at_line(buff->get_line_count() - 1), buff->end());

    std::cout << "Last line " << buff->get_line_count() - 1 << " is "
              << lastLine << std::endl;
  }

  return false;
}

TerminalView* const TerminalModel::getView() {
  return view;
}
