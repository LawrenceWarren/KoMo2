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
#include "../models/DisassemblyModel.h"

/**
 * @brief Construct a new DisassemblyView::DisassemblyView object.
 * @param parent A pointer to this views parent, set during initialisation.
 */
DisassemblyView::DisassemblyView(MainWindowView* const parent)
    : parent(parent) {
  initDisassemblyContainer();
}

/**
 * @brief Initialises the containers & their children - packs children into
 * containers, sets sizes and layouts, adds CSS classes.
 */
void DisassemblyView::initDisassemblyContainer() {
  container.set_layout(Gtk::BUTTONBOX_START);
  disassemblyContainer.set_layout(Gtk::BUTTONBOX_START);

  packView();

  // Pack in to the master container
  container.pack_start(disassemblyContainer, false, false);
  container.pack_start(navigationButtons, false, false);

  // Show everything
  disassemblyContainer.show();
  disassemblyContainer.show_all_children();
  navigationButtons.show();
  navigationButtons.show_all_children();
  container.show();
  container.show_all_children();

  // Show everything
  add(container);
  show();
  show_all_children();
}

/**
 * @brief Gets a pointer to the rows of views.
 * @return std::vector<DisassemblyRows>* const A pointer to the vector of rows.
 */
std::vector<DisassemblyRows>* const DisassemblyView::getRows() {
  return &rows;
}

/**
 * @brief Packing in 15 DisassemblyRows into the disassemblyContainer view.
 */
void DisassemblyView::packView() {
  for (long unsigned int i = 0; i < 15; i++) {
    disassemblyContainer.pack_start(rows[i], false, false);
    rows[i].show();
    rows[i].show_all_children();
  }
}

/**
 * @brief Set the values in the disassemblyRows.
 * @param vals An array of 15 memoryValues.
 */
void DisassemblyView::refreshViews(
    std::array<Jimulator::MemoryValues, 15> vals) {
  for (int i = 0; i < 15; i++) {
    rows[i].setAddress(getModel()->intToFormattedHexString(vals[i].address));
    rows[i].setHex(vals[i].hex);
    rows[i].setDisassembly(vals[i].disassembly);
  }
}

// !!!!!!!!!!!!!!!!!!!!!!!
// ! Getters and setters !
// !!!!!!!!!!!!!!!!!!!!!!!

/**
 * @brief Sets the value of the `model` member.
 * @param val The value to set the `model` member to.
 */
void DisassemblyView::setModel(DisassemblyModel* const val) {
  model = val;
}
/**
 * @brief Gets a pointer to the navigation buttons container.
 * @return Gtk::VButtonBox* const A constant pointer to the navigation buttons
 * container.
 */
Gtk::VButtonBox* const DisassemblyView::getNavigationButtons() {
  return &navigationButtons;
}
/**
 * @brief Gets a pointer to the disassembly container.
 * @return Gtk::VButtonBox* const A constant pointer to the disassembly
 * container.
 */
Gtk::VButtonBox* const DisassemblyView::getDisassemblyContainer() {
  return &disassemblyContainer;
}
/**
 * @brief Gets a pointer to the master container.
 * @return Gtk::HButtonBox* const A constant pointer to the master container.
 */
Gtk::HButtonBox* const DisassemblyView::getContainer() {
  return &container;
}
/**
 * @brief Gets a constant pointer to the model member.
 * @return DisassemblyModel* const A constant pointer to the model member.
 */
DisassemblyModel* const DisassemblyView::getModel() const {
  return model;
}

// !!!!!!!!!!!!!!!!!!!!!!
// ! Nested class stuff !
// !!!!!!!!!!!!!!!!!!!!!!

/**
 * @brief Construct a new DisassemblyRows::DisassemblyRows object.
 */
DisassemblyRows::DisassemblyRows() {
  set_layout(Gtk::BUTTONBOX_START);

  // Set minimum sizes
  breakpoint.set_size_request(5, 5);
  buttonSizer.set_size_request(5, 5);
  address.set_size_request(100, 10);
  hex.set_size_request(100, 10);
  disassembly.set_size_request(400, 10);

  // Packing objects
  buttonSizer.pack_start(breakpoint, false, false);
  pack_start(buttonSizer, false, false);
  pack_start(address, false, false);
  pack_start(hex, false, false);
  pack_start(disassembly, false, false);

  // Add CSS styles
  get_style_context()->add_class("disassembly_rows");
  breakpoint.get_style_context()->add_class("breakpoint_buttons");
  address.get_style_context()->add_class("address_label");
  hex.get_style_context()->add_class("hex_label");
  disassembly.get_style_context()->add_class("disassembly_label");

  // Set text align to the left
  address.set_xalign(0.2);
  hex.set_xalign(0.2);
  disassembly.set_xalign(0.2);

  show();
  show_all_children();
}
/**
 * @brief Set the state of the breakpoint button.
 * @param state The state to set the breakpoint to.
 */
void DisassemblyRows::setBreakpoint(const bool state) {
  breakpoint.set_active(state);
}
/**
 * @brief Set the text of the address label.
 * @param text The text to set the label to.
 */
void DisassemblyRows::setAddress(const std::string text) {
  address.set_text(text);
  breakpoint.get_accessible()->set_description(text);
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
