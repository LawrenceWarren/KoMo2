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

  // Gets the mnemonic OR an English "translation"
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
  std::vector<std::string> m(std::istream_iterator<std::string>{iss},
                             std::istream_iterator<std::string>());

  // If there is nothing to convert, return - should not be met
  if (m.size() <= 1) {
    return mnemonic;
  }

  // Sanitize all parameters
  for (auto& s : m) {
    s = sanitizeParamters(s);
  }

  m[0] = toLowerCase(m[0]);
  m = parseSWI(m);
  std::string outputText = "";
  tie(m, outputText) = parseLabel(m);

  try {
    return buildMnemonicString(outputText + mnemonicsMap.at(m[0]), m);
  } catch (std::out_of_range const&) {
    // ! Only happens in mnemonic is unrecognised - consider adding to the map
    return mnemonic;
  }
}

/**
 * @brief Parses the DEFB ARM mnemonic - since a single ARM command is broken
 * into a vector by the spaces between strings, the string that the DEFB command
 * defines will be split across several vector indexes. This function packs this
 * string back into a single index of the vector.
 * @param m The vector of strings making up the current ARM command.
 * @return const std::vector<std::string> The parsed vector of string making up
 * the current ARM command.
 */
const std::vector<std::string> DisassemblyModel::parseDEFB(
    std::vector<std::string> m) const {
  if (m[0] == "defb") {
    // Pack the string into the first paramter vector entry
    std::string s;
    for (long unsigned int i = 1; i < m.size(); i++) {
      s += m[i] + " ";
    }

    // replace anything past the final comma
    m[1] = std::regex_replace(s, std::regex(",[^,]*$"), "");
  }

  return m;
}

/**
 * @brief Parses the SWI ARM mnemonic - the SWI command always takes 1 paramter,
 * but means very different things depending on the parameter, meaning that a
 * single entry in `mnemonicsMap` for the SWI command cannot work. This function
 * packs the paramter of the SWI command in with the SWI key word for easier
 * lookup in the map, if the first key word in the vector is SWI.
 * @param m The vector of strings making up the current ARM command.
 * @return const std::vector<std::string> The parsed vector of strings making up
 * the current ARM command.
 */
const std::vector<std::string> DisassemblyModel::parseSWI(
    std::vector<std::string> m) const {
  if (m[0] == "swi") {
    m[0] += " " + m[1];
  }

  return m;
}

/**
 * @brief Parses labels from an ARM command. If the first index in the vector is
 * not a recognised keyword, then it must be a label. This function defines an
 * initial string that is only defined if the first index is a label. It then
 * updates the vector such that upon returning, the first index is the keyword.
 * It returns this parsed vector along with the new output text.
 * @param m The vector of strings making up the current ARM command.
 * @return const std::pair<std::vector<std::string>, std::string> A pair making
 * up the parsed vector and any output text that was derived from this function.
 */
const std::pair<std::vector<std::string>, std::string>
DisassemblyModel::parseLabel(std::vector<std::string> m) const {
  std::string out = "";

  try {
    auto t = mnemonicsMap.at(m[0]);  // If not present in the map, throws
  } catch (std::out_of_range const&) {
    out += "At label \"" + m[0] + "\", ";

    // Remove label from vector - m[1] becomes m[0]
    m.erase(m.begin());

    // The next element is now assumed to be the keyword.
    m[0] = toLowerCase(m[0]);
    m = parseSWI(m);
    m = parseDEFB(m);
  }

  return std::make_pair(m, out);
}

/**
 * @brief Builds the output mnemonics string from the vector making up the
 * current ARM command and the value read from the map. See the comment for
 * mnemonicMap for details on the syntax of the value read from the map.
 * @param s The value read from the map.
 * @param m The vector making up the current ARM command.
 * @return const std::string The mnemonic, translated into English.
 */
const std::string DisassemblyModel::buildMnemonicString(
    std::string s,
    std::vector<std::string> m) const {
  for (long unsigned int i = 1; i < m.size(); i++) {
    // Builds the regex match string
    std::string reg = "@" + std::to_string(i) + "@";
    // Replaces the regex match string with the parameter
    s = std::regex_replace(s, std::regex(reg), m[i]);
  }

  return s;
}

/**
 * @brief Sanitizes any of the paramters used with an ARM mnemonic.
 * @param param The parameter to be sanitized.
 * @return const std::string The sanitized paramter.
 */
const std::string DisassemblyModel::sanitizeParamters(std::string param) const {
  if (param[0] == 'R' || param[0] == 'r') {
    return "Register " + param.erase(0, 1);
  } else if (param[0] == '#') {
    return param.erase(0, 1);
  } else {
    return param;
  }
}

/**
 * @brief Converts a string to lower case.
 * @param s The string to convert to lower case.
 * @return const std::string The lower case string.
 */
const std::string DisassemblyModel::toLowerCase(std::string s) const {
  for (auto& c : s) {
    c = ::tolower(c);
  }

  return s;
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
      refreshViews();  // Also rebuilds the screenreader strings
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
