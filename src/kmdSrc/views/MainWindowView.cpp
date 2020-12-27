/**
 * @file MainWindowView.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief Definitions of the functions declared in the class definition, found
 * at `MainWindowView.h`.
 * @version 0.1
 * @date 2020-12-22
 * @section LICENSE
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * https://www.gnu.org/copyleft/gpl.html
 * @copyright Copyright (c) 2020
 *
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
MainWindowView::MainWindowView(int x, int y)
    : masterLayout(),
      selectAndLoadContainer(),
      controlsAndCompileBar(),
      programControlsContainer(this),
      selectedFileLabel("File: "),
      compileAndLoadButton("Compile & Load"),
      browseButton("Select File") {
  set_border_width(4);
  set_default_size(x, y);  // ~16:9 ration

  setSizes(x, y);
  initSelectAndLoadContainer();

  controlsAndCompileBar.set_layout(Gtk::BUTTONBOX_EDGE);
  controlsAndCompileBar.pack_end(programControlsContainer, false, false);
  controlsAndCompileBar.pack_end(selectAndLoadContainer, false, false);
  controlsAndCompileBar.show();

  masterLayout.pack_start(controlsAndCompileBar, false, false);

  masterLayout.show_all_children();
  masterLayout.show();
  add(masterLayout);
}

/**
 * @brief Destroys a main window object.
 */
MainWindowView::~MainWindowView() {}

/**
 * @brief Packs children into the selectAndLoadContainer, and sets the layouts
 * and size of it.
 */
void MainWindowView::initSelectAndLoadContainer() {
  browseButton.set_tooltip_text("Browse for an ARM assembly file (CTRL+L)");
  compileAndLoadButton.set_tooltip_text(
      "Compile and load your file into Jimulator (CTRL+R)");

  selectAndLoadContainer.set_layout(Gtk::BUTTONBOX_END);
  selectAndLoadContainer.pack_end(*getBrowseButton(), false, false);
  selectAndLoadContainer.pack_end(*getSelectedFileLabel(), false, false);
  selectAndLoadContainer.pack_end(*getCompileAndLoadButton(), false, false);
  selectAndLoadContainer.show_all_children();
  selectAndLoadContainer.show();
}

/**
 * @brief Set the sizes of all views.
 * @param x The width of the window.
 * @param y The height of the window.
 */
void MainWindowView::setSizes(int x, int y) {
  // button sizes
  browseButton.set_size_request(100, 33);
  compileAndLoadButton.set_size_request(100, 33);
  selectedFileLabel.set_size_request(100, 33);

  // Layout sizes
  masterLayout.set_size_request(x, y);
  controlsAndCompileBar.set_size_request(x, 100);
  selectAndLoadContainer.set_size_request(100, 100);
  programControlsContainer.set_size_request(x - 100, 33);
}

/**
 * @brief Sets the style attributes for the views - namely any icons and CSS.
 */
void MainWindowView::setStyling() {
  set_title(" KoMo2");

  // Sets the icon for the window
  set_icon_from_file(getModel()->getAbsolutePathToProjectRoot() +
                     "res/komo2Logo.png");

  // Create a css provider, get the style context, load the css file
  auto ctx = get_style_context();
  auto css = Gtk::CssProvider::create();
  css->load_from_path(getModel()->getAbsolutePathToProjectRoot() +
                      "res/styles.css");

  // Adds a CSS class for the main window
  get_style_context()->add_class("mainWindow");

  // Adds a CSS class for the layouts
  programControlsContainer.get_style_context()->add_class("layouts");
  controlsAndCompileBar.get_style_context()->add_class("layouts");
  selectAndLoadContainer.get_style_context()->add_class("layouts");

  // Adds a CSS class for the compiler buttons
  compileAndLoadButton.get_style_context()->add_class("compButtons");
  browseButton.get_style_context()->add_class("compButtons");

  // Adds a CSS class for the label
  selectedFileLabel.get_style_context()->add_class("fileLabel");

  // Adds a CSS class for the container
  selectAndLoadContainer.get_style_context()->add_class("selectLoadContainer");

  // ! Add the CSS to the screen
  ctx->add_provider_for_screen(Gdk::Screen::get_default(), css,
                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

// ! Getters and setters.

ControlsView* MainWindowView::getControlsView() {
  return &programControlsContainer;
}

/**
 * @brief Gets the `model` member variable.
 * @return KoMo2Model* A pointer to the `model` member variable.
 */
KoMo2Model* MainWindowView::getModel() {
  return model;
}
/**
 * @brief Sets the `model` member variable.
 * @param val The pointer to set the model member variable to.
 */
void MainWindowView::setModel(KoMo2Model* val) {
  model = val;
}
/**
 * @brief Gets the `compileAndLoadButton` member variable.
 * @return Gtk::Button* A pointer to the `compileAndLoadButton` member
 * variable.
 */
Gtk::Button* MainWindowView::getCompileAndLoadButton() {
  return &compileAndLoadButton;
}
/**
 * @brief Gets the `browseButton` member variable.
 * @return Gtk::Button* A pointer to the `browseButton` member variable.
 */
Gtk::Button* MainWindowView::getBrowseButton() {
  return &browseButton;
}
/**
 * @brief Gets the `selectedFileLabel` member variable.
 * @return Gtk::Label* A pointer to the `selectedFileLabel` member variable.
 */
Gtk::Label* MainWindowView::getSelectedFileLabel() {
  return &selectedFileLabel;
}
/**
 * @brief Sets the text displayed by the `selectedFileLabel` member variable.
 * @param val The text to display in the `selectedFileLabel`.
 */
void MainWindowView::setSelectedFileLabelText(std::string val) {
  getSelectedFileLabel()->set_text(val);
}
