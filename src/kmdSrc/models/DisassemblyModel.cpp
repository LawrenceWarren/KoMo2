#include "DisassemblyModel.h"
#include <iostream>
#include "../views/DisassemblyView.h"

DisassemblyModel::DisassemblyModel(DisassemblyView* const view,
                                   KoMo2Model* const parent)
    : Model(parent), view(view) {
  view->setModel(this);

  getView()->add_events(Gdk::SMOOTH_SCROLL_MASK);

  getView()->signal_scroll_event().connect(
      sigc::mem_fun(*this, &DisassemblyModel::handleScroll), false);
}

/**
 * @brief Passes the key press event off to other child models.
 * @param e The key press event.
 * @return bool if a key was pressed.
 */
const bool DisassemblyModel::handleScroll(GdkEventScroll* e) {
  std::cout << "scroll ";

  switch (e->direction) {
    case GDK_SCROLL_UP:
      std::cout << "UP" << std::endl;
      break;
    case GDK_SCROLL_DOWN:
      std::cout << "DOWN" << std::endl;
      break;
    default:
      std::cout << "INVALID" << std::endl;
  }

  return true;
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
