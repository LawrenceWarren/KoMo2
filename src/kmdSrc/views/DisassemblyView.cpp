/**
 * @file DisassemblyView.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief This file contains the source code for the DisassemblyView GTK GUI
 * component & the DisassemblyRows GUI sub-component, for the `KoMo2` program.
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

#include "DisassemblyView.h"
#include <sstream>
#include "../models/DisassemblyModel.h"

/**
 * @brief Construct a new DisassemblyView::DisassemblyView object.
 * @param parent A pointer to this views parent, set during initialisation.
 */
DisassemblyView::DisassemblyView(MainWindowView* const parent)
    : parent(parent) {
  initDisassemblyRows();
  initDisassemblyContainer();
}

/**
 * @brief Initialises the containers & their children - packs children into
 * containers, sets sizes and layouts, adds a CSS class.
 */
void DisassemblyView::initDisassemblyContainer() {
  disassemblyContainer.set_layout(Gtk::BUTTONBOX_START);
  get_style_context()->add_class("disassemblyContainer");
  add(disassemblyContainer);
  show();
  show_all_children();
}

/**
 * @brief Packs the 15 disassemblyRows into their container and gives them a
 * pointer to the DisassemblyModel object.
 */
void DisassemblyView::initDisassemblyRows() {
  for (long unsigned int i = 0; i < 15; i++) {
    disassemblyContainer.pack_start(rows[i], false, false);
    rows[i].setModel(model);
  }
}

// !!!!!!!!!!!!!!!!!!!!!!!
// ! Getters and setters !
// !!!!!!!!!!!!!!!!!!!!!!!

/**
 * @brief Gets a pointer to the rows of views.
 * @return std::vector<DisassemblyRows>* const A pointer to the vector of rows.
 */
std::vector<DisassemblyRows>* const DisassemblyView::getRows() {
  return &rows;
}
/**
 * @brief Sets the value of the `model` member.
 * @param val The value to set the `model` member to.
 */
void DisassemblyView::setModel(DisassemblyModel* const val) {
  model = val;
}
/**
 * @brief Gets a constant pointer to the model member.
 * @return DisassemblyModel* const A constant pointer to the model member.
 */
DisassemblyModel* const DisassemblyView::getModel() const {
  return model;
}
/**
 * @brief Return a constant pointer to the parent member.
 * @return MainWindowView* const  A constat pointer to the parent member.
 */
MainWindowView* const DisassemblyView::getParent() const {
  return parent;
}

// !!!!!!!!!!!!!!!!!!!!!!
// ! Nested class stuff !
// !!!!!!!!!!!!!!!!!!!!!!

/**
 * @brief Construct a new DisassemblyRows::DisassemblyRows object.
 */
DisassemblyRows::DisassemblyRows()
    : Box(Gtk::Orientation::ORIENTATION_HORIZONTAL, 0) {
  initBreakpoint();
  initAddress();
  initHex();
  initDisassembly();

  set_spacing(0);

  buttonSizer.set_size_request(5, 5);
  buttonSizer.pack_start(breakpoint, false, false);
  add(buttonSizer);
  add(address);
  add(hex);
  add(disassembly);
  get_style_context()->add_class("disassemblyRows");
  set_can_focus(true);
  set_focus_on_click(true);
  show();
  show_all_children();
}

/**
 * @brief Initialises the breakpoint button.
 */
void DisassemblyRows::initBreakpoint() {
  breakpoint.set_size_request(5, 5);
  breakpoint.get_style_context()->add_class("breakpointButtons");
  breakpoint.set_tooltip_text("Toggle breakpoint");
  breakpoint.set_can_focus(false);
}
/**
 * @brief Initialises the address label.
 */
void DisassemblyRows::initAddress() {
  address.get_style_context()->add_class("disassemblyLabels");
  address.set_size_request(100, 10);
  address.set_xalign(0.0);
}
/**
 * @brief Initialises the hex label.
 */
void DisassemblyRows::initHex() {
  hex.get_style_context()->add_class("disassemblyLabels");
  hex.set_size_request(100, 10);
  hex.set_xalign(0.0);
}
/**
 * @brief Initialises the disassembly label.
 */
void DisassemblyRows::initDisassembly() {
  disassembly.get_style_context()->add_class("disassemblyLabels");
  disassembly.set_size_request(500, 10);
  disassembly.set_xalign(0.0);
}

// ! Getters and setters

/**
 * @brief Set the state of the breakpoint button. This is a little unusual - it
 * sets the state in terms of CSS state, making the button appear either toggled
 * or un-toggled.
 * @param state The state to set the breakpoint to.
 */
void DisassemblyRows::setBreakpoint(const bool state) {
  if (state) {
    breakpoint.set_state_flags(Gtk::STATE_FLAG_CHECKED);
  } else {
    breakpoint.set_state_flags(Gtk::STATE_FLAG_NORMAL);
  }
}
/**
 * @brief Set the text of the address label.
 * @param text The text to set the label to.
 */
void DisassemblyRows::setAddress(const std::string text) {
  address.set_text(text);
}
/**
 * @brief Set the text of the hex label.
 * @param text The text to set the hex label to.
 */
void DisassemblyRows::setHex(const std::string text) {
  hex.set_text(text);
}
/**
 * @brief Set the text of disassembly label.
 * @param text The text to set the disassembly label to.
 */
void DisassemblyRows::setDisassembly(const std::string text) {
  disassembly.set_text(text);
}
/**
 * @brief Gets a constant pointer to the breakpoint button member.
 * @return Gtk::ToggleButton* const A constant pointer to the breakpoint button
 * member.
 */
Gtk::ToggleButton* const DisassemblyRows::getButton() {
  return &breakpoint;
}
/**
 * @brief Sets the value of the model pointer.
 * @param val The value to set the model pointer to.
 */
void DisassemblyRows::setModel(DisassemblyModel* const val) {
  model = val;
}
/**
 * @brief Get the text in the address box.
 * @return const std::string The text in the address box.
 */
const std::string DisassemblyRows::getAddress() const {
  return address.get_text();
}
/**
 * @brief Get the addressVal member.
 * @return const uint32_t The addressVal member.
 */
const uint32_t DisassemblyRows::getAddressVal() const {
  return addressVal;
}
/**
 * @brief Set the value of addressVal member.
 * @param val The value to set it to.
 */
void DisassemblyRows::setAddressVal(const uint32_t val) {
  addressVal = val;
}
/**
 * @brief Returns if a breakpoint is set or not.
 * @return true if the breakpoint is set.
 * @return false if the breakpoint is not set.
 */
const bool DisassemblyRows::getBreakpoint() {
  return breakpoint.get_state_flags() ==
         (Gtk::STATE_FLAG_CHECKED | Gtk::STATE_FLAG_DIR_LTR);
}
/**
 * @brief Gets the disassembly text for the breakpoint row.
 * @return const std::string The disassembly text.
 */
const std::string DisassemblyRows::getDisassembly() {
  return disassembly.get_text();
}
