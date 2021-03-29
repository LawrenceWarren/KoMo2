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

#include "DisassemblyModel.h"
#include <iomanip>
#include <iostream>
#include <sstream>
#include "../views/MainWindowView.h"

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
  getView()->refreshViews(getMemoryValues());
}

/**
 * @brief Adds scroll recognition to the container object, and sends scroll
 * events to the  member function `handleScroll`.
 */
void DisassemblyModel::addScrollRecognition() {
  getView()->add_events(Gdk::SMOOTH_SCROLL_MASK);
  getView()->signal_scroll_event().connect(
      sigc::mem_fun(*this, &DisassemblyModel::handleScroll), false);
}

/**
 * @brief Converts an fixed width 32-bit integer to a hex string formatted as
 * required (i.e. padded to 8 characters, pre-fixed with a "0x" string, raised
 * to all capitals)
 * @param formatMe The integer to be formatted
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
      break;
  }

  // Refresh the views.
  getView()->refreshViews(getMemoryValues());
  return true;
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
  // Get the rows
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

  // If enter key pressed, find out if a child has focus and toggle it's break
  else if (e->keyval == GDK_KEY_Return) {
    for (auto& row : *rows) {
      if (row.has_focus()) {
        row.setBreakpoint(not row.getBreakpoint());
        return true;
      }
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
std::array<Jimulator::MemoryValues, 15> DisassemblyModel::getMemoryValues() {
  return Jimulator::getJimulatorMemoryValues(memoryIndex);
}
