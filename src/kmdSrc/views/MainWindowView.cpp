/**
 * @file MainWindowView.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief Definitions of the functions declared in the class definition, found
 * at `MainWindowView.h`.
 * @version 0.1
 * @date 2020-12-28
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
    : masterLayout(),
      controlsAndCompileBar(),
      compileLoadView(this),
      controlsView(this),
      registersView(this),
      terminalView(this),
      disassemblyView(this) {
  std::cout << "Main window view..." << std::endl;

  std::cout << "1 Main window view..." << std::endl;
  set_border_width(1);
  std::cout << "2 Main window view..." << std::endl;
  set_default_size(x, y);
  std::cout << " 3Main window view..." << std::endl;
  set_gravity(Gdk::GRAVITY_WEST);
  std::cout << "4 Main window view..." << std::endl;
  getMasterLayout()->set_layout(Gtk::BUTTONBOX_START);
  std::cout << " 5 Main window view..." << std::endl;

  // Packs containers into one another.
  getControlsAndCompileBar()->set_layout(Gtk::BUTTONBOX_START);
  std::cout << "6 Main window view..." << std::endl;
  getControlsAndCompileBar()->pack_end(controlsView, false, false);
  std::cout << "7 Main window view..." << std::endl;
  getControlsAndCompileBar()->pack_end(compileLoadView, false, false);
  std::cout << "8 Main window view..." << std::endl;
  getRegistersAndDisassemblyBar()->set_layout(Gtk::BUTTONBOX_START);
  std::cout << "9 Main window view..." << std::endl;
  getRegistersAndDisassemblyBar()->pack_end(registersView, false, false);
  std::cout << "10 Main window view..." << std::endl;
  getRegistersAndDisassemblyBar()->pack_end(disassemblyView, false, false);
  std::cout << "11 Main window view..." << std::endl;
  getMasterLayout()->pack_start(controlsAndCompileBar, false, false);
  std::cout << "12 Main window view..." << std::endl;
  getMasterLayout()->pack_start(registersAndDisassemblyBar, false, false);
  std::cout << "13 Main window view..." << std::endl;
  getMasterLayout()->pack_start(terminalView, false, false);
  std::cout << "14 Main window view..." << std::endl;

  getMasterLayout()->show_all_children();
  std::cout << "15 Main window view..." << std::endl;
  getMasterLayout()->show();
  std::cout << "16 Main window view..." << std::endl;
  add(masterLayout);
  std::cout << "Through it ! Main window view..." << std::endl;

  setStyling();
  std::cout << "Just fakin bestie" << std::endl;
}

/**
 * @brief Sets the style attributes for the views - namely any icons and CSS.
 */
void MainWindowView::setStyling() {
  int i = 0;
  std::cout << i++ << " set styling" << std::endl;
  set_title("KoMo2");
  std::cout << i++ << " set styling" << std::endl;

  std::cout << getModel()->getAbsolutePathToProjectRoot() << std::endl;

  // Sets the icon for the window
  // set_icon_from_file(getModel()->getAbsolutePathToProjectRoot() +
  //                    "res/img/komo2Logo.png");
  std::cout << i++ << " set styling" << std::endl;

  // Create a css provider, get the style context, load the css file
  auto ctx = get_style_context();
  std::cout << i++ << " set styling" << std::endl;
  auto css = Gtk::CssProvider::create();
  std::cout << i++ << " set styling" << std::endl;
  css->load_from_path(getModel()->getAbsolutePathToProjectRoot() +
                      "res/styles.css");
  std::cout << i++ << " set styling" << std::endl;

  // Adds a CSS class for the main window
  get_style_context()->add_class("mainWindow");
  std::cout << i++ << " set styling" << std::endl;

  // Adds a CSS class for the layouts
  getControlsView()->get_style_context()->add_class("controls_layouts");
  std::cout << i++ << " set styling" << std::endl;
  getControlsAndCompileBar()->get_style_context()->add_class(
      "topContainer_layouts");
  std::cout << i++ << " set styling" << std::endl;
  getCompileLoadView()->get_style_context()->add_class("compileLoad_layouts");
  std::cout << i++ << " set styling" << std::endl;
  getRegistersView()->get_style_context()->add_class("registers_layouts");
  std::cout << i++ << " set styling" << std::endl;
  getDisassemblyView()->get_style_context()->add_class("dis_layouts");
  std::cout << i++ << " set styling" << std::endl;
  getTerminalView()->get_style_context()->add_class("terminal_layouts");
  std::cout << i++ << " set styling" << std::endl;

  // Add the CSS to the screen
  ctx->add_provider_for_screen(Gdk::Screen::get_default(), css,
                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  std::cout << i++ << " set styling" << std::endl;
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
Gtk::VButtonBox* const MainWindowView::getMasterLayout() {
  return &masterLayout;
}
