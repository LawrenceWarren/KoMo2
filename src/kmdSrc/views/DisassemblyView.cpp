#include "DisassemblyView.h"
#include "../models/DisassemblyModel.h"

/**
 * @brief Construct a new DisassemblyView::DisassemblyView object.
 * @param parent A pointer to this views parent.
 */
DisassemblyView::DisassemblyView(MainWindowView* const parent)
    : parent(parent) {
  initDisassemblyContainer();
}

/**
 * @brief Initialises the containers & views.
 */
void DisassemblyView::initDisassemblyContainer() {
  packView();

  // Pack in to the master container
  container.pack_end(disassemblyContainer, false, false);
  container.pack_end(navigationButtons, false, false);

  // Set layouts
  disassemblyContainer.set_layout(Gtk::BUTTONBOX_START);
  container.set_layout(Gtk::BUTTONBOX_START);

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
 * @param vals
 */
void DisassemblyView::refreshViews(std::array<MemoryValues, 15> vals) {
  for (int i = 0; i < 15; i++) {
    rows[i].setAddress(getModel()->intToFormattedHexString(vals[i].address));
    rows[i].setHex(vals[i].hex);
    rows[i].setDisassembly(vals[i].disassembly);
  }
}

// !!!!!!!!!!!!!!!!!!!!!!!
// ! Getters and setters !
// !!!!!!!!!!!!!!!!!!!!!!!

void DisassemblyView::setModel(DisassemblyModel* const val) {
  model = val;
}
Gtk::VButtonBox* const DisassemblyView::getNavigationButtons() {
  return &navigationButtons;
}
Gtk::VButtonBox* const DisassemblyView::getDisassemblyContainer() {
  return &disassemblyContainer;
}
Gtk::HButtonBox* const DisassemblyView::getContainer() {
  return &container;
}
DisassemblyModel* const DisassemblyView::getModel() const {
  return model;
}

// !!!!!!!!!!!!!!!!!!!!!!
// ! Nested class stuff !
// !!!!!!!!!!!!!!!!!!!!!!

DisassemblyRows::DisassemblyRows() {
  set_layout(Gtk::BUTTONBOX_START);
  pack_end(breakpoint, false, false);
  pack_end(address, false, false);
  pack_end(hex, false, false);
  pack_end(disassembly, false, false);

  breakpoint.set_size_request(10, 10);

  show();
  show_all_children();
}
void DisassemblyRows::setBreakpoint(const bool state) {
  breakpoint.set_active(state);
}
void DisassemblyRows::setAddress(const std::string text) {
  address.set_text(text);
}
void DisassemblyRows::setHex(const std::string text) {
  hex.set_text(text);
}
void DisassemblyRows::setDisassembly(const std::string text) {
  disassembly.set_text(text);
}
