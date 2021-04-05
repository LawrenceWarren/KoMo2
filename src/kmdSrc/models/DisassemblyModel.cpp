/**
 * @file DisassemblyModel.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief This file defines the functions declared in the file
 * `DisassemblyModel.h`.
 * @version 0.1
 * @date 2021-03-18
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

#include <iomanip>
#include <iostream>
#include <sstream>
#include "../views/MainWindowView.h"
#include "KoMo2Model.h"

// Initialise static list pointers
uint32_t DisassemblyModel::memoryIndex = 0;

/**
 * @brief Construct a new DisassemblyModel::DisassemblyModel object.
 * @param view A pointer to the view object, set at initialisation.
 * @param parent A pointer to the parent object, set at initialisation.
 */
DisassemblyModel::DisassemblyModel(DisassemblyView* const view,
                                   KoMo2Model* const parent)
    : Model(parent), view(view) {
  view->setModel(this);
  addScrollRecognition();
  setupButtonHandlers();
}

/**
 * @brief Handle the toggling of a breakpoint within the `DisassemblyView.`
 * @param row The row which has had it's breakpoint toggled. Row id's are a
 * direct index - the first row is 0, the last row is 14 - so we can combine the
 * row ID with the `memoryIndex` variable to identify what address the
 * breakpoint should be set at.
 */
void DisassemblyModel::onBreakpointToggle(DisassemblyRows* const row) const {
  row->setBreakpoint(Jimulator::setBreakpoint(row->getAddressVal()));
}

/**
 * @brief Adds button handlers to every breakpoint button.
 */
void DisassemblyModel::setupButtonHandlers() {
  auto* const rows = getView()->getRows();

  for (long unsigned int i = 0; i < rows->size(); i++) {
    (*rows)[i].getButton()->signal_clicked().connect(
        sigc::bind(sigc::mem_fun(*this, &DisassemblyModel::onBreakpointToggle),
                   &(*rows)[i]));
  }
}

/**
 * @brief Adds scroll recognition to the container object, which causes scroll
 * events to be sent to the member function `handleScroll.`
 */
void DisassemblyModel::addScrollRecognition() {
  getView()->add_events(Gdk::SMOOTH_SCROLL_MASK);
  getView()->signal_scroll_event().connect(
      sigc::mem_fun(*this, &DisassemblyModel::handleScroll), false);
}

/**
 * @brief Handles the scroll events.
 * @param e The key press event.
 * @return bool if a key was pressed.
 */
const bool DisassemblyModel::handleScroll(GdkEventScroll* const e) {
  // Increase or decrease the memory index.
  switch (e->direction) {
    case GDK_SCROLL_UP:
      incrementMemoryIndex(-1);
      break;
    case GDK_SCROLL_DOWN:
      incrementMemoryIndex(1);
      break;
    case GDK_SCROLL_SMOOTH:
      incrementMemoryIndex(e->delta_y);
      break;
    default:
      return false;
  }

  // Refresh this view only
  refreshViews();
  return true;
}

/**
 * @brief Converts a fixed width 32-bit integer to a hex string, padded with 0's
 * to 8 characters, pre-fixed with "0x", and raised to all capitals.
 * @param formatMe The integer to be formatted.
 * @return const std::string The formatted string.
 */
const std::string DisassemblyModel::intToFormattedHexString(
    const uint32_t formatMe) const {
  std::stringstream stream;

  // Pads string, converts to hex
  stream << "0x" << std::setfill('0') << std::setw(8) << std::uppercase
         << std::hex << formatMe;

  return stream.str();
}

/**
 * @brief Refreshes the values in the views to display the new values fetched
 * from Jimulator.
 */
void DisassemblyModel::refreshViews() {
  const auto vals = getMemoryValues();
  auto* const rows = getView()->getRows();

  // Loop through each of the fetched rows
  for (int i = 0; i < 15; i++) {
    auto& row = (*rows)[i];
    auto flag = row.get_state_flags();

    updateCSSFlags(flag, row, vals[i].address);
    row.setAddressVal(vals[i].address);
    row.setAddress(intToFormattedHexString(vals[i].address));
    row.setHex(vals[i].hex);
    row.setDisassembly(vals[i].disassembly);
    row.setBreakpoint(vals[i].breakpoint);

    // Gets a string describing the state of the breakpoint
    // Used for the accessibility object
    std::string bp = vals[i].breakpoint ? "breakpoint set" : "no breakpoint";

    // Set the accessibility for the view
    std::stringstream ss;
    ss << "address " << std::hex << vals[i].address << " : "
       << vals[i].disassembly << " : " << bp;
    row.get_accessible()->set_description(ss.str());
  }
}

/**
 * @brief Handles setting the CSS flags for each disassembly row, which
 * determines which CSS class it uses and therefore how it looks.
 * @param flag The current CSS state of the memory row.
 * @param row The current memory row.
 * @param address The address of the current memory row.
 */
void DisassemblyModel::updateCSSFlags(const Gtk::StateFlags flag,
                                      DisassemblyRows& row,
                                      const uint32_t address) {
  // If this is the program counter address...
  if (intToFormattedHexString(address) == PCValue) {
    // if grey, make yellow
    if (flag == Gtk::STATE_FLAG_DIR_LTR) {
      row.set_state_flags(Gtk::STATE_FLAG_DIR_LTR | Gtk::STATE_FLAG_ACTIVE);
    }
    // if blue, make yellow-blue
    else if (flag == (Gtk::STATE_FLAG_DIR_LTR | Gtk::STATE_FLAG_FOCUSED)) {
      row.set_state_flags(Gtk::STATE_FLAG_DIR_LTR | Gtk::STATE_FLAG_FOCUSED |
                          Gtk::STATE_FLAG_ACTIVE);
    }
  }
  // If this is just a generic address
  else {
    // if yellow, make grey
    if (flag == (Gtk::STATE_FLAG_DIR_LTR | Gtk::STATE_FLAG_ACTIVE)) {
      row.set_state_flags(Gtk::STATE_FLAG_DIR_LTR);
    }
    // if yellow-blue, make blue
    else if (flag == (Gtk::STATE_FLAG_DIR_LTR | Gtk::STATE_FLAG_FOCUSED |
                      Gtk::STATE_FLAG_ACTIVE)) {
      row.set_state_flags(Gtk::STATE_FLAG_DIR_LTR | Gtk::STATE_FLAG_FOCUSED);
    }
  }
}

/**
 * @brief Updates the list pointers to a new value. `Val` is multipled by 4 -
 * for example, if `val` is 1, `memoryIndex` is incremented by 4, as this is the
 * gap between memory registers.
 * @param val The value to increment by.
 */
void DisassemblyModel::incrementMemoryIndex(const uint32_t val) {
  memoryIndex += val * 4;
}

// ! Virtual override functions

/**
 * @brief Handles changes of Jimulator state.
 * @param newState The state Jimulator has changed into.
 */
void DisassemblyModel::changeJimulatorState(const JimulatorState newState) {}

/**
 * @brief Handles any key press events.
 * @param e The key press event.
 * @return bool true if the key press was handled.
 */
const bool DisassemblyModel::handleKeyPress(const GdkEventKey* const e) {
  auto rows = getView()->getRows();

  // If the top row has focus and it's a key press up, handle it
  if ((*rows)[0].has_focus() && e->keyval == GDK_KEY_Up) {
    auto scroll = GdkEventScroll();
    scroll.direction = GDK_SCROLL_UP;
    handleScroll(&scroll);
    return true;
  }

  // If the bottom row has focus and it's a key press down, handle it
  else if ((*rows)[14].has_focus() && e->keyval == GDK_KEY_Down) {
    auto scroll = GdkEventScroll();
    scroll.direction = GDK_SCROLL_DOWN;
    handleScroll(&scroll);
    return true;
  }

  // Identifies if a child has focus - if not, return false

  long unsigned int hasFocus = -1;

  for (long unsigned int i = 0; i < rows->size(); i++) {
    if ((*rows)[i].has_focus()) {
      hasFocus = i;
    }
  }

  if (hasFocus == static_cast<long unsigned int>(-1)) {
    return false;
  }

  // If enter key pressed and a child has focus, toggle its breakpoint
  else if (e->keyval == GDK_KEY_Return) {
    onBreakpointToggle(&(*rows)[hasFocus]);
    return true;

  }

  // If escape pressed and in focus, lose focus
  else if (e->keyval == GDK_KEY_Escape) {
    switch (hasFocus < rows->size() / 2) {
      // Help button grabs focus
      case true:
        getParent()
            ->getMainWindow()
            ->getControlsView()
            ->getHelpButton()
            ->grab_focus();
        return true;
      // output box grabs focus
      case false:
        getParent()
            ->getMainWindow()
            ->getTerminalView()
            ->getTextView()
            ->grab_focus();
        return true;
    }
  }

  return false;
}

// !!!!!!!!!!!!!!!!!!!!!!!
// ! Getters and setters !
// !!!!!!!!!!!!!!!!!!!!!!!

/**
 * @brief Returns a pointer to the view object.
 * @return DisassemblyView* const The view pointer.
 */
DisassemblyView* const DisassemblyModel::getView() {
  return view;
}
/**
 * @brief Reads memory values from Jimulator.
 * @return std::array<Jimulator::MemoryValues, 15> An array of the 15 memory
 * values - their addresses, their hex columns and their disassembly/source
 * columns.
 */
const std::array<Jimulator::MemoryValues, 15>
DisassemblyModel::getMemoryValues() const {
  return Jimulator::getJimulatorMemoryValues(memoryIndex);
}
/**
 * @brief Updates the value of PCValue.
 * @param val The value to set PCValue to.
 */
void DisassemblyModel::setPCValue(const std::string val) {
  PCValue = val;
}