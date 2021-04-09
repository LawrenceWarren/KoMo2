/**
 * @file TerminalModel.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief The file containing the definition of the class TerminalModel, which
 * represents all of the data and manipulation of the data associated with the
 * terminal view of the KoMo2 GUI.
 * @version 0.1
 * @date 2021-03-31
 * @section LICENSE
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details at
 * https://www.gnu.org/copyleft/gpl.html
 */

#include "../views/TerminalView.h"
#include "KoMo2Model.h"

/**
 * @brief Construct a new TerminalModel::TerminalModel object.
 * @param view A constant pointer to the related view.
 * @param parent A constant pointer to the parent model.
 */
TerminalModel::TerminalModel(TerminalView* const view, KoMo2Model* const parent)
    : Model(parent), view(view) {
  view->setModel(this);
  setButtonListener(view->getClearButton(), this, &TerminalModel::onClearClick);
}

/**
 * @brief Handles the internal state of Jimulator being changed.
 * @param newState
 */
void TerminalModel::changeJimulatorState(const JimulatorState newState) {
  switch (newState) {
    case FINISHED:
      appendTextToTextView("Program execution finished!\n\n");
      break;
    default:
      break;
  }
}

/**
 * @brief Handles the clear button being clicked.
 */
void TerminalModel::onClearClick() {
  getView()->getTextView()->get_buffer()->set_text("");
}

/**
 * @brief Handles any key press events.
 * @param e The key press event.
 * @return bool true if the key press was handled.
 */
const bool TerminalModel::handleKeyPress(const GdkEventKey* const e) {
  // Do not override key presses if not in focus
  if (not isFocused()) {
    return false;
  }

  // If tab or right arrow pressed, give clear button focus
  if (e->keyval == GDK_KEY_Tab || e->keyval == GDK_KEY_Right) {
    getView()->getClearButton()->grab_focus();
  }
  // If escape or up arrow pressed, give scroll view focus
  else if (e->keyval == GDK_KEY_Escape || e->keyval == GDK_KEY_Up) {
    getView()->getTextView()->get_parent()->grab_focus();
  }
  // Else send key press to Jimulator...
  else {
    return Jimulator::sendTerminalInputToJimulator(e->keyval);
  }

  return true;
}

/**
 * @brief Appends a string to the current text view of the terminal, and scroll
 * to the bottom of the terminal.
 * @param text The text to append to the text view.
 */
void TerminalModel::appendTextToTextView(std::string text) {
  auto* const view = getView()->getTextView();

  // Append text to the buffer, reset the buffer
  auto buff = view->get_buffer();
  buff->insert(buff->end(), text);
  view->set_buffer(buff);

  // Scroll to the bottom of the scroll bar
  buff = view->get_buffer();
  view->scroll_to(buff->create_mark(buff->end(), false));

  // Notify that text was logged
  view->get_accessible()->set_role(Atk::ROLE_NOTIFICATION);
  view->get_accessible()->notify_state_change(ATK_STATE_SHOWING, true);
}

/**
 * @brief Reads for any data from Jimulator.
 * @return const std::string the data read from Jimulator.
 */
const std::string TerminalModel::readJimulator() const {
  return Jimulator::getJimulatorTerminalMessages();
}

/**
 * @brief Returns whether or not the input box has focus or not.
 * @return true If the input box has focus.
 * @return false If the input box does not have focus.
 */
const bool TerminalModel::isFocused() {
  return getView()->getInputBox()->is_focus();
}

// ! Getters and setters

/**
 * @brief Gets a pointer to the view object.
 * @return TerminalView* const
 */
TerminalView* const TerminalModel::getView() const {
  return view;
}