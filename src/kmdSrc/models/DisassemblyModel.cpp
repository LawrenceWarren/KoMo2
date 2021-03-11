#include "DisassemblyModel.h"
#include <iostream>
#include "../views/DisassemblyView.h"

int DisassemblyModel::rowIDTail = 0;
int DisassemblyModel::rowIDHead = 56;

DisassemblyModel::DisassemblyModel(DisassemblyView* const view,
                                   KoMo2Model* const parent)
    : Model(parent), view(view) {
  view->setModel(this);
  addScrollRecognition();
  initialiseRowViews();
}

void DisassemblyModel::addScrollRecognition() {
  getView()->add_events(Gdk::SMOOTH_SCROLL_MASK);
  getView()->signal_scroll_event().connect(
      sigc::mem_fun(*this, &DisassemblyModel::handleScroll), false);
}

void DisassemblyModel::initialiseRowViews() {
  for (long unsigned int i = 0; i < rowModels.size(); i++) {
    rowModels[i] = RowModel(false, std::to_string(rowIDTail), "00 00 00 00 ",
                            "Extra long text here for good luck.");

    (*getView()->getRows())[i].setBreakpoint(rowModels[i].getBreakpoint());
    (*getView()->getRows())[i].setAddress(rowModels[i].getAddress());
    (*getView()->getRows())[i].setHex(rowModels[i].getHex());
    (*getView()->getRows())[i].setDisassembly(rowModels[i].getDisassembly());

    rowIDTail += 4;
  }

  rowIDTail = 0;
}

/**
 * @brief Passes the key press event off to other child models.
 * @param e The key press event.
 * @return bool if a key was pressed.
 */
const bool DisassemblyModel::handleScroll(GdkEventScroll* e) {
  switch (e->direction) {
    case GDK_SCROLL_UP: {
      std::rotate(rowModels.begin(), rowModels.begin() + 1, rowModels.end());
      std::rotate(getView()->getRows()->begin(),
                  getView()->getRows()->begin() + 1,
                  getView()->getRows()->end());
      // TODO: setup rolling ID's
      break;
    }
    case GDK_SCROLL_DOWN: {
      std::rotate(rowModels.rbegin(), rowModels.rbegin() + 1, rowModels.rend());
      std::rotate(getView()->getRows()->rbegin(),
                  getView()->getRows()->rbegin() + 1,
                  getView()->getRows()->rend());
      // TODO: setup rolling ID's
      break;
    }
    default:
      break;
  }

  getView()->packView(true);
  return true;
}

void DisassemblyModel::changeJimulatorState(const JimulatorState newState) {}

/**
 * @brief Handles any key press events.
 * @param e The key press event.
 * @return bool true if the key press was handled.
 */
const bool DisassemblyModel::handleKeyPress(const GdkEventKey* const e) {
  return false;
}

DisassemblyView* const DisassemblyModel::getView() {
  return view;
}

// !!!!!!!!!!!!!!!!!!!!!
// ! Child class stuff !
// !!!!!!!!!!!!!!!!!!!!!

RowModel::RowModel(const bool breakpoint,
                   const std::string address,
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
const std::string RowModel::getAddress() const {
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
void RowModel::setAddress(const std::string text) {
  address = text;
}
void RowModel::setHex(const std::string text) {
  hex = text;
}
void RowModel::setDisassembly(const std::string text) {
  disassembly = text;
}
