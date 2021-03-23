#include <iostream>
#include <regex>
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

    // TODO: regex out the previously text of the line
    // TODO: try to convert to sendable format
    // TODO: send it!

    return false;
  }

  return false;
}

TerminalView* const TerminalModel::getView() {
  return view;
}

void TerminalModel::appendTextToTextView(std::string text) {
  auto buff = getView()->getTextView()->get_buffer();
  buff->insert(buff->end(), text);
  getView()->getTextView()->set_buffer(buff);
}