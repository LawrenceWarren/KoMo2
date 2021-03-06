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
  set_border_width(4);
  setSizes(x, y);

  // Packs
  getControlsAndCompileBar()->set_layout(Gtk::BUTTONBOX_EDGE);
  getControlsAndCompileBar()->pack_end(controlsView, false, false);
  getControlsAndCompileBar()->pack_end(compileLoadView, false, false);
  getControlsAndCompileBar()->show();

  getRegistersAndDisassemblyBar()->set_layout(Gtk::BUTTONBOX_EDGE);
  getRegistersAndDisassemblyBar()->pack_end(registersView, false, false);
  getRegistersAndDisassemblyBar()->pack_end(disassemblyView, false, false);
  getRegistersAndDisassemblyBar()->show();

  getMasterLayout()->pack_start(controlsAndCompileBar, false, false);
  getMasterLayout()->pack_start(registersAndDisassemblyBar, false, false);
  getMasterLayout()->pack_start(terminalView, false, false);

  getMasterLayout()->set_layout(Gtk::BUTTONBOX_START);
  getMasterLayout()->show_all_children();
  getMasterLayout()->show();
  add(masterLayout);
}

/**
 * @brief Set the sizes of all views.
 * @param x The width of the window.
 * @param y The height of the window.
 */
void MainWindowView::setSizes(const int x, const int y) {
  set_default_size(x, y);  // ~16:9 ration

  const float topBarHeight = (y / 7.f) * 1.f;
  const float middleBarHeight = (y / 7.f) * 4.f;
  const float bottomBarHeight = (y / 7.f) * 2.f;
  const float xUnits = x / 12.4f;

  // Layout sizes
  // TODO: these should be deleted once all components are implemented
  getMasterLayout()->set_size_request(x, y);
  getControlsAndCompileBar()->set_size_request(x, -1);
  getRegistersAndDisassemblyBar()->set_size_request(x, middleBarHeight);

  getCompileLoadView()->set_size_request(xUnits, topBarHeight);
  getControlsView()->set_size_request(x - xUnits, topBarHeight / 3.f);
  getRegistersView()->set_size_request(xUnits * 3.f, middleBarHeight);
  getDisassemblyView()->set_size_request(x - (xUnits * 3.f), middleBarHeight);
  getTerminalView()->set_size_request(x, bottomBarHeight);
}

/**
 * @brief Sets the style attributes for the views - namely any icons and CSS.
 */
void MainWindowView::setStyling() {
  set_title(" KoMo2");

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
  getControlsView()->get_style_context()->add_class("controls_layouts");
  getControlsAndCompileBar()->get_style_context()->add_class(
      "topContainer_layouts");
  getCompileLoadView()->get_style_context()->add_class("compileLoad_layouts");
  getRegistersView()->get_style_context()->add_class("registers_layouts");
  getDisassemblyView()->get_style_context()->add_class("dis_layouts");
  getTerminalView()->get_style_context()->add_class("terminal_layouts");

  // ! Add the CSS to the screen
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
Gtk::VButtonBox* const MainWindowView::getMasterLayout() {
  return &masterLayout;
}
