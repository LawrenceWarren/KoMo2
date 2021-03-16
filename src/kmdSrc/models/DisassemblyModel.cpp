#include "DisassemblyModel.h"
#include <iomanip>
#include <iostream>
#include <sstream>
#include "../jimulatorInterface.h"
#include "../views/DisassemblyView.h"

// Initialise static list pointers.
uint32_t DisassemblyModel::rowIDTail = 0;
uint32_t DisassemblyModel::rowIDHead = 56;

/**
 * @brief Construct a new Disassembly Model:: Disassembly Model object.
 * @param view A pointer to the view object.
 * @param parent A pointer to the parent object.
 */
DisassemblyModel::DisassemblyModel(DisassemblyView* const view,
                                   KoMo2Model* const parent)
    : Model(parent), view(view) {
  view->setModel(this);
  addScrollRecognition();
  initialiseRowViews();
}

/**
 * @brief Adds scroll recognition to the container object, and links it to the
 * `handleScroll` function.
 */
void DisassemblyModel::addScrollRecognition() {
  getView()->add_events(Gdk::SMOOTH_SCROLL_MASK);
  getView()->signal_scroll_event().connect(
      sigc::mem_fun(*this, &DisassemblyModel::handleScroll), false);
}

/**
 * @brief Sets up the initial rows of the container.
 */
void DisassemblyModel::initialiseRowViews() {
  for (long unsigned int i = 0; i < rowModels.size(); i++) {
    // Populates the model
    rowModels[i] = RowModel(false, i * 4, "00000000", "NOP");

    // Set the view values
    (*getView()->getRows())[i].setBreakpoint(rowModels[i].getBreakpoint());
    (*getView()->getRows())[i].setAddress(
        intToFormattedHexString(rowModels[i].getAddress()));
    (*getView()->getRows())[i].setHex(rowModels[i].getHex());
    (*getView()->getRows())[i].setDisassembly(rowModels[i].getDisassembly());
  }
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
const bool DisassemblyModel::handleScroll(GdkEventScroll* e) {
  switch (e->direction) {
    // Rotates left. Example output:
    // 0 1 2 3 4 5 6 7 8 9
    // 9 0 1 2 3 4 5 6 7 8
    // The final element of the vector (14) becomes first element (0). All
    // others are shuffled down.
    case GDK_SCROLL_UP: {
      adjustListPointers(-4);

      // Update model & view values
      rowModels[14].setAddress(rowIDTail);
      (*getView()->getRows())[14].setAddress(
          intToFormattedHexString(rowIDTail));

      // TODO: read from Jimulator the memory value rowIDTail

      // Rotate left
      std::rotate(rowModels.rbegin(), rowModels.rbegin() + 1, rowModels.rend());
      std::rotate(getView()->getRows()->rbegin(),
                  getView()->getRows()->rbegin() + 1,
                  getView()->getRows()->rend());

      break;
    }

    // Rotates right. Example output:
    // 0 1 2 3 4 5 6 7 8 9
    // 1 2 3 4 5 6 7 8 9 0
    // The first element of the vector (0) becomes final element (14). All
    // others are shuffled down.
    case GDK_SCROLL_DOWN: {
      adjustListPointers(4);

      // Update model & view values
      rowModels[0].setAddress(rowIDHead);

      // TODO: read from Jimulator the memory value rowIDHead

      (*getView()->getRows())[0].setAddress(intToFormattedHexString(rowIDHead));

      // Rotate right
      std::rotate(rowModels.begin(), rowModels.begin() + 1, rowModels.end());
      std::rotate(getView()->getRows()->begin(),
                  getView()->getRows()->begin() + 1,
                  getView()->getRows()->end());

      break;
    }
    default:
      // Do nothing in this case
      break;
  }

  getView()->packView(true);
  return true;
}

/**
 * @brief Updates the list pointers to a new value. SHOULD ALWAYS BE A MULTIPLE
 * OF 4.
 * @param val The value to increment by.
 */
void DisassemblyModel::adjustListPointers(const uint32_t val) {
  rowIDTail += val;
  rowIDHead += val;
}

void DisassemblyModel::getMemoryValues() {
  getJimulatorMemoryValues();
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
  return false;
}

/**
 * @brief Returns a pointer to the view object.
 * @return DisassemblyView* const The view pointer.
 */
DisassemblyView* const DisassemblyModel::getView() {
  return view;
}

// !!!!!!!!!!!!!!!!!!!!!
// ! Child class stuff !
// !!!!!!!!!!!!!!!!!!!!!

RowModel::RowModel(const bool breakpoint,
                   const uint32_t address,
                   const std::string hex,
                   const std::string disassembly)
    : breakpoint(breakpoint),
      address(address),
      hex(hex),
      disassembly(disassembly) {}

RowModel::RowModel() {}

constexpr const bool RowModel::getBreakpoint() const {
  return breakpoint;
}
const uint32_t RowModel::getAddress() const {
  return address;
}
const std::string RowModel::getHex() const {
  return hex;
}
const std::string RowModel::getDisassembly() const {
  return disassembly;
}
void RowModel::setBreakpoint(const bool toggle) {
  breakpoint = toggle;
}
void RowModel::setAddress(const uint32_t text) {
  address = text;
}
void RowModel::setHex(const std::string text) {
  hex = text;
}
void RowModel::setDisassembly(const std::string text) {
  disassembly = text;
}
