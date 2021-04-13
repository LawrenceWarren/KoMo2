/**
 * @file MainWindowView.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief Definitions of the functions declared in the class definition, found
 * at `MainWindowView.h`.
 * @version 1.0.0
 * @date 2020-12-28
 */

#include "MainWindowView.h"
#include <atkmm/object.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/image.h>
#include <iostream>
#include <string>
#include "../models/KoMo2Model.h"

/**
 * @brief Construct a new MainWindowView object.
 * @param x The width of the window.
 * @param y The height of the window.
 */
MainWindowView::MainWindowView(const int x, const int y)
    : masterLayout(Gtk::Orientation::ORIENTATION_VERTICAL, 0),
      controlsAndCompileBar(),
      compileLoadView(this),
      controlsView(this),
      registersView(this),
      terminalView(this),
      disassemblyView(this) {
  set_border_width(1);
  // set_default_size(x, y);
  set_gravity(Gdk::GRAVITY_WEST);
  initControlsAndCompileBar();
  initRegistersAndDisassemblyBar();
  initMasterLayout();
}

/**
 * @brief Initialise the controls and compile bar.
 */
void MainWindowView::initControlsAndCompileBar() {
  getControlsAndCompileBar()->set_layout(Gtk::BUTTONBOX_EDGE);
  getControlsAndCompileBar()->pack_start(controlsView, false, false);
  getControlsAndCompileBar()->pack_end(compileLoadView, false, false);
}

/**
 * @brief Initialise the registers and disassembly bar.
 */
void MainWindowView::initRegistersAndDisassemblyBar() {
  getRegistersAndDisassemblyBar()->set_layout(Gtk::BUTTONBOX_START);
  getRegistersAndDisassemblyBar()->pack_end(registersView, false, false);
  getRegistersAndDisassemblyBar()->pack_end(disassemblyView, false, false);
}

/**
 * @brief Initialise the master layout.
 */
void MainWindowView::initMasterLayout() {
  getMasterLayout()->set_homogeneous(false);
  getMasterLayout()->add(controlsAndCompileBar);
  getMasterLayout()->add(registersAndDisassemblyBar);
  getMasterLayout()->add(terminalView);
  getMasterLayout()->show_all_children();
  getMasterLayout()->show();
  add(masterLayout);
}

/**
 * @brief Sets the style attributes for the views - namely any icons and CSS.
 */
void MainWindowView::setStyling() {
  set_title("KoMo2");

  // Sets the icon for the window
  set_icon_from_file(getModel()->getAbsolutePathToProjectRoot() +
                     "res/img/komo2Logo.png");

  // Create a css provider, get the style context, load the css file
  auto ctx = get_style_context();
  auto css = Gtk::CssProvider::create();
  css->load_from_path(getModel()->getAbsolutePathToProjectRoot() +
                      "res/styles.css");

  // Adds a CSS class for the main window
  get_style_context()->add_class("mainWindow");

  // Adds a CSS class for the layouts
  getControlsView()->get_style_context()->add_class("layouts");
  getControlsAndCompileBar()->get_style_context()->add_class("layouts");
  getCompileLoadView()->get_style_context()->add_class("layouts");
  getRegistersView()->get_style_context()->add_class("layouts");
  getDisassemblyView()->get_style_context()->add_class("layouts");
  getTerminalView()->get_style_context()->add_class("layouts");

  // Add the CSS to the screen
  ctx->add_provider_for_screen(Gdk::Screen::get_default(), css,
                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

// !!!!!!!!!!!!!!!!!!!!!!!
// ! Getters and setters !
// !!!!!!!!!!!!!!!!!!!!!!!

/**
 * @brief Returns a pointer to the registersView object.
 * @return RegistersView* const The registersView object.
 */
RegistersView* const MainWindowView::getRegistersView() {
  return &registersView;
}
/**
 * @brief Get the CompileLoadView.
 * @return CompileLoadView* A pointer to the compileLoadView.
 */
CompileLoadView* const MainWindowView::getCompileLoadView() {
  return &compileLoadView;
}
/**
 * @brief Get the ControlsView.
 * @return ControlsView* A pointer to the controlsView.
 */
ControlsView* const MainWindowView::getControlsView() {
  return &controlsView;
}
/**
 * @brief Get the TerminalView.
 * @return ControlsView* A pointer to the controlsView.
 */
TerminalView* const MainWindowView::getTerminalView() {
  return &terminalView;
}
/**
 * @brief Get the DisassemblyView.
 * @return ControlsView* A pointer to the controlsView.
 */
DisassemblyView* const MainWindowView::getDisassemblyView() {
  return &disassemblyView;
}
/**
 * @brief Gets the `model` member variable.
 * @return KoMo2Model* A pointer to the `model` member variable.
 */
KoMo2Model* const MainWindowView::getModel() const {
  return model;
}
/**
 * @brief Sets the `model` member variable.
 * @param val The pointer to set the model member variable to.
 */
void MainWindowView::setModel(KoMo2Model* const val) {
  model = val;
}
/**
 * @brief Gets the controlsAndCompileBar layout.
 * @return Gtk::HButtonBox* const The controlsAndCompileBar layout.
 */
Gtk::HButtonBox* const MainWindowView::getControlsAndCompileBar() {
  return &controlsAndCompileBar;
}
/**
 * @brief Gets the registersAndSiassemblyBar layout.
 * @return Gtk::HButtonBox* const The registersAndSiassemblyBar layout.
 */
Gtk::HButtonBox* const MainWindowView::getRegistersAndDisassemblyBar() {
  return &registersAndDisassemblyBar;
}
/**
 * @brief Gets the masterLayout layout.
 * @return Gtk::VButtonBox* const The masterLayout layout.
 */
Gtk::Box* const MainWindowView::getMasterLayout() {
  return &masterLayout;
}
