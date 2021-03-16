#include "DisassemblyModel.h"
#include <iomanip>
#include <iostream>
#include <sstream>
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
    case GDK_SCROLL_UP:
      adjustListPointers(-4);
      break;
    case GDK_SCROLL_DOWN:
      adjustListPointers(4);
      break;
    default:
      // Do nothing in this case
      break;
  }

  auto fetched = getMemoryValues();
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

/**
 * @brief Reads memory values from Jimulator.
 * @return std::array<MemoryValues, 15> An array of the 15 memory values - their
 * addresses, their hex columns and their disassembly/source columns.
 */
std::array<MemoryValues, 15> DisassemblyModel::getMemoryValues() {
  auto vals = getJimulatorMemoryValues(rowIDTail);

  for (auto v : vals) {
    std::cout << intToFormattedHexString(v.address) << "/" << v.hex << "/"
              << v.disassembly << "/" << std::endl;
  }

  std::cout << std::endl;

  return vals;
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
