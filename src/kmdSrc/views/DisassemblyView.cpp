#include "DisassemblyView.h"

DisassemblyView::DisassemblyView(MainWindowView* const parent)
    : parent(parent) {
  initDisassemblyContainer();
}

void DisassemblyView::initDisassemblyContainer() {
  packView(false);

  container.pack_end(disassemblyContainer, false, false);
  container.pack_end(navigationButtons, false, false);

  disassemblyContainer.show();
  disassemblyContainer.show_all_children();
  navigationButtons.show();
  navigationButtons.show_all_children();
  container.show();
  container.show_all_children();

  add(container);
  show();
  show_all_children();
}

std::vector<DisassemblyRows>* const DisassemblyView::getRows() {
  return &rows;
}

void DisassemblyView::packView(const bool emptyChild) {
  // Clear existing children
  for (long unsigned int i = 0; emptyChild && i < rows.size(); i++) {
    disassemblyContainer.remove(rows[i]);
  }

  // Packs in 15 children
  for (long unsigned int i = 0; i < 15; i++) {
    disassemblyContainer.pack_start(rows[i], false, false);
    rows[i].show();
    rows[i].show_all_children();
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

void DisassemblyView::refreshViews() {
  
}

// !!!!!!!!!!!!!!!!!!!!!!
// ! Nested class stuff !
// !!!!!!!!!!!!!!!!!!!!!!

DisassemblyRows::DisassemblyRows() {
  set_layout(Gtk::BUTTONBOX_CENTER);
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
