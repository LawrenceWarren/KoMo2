/**
 * @file DisassemblyModel.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief This file defines the functions declared in the file
 * `DisassemblyModel.h`.
 * @version 1.0.0
 * @date 10-04-2021
 */

#include <ctype.h>
#include <iomanip>
#include <iostream>
#include <regex>
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
 * direct index - the first row is 0, the last row is 11 - so we can combine the
 * row ID with the `memoryIndex` variable to identify what address the
 * breakpoint should be set at.
 */
void DisassemblyModel::onBreakpointToggle(DisassemblyRows* const row) {
  row->setBreakpoint(Jimulator::setBreakpoint(row->getAddressVal()));
  const auto s = buildDisassemblyRowAccessibilityString(*row);
  row->get_accessible()->set_description(s);
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
  for (long unsigned int i = 0; i < vals.size(); i++) {
    auto& row = (*rows)[i];
    auto flag = row.get_state_flags();

    updateCSSFlags(flag, row, vals[i].address);
    row.setAddressVal(vals[i].address);
    row.setAddress(intToFormattedHexString(vals[i].address));
    row.setHex(vals[i].hex);
    row.setDisassembly(vals[i].disassembly);
    row.setBreakpoint(vals[i].breakpoint);

    const auto s = buildDisassemblyRowAccessibilityString(row);
    row.get_accessible()->set_description(s);
  }
}

/**
 * @brief Generates the string to set for the accessibility model.
 * @param row The row to build to accessibility string from.
 * @return const std::string The accessibility string.
 */
const std::string DisassemblyModel::buildDisassemblyRowAccessibilityString(
    DisassemblyRows& row) {
  // Gets a string describing the state of the breakpoint
  // Used for the accessibility object
  std::string bp = row.getBreakpoint() ? "breakpoint set" : "no breakpoint";

  // Removes leading 0's from addresses
  std::stringstream gHex;
  gHex << std::hex << row.getAddress();
  const auto addr = std::regex_replace(gHex.str(), std::regex("^0x0{0,7}"), "");

  const std::string disassemblyInfo =
      englishMnemonic ? convertMnemonicToEnglish(row.getDisassembly())
                      : row.getDisassembly();

  std::stringstream ss;
  ss << "address " << addr << ", " << disassemblyInfo << bp;

  return ss.str();
}

/**
 * @brief Converts a mnemonic into plain English.
 * @param mnemonic An ARM instruction mnemonic.
 * @return const std::string The input parameter converted into English.
 */
const std::string DisassemblyModel::convertMnemonicToEnglish(
    const std::string mnemonic) const {
  // Splits mnemonic string into an array, at the space
  std::istringstream iss(mnemonic);
  std::vector<std::string> results(std::istream_iterator<std::string>{iss},
                                   std::istream_iterator<std::string>());

  // If there is nothing to convert, return
  if (results.size() <= 1) {
    return mnemonic;
  }

  bool isLabel = false;

  // Identify any labels
  for (auto c : results[0]) {
    isLabel = isLabel | islower(c);
  }

  std::string outputText = "";

  // If is label, specify
  if (isLabel) {
    outputText += "At label \"" + results[0] + "\", ";
    results.erase(results.begin());
  }

  if (results[0] == "DEFB") {
    return outputText + parseDEFB(results);
  }

  // Parse text
  switch (results.size()) {
    case 2:
      return outputText + parse1Param(results);
    case 3:
      return outputText + parse2Param(results);
    case 4:
      return outputText + parse3Param(results);
    case 5:
      return outputText + parse4Param(results);
    default:
      return mnemonic;
  }
}

/**
 * @brief Parses a DEFB disassembly line.
 * @param v A vector of strings that made up the line.
 * @return const std::string A plain English string describing the contents of
 * the line.
 */
const std::string DisassemblyModel::parseDEFB(
    const std::vector<std::string> v) const {
  std::string s = "defined a string ";
  for (long unsigned int i = 1; i < v.size(); i++) {
    s += v[i] + " ";
  }

  return std::regex_replace(s, std::regex(",[^,]*$"), "");
}

/**
 * @brief Parse an ARM instruction that takes 1 additional parameter.
 * @param v A vector of strings that made up a disassembly line.
 * @return const std::string A plain English string describing the contents of
 * the line.
 */
const std::string DisassemblyModel::parse1Param(
    std::vector<std::string> v) const {
  v[1] = sanitizeParamters(v[1]);

  if (v[0] == "SWI") {
    switch (std::stoi(v[1])) {
      case 0:
        return "Printing character";
      case 1:
        return "Reading character";
      case 2:
        return "Halting execution";
      case 3:
        return "Printing string";
      case 4:
        return "Printing integer";
      default:
        return v[0] + " " + v[1];
    }
  } else if (v[0] == "DEFW") {
    return "defined as integer " + v[1];
  } else if (v[0] == "BEQ") {
    return "Branch to label \"" + v[1] + "\" if equal";
  } else if (v[0] == "BLT") {
    return "Branch to label \"" + v[1] + "\" if less than";
  } else if (v[0] == "BNE") {
    return "Branch to label \"" + v[1] + "\" if not equal";
  } else if (v[0] == "BGT") {
    return "Branch to label \"" + v[1] + "\" if greater than";
  } else if (v[0] == "B") {
    return "Branch to label \"" + v[1] + "\"";
  }

  return v[0] + " " + v[1];
}

/**
 * @brief Parse an ARM instruction that takes 2 additional parameters.
 * @param v A vector of strings that made up a disassembly line.
 * @return const std::string A plain English string describing the contents of
 * the line.
 */
const std::string DisassemblyModel::parse2Param(
    std::vector<std::string> v) const {
  v[1] = sanitizeParamters(v[1]);
  v[2] = sanitizeParamters(v[2]);

  if (v[0] == "MOV") {
    return "Moves " + v[2] + " into " + v[1];
  } else if (v[0] == "ADR" || v[0] == "ADRL") {
    return "Value at label " + v[2] + " moves into " + v[1];
  } else if (v[0] == "CMP") {
    return "Compares " + v[1] + " to " + v[2];
  } else if (v[0] == "CMN") {
    return "Negatively compares " + v[1] + " to " + v[2];
  } else if (v[0] == "STR") {
    return "Stores " + v[1] + " in " + v[2];
  } else if (v[0] == "LDR") {
    return "Stores " + v[2] + " in " + v[1];
  } else {
    return v[0] + " " + v[1] + " " + v[2];
  }
}

/**
 * @brief Parse an ARM instruction that takes 3 additional parameters.
 * @param v A vector of strings that made up a disassembly line.
 * @return const std::string A plain English string describing the contents of
 * the line.
 */
const std::string DisassemblyModel::parse3Param(
    std::vector<std::string> v) const {
  v[1] = sanitizeParamters(v[1]);
  v[2] = sanitizeParamters(v[2]);
  v[3] = sanitizeParamters(v[3]);

  if (v[0] == "SUB") {
    return "Subtract " + v[3] + " from " + v[2] + " and store in " + v[1];
  } else if (v[0] == "ADD") {
    return "Add " + v[3] + " from " + v[2] + " and store in " + v[1];
  } else if (v[0] == "MUL") {
    return "Multiply " + v[3] + " from " + v[2] + " and store in " + v[1];
  } else if (v[0] == "AND") {
    return "Bitwise AND " + v[2] + " with " + v[3] + " and store in " + v[1];
  } else if (v[0] == "ORR") {
    return "Bitwise OR " + v[2] + " with " + v[3] + " and store in " + v[1];
  }

  return v[0] + " " + v[1] + " " + v[2] + " " + v[3];
}

/**
 * @brief Parse an ARM instruction that takes 4 additional parameters.
 * @param v A vector of strings that made up a disassembly line.
 * @return const std::string A plain English string describing the contents of
 * the line.
 */
const std::string DisassemblyModel::parse4Param(
    std::vector<std::string> v) const {
  v[1] = sanitizeParamters(v[1]);
  v[2] = sanitizeParamters(v[2]);
  v[3] = sanitizeParamters(v[3]);
  v[4] = sanitizeParamters(v[4]);

  if (v[0] == "MLA") {
    return "Multiplying " + v[2] + "  with " + v[3] + " adding " + v[4] +
           " and storing in " + v[1];
  } else if (v[0] == "MLS") {
    return "Multiplying " + v[2] + "  with " + v[3] + " subtracting " + v[4] +
           " and storing in " + v[1];
  }

  return v[0] + " " + v[1] + " " + v[2] + " " + v[3] + " " + v[4];
}

/**
 * @brief Sanitizes any of the paramters used with an ARM mnemonic.
 * @param param The parameter to be sanitized.
 * @return const std::string The sanitized paramter.
 */
const std::string DisassemblyModel::sanitizeParamters(std::string param) const {
  if (param[0] == 'R') {
    return "Register " + param.erase(0, 1);
  } else if (param[0] == '#') {
    return param.erase(0, 1);
  } else {
    return param;
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
  // If this is the address in program counter:
  if (intToFormattedHexString(address) == PCValue) {
    // Make it highlighted if it is not focused
    if (flag == NORMAL) {
      row.set_state_flags(PC_ADDRESS);
    }
    // Highlight differently if it is focused
    else if (flag == (FOCUSED)) {
      row.set_state_flags(PC_ADDRESS_FOCUSED);
    }
  }
  // If this is NOT the address in the program counter:
  else {
    // If it is not focused, remove highlight
    if (flag == PC_ADDRESS) {
      row.set_state_flags(NORMAL);
    }
    // If it is focused, set the focus highlight only
    else if (flag == (PC_ADDRESS_FOCUSED)) {
      row.set_state_flags(FOCUSED);
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
  // if alt is held
  if ((e->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK)) ==
      GDK_MOD1_MASK) {
    // toggle mnemonics mode, else stop
    if (e->keyval == GDK_KEY_m || e->keyval == GDK_KEY_M) {
      setEnglishMnemonic(not englishMnemonic);
      refreshViews(); // Also rebuilds the screenreader strings
    } else {
      return false;
    }
  }

  auto rows = getView()->getRows();

  // If the top row has focus and it's a key press up, handle it
  if ((*rows)[0].has_focus() && e->keyval == GDK_KEY_Up) {
    auto scroll = GdkEventScroll();
    scroll.direction = GDK_SCROLL_UP;
    handleScroll(&scroll);
    return true;
  }

  // If the bottom row has focus and it's a key press down, handle it
  else if ((*rows)[rows->size() - 1].has_focus() && e->keyval == GDK_KEY_Down) {
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
 * @return std::array<Jimulator::MemoryValues, 13> An array of the 13 memory
 * values - their addresses, their hex columns and their disassembly/source
 * columns.
 */
const std::array<Jimulator::MemoryValues, 13>
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
/**
 * @brief Set the value of the englishMnemonic member variable.
 * @param val The value to set englishMnemonic to.
 */
void DisassemblyModel::setEnglishMnemonic(const bool val) {
  englishMnemonic = val;
}
