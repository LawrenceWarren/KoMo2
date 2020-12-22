/**
 * @file MainWindow.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief Definitions of the functions declared in the class definition, found
 * at `MainWindow.h`.
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

#include "MainWindow.h"
#include <gdkmm/rgba.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/frame.h>
#include <iostream>
#include <string>
#include "../models/KoMo2Model.h"

/**
 * @brief Construct a new MainWindow object.
 * @param x The width of the window.
 * @param y The height of the window.
 */
MainWindow::MainWindow(int x, int y)
    : masterLayout(),
      selectAndLoadContainer(),
      controlsAndCompileBar(),
      programControlsContainer(),
      selectedFileLabel("File: "),
      compileAndLoadButton("Compile & Load"),
      browseButton("Select File"),
      helpButton("Help"),
      beginRunJimulatorButton("Execute program"),
      reloadJimulatorButton("Reload program"),
      pauseResumeButton("Pause/resume execution"),
      singleStepExecuteButton("Single step execution"),
      haltExecutionButton("Halt execution") {
  set_border_width(4);
  set_default_size(x, y);  // ~16:9 ration

  setSizes(x, y);
  initSelectAndLoadContainer();
  initProgramControlsContainer();

  // TODO: pack the program controls and status box into an ordered list
  // use `programControlsContainer`, and defer any onClicks to `ControlsModel.h`
  // Do not implement any logic

  controlsAndCompileBar.set_layout(Gtk::BUTTONBOX_EDGE);
  controlsAndCompileBar.pack_end(programControlsContainer, false, false);
  controlsAndCompileBar.pack_end(selectAndLoadContainer, false, false);

  // masterLayout.pack_start(controlsAndCompileBar, false, false);

  add(controlsAndCompileBar);
  controlsAndCompileBar.show();
  // masterLayout.show();
}

/**
 * @brief Destroys a main window object.
 */
MainWindow::~MainWindow() {}

/**
 * @brief Packs children into the selectAndLoadContainer, and sets the layouts
 * and size of it.
 */
void MainWindow::initSelectAndLoadContainer() {
  selectAndLoadContainer.set_layout(Gtk::BUTTONBOX_END);
  selectAndLoadContainer.pack_end(*getBrowseButton(), false, false);
  selectAndLoadContainer.pack_end(*getSelectedFileLabel(), false, false);
  selectAndLoadContainer.pack_end(*getCompileAndLoadButton(), false, false);
  selectAndLoadContainer.show_all_children();
  selectAndLoadContainer.show();
}

/**
 * @brief Packs children into the programControlsContainer, and sets the layouts
 * and size of it.
 */
void MainWindow::initProgramControlsContainer() {
  programControlsContainer.set_layout(Gtk::BUTTONBOX_EDGE);
  programControlsContainer.pack_end(helpButton, true, true);
  programControlsContainer.pack_end(beginRunJimulatorButton, true, true);
  programControlsContainer.pack_end(reloadJimulatorButton, true, true);
  programControlsContainer.pack_end(pauseResumeButton, true, true);
  programControlsContainer.pack_end(singleStepExecuteButton, true, true);
  programControlsContainer.pack_end(haltExecutionButton, true, true);
  programControlsContainer.show_all_children();
  programControlsContainer.show();
}

/**
 * @brief Set the sizes of all views.
 * @param x The width of the window.
 * @param y The height of the window.
 */
void MainWindow::setSizes(int x, int y) {
  // button sizes
  browseButton.set_size_request(100, 33);
  compileAndLoadButton.set_size_request(100, 33);
  selectedFileLabel.set_size_request(100, 33);
  helpButton.set_size_request(190, 33);
  beginRunJimulatorButton.set_size_request(190, 33);
  reloadJimulatorButton.set_size_request(190, 33);
  pauseResumeButton.set_size_request(190, 33);
  singleStepExecuteButton.set_size_request(190, 33);
  haltExecutionButton.set_size_request(190, 33);

  // Layout sizes
  controlsAndCompileBar.set_size_request(x, 105);
  masterLayout.set_size_request(x, y);
  selectAndLoadContainer.set_size_request(100, 100);
  programControlsContainer.set_size_request(x - 100, 100);
}

/**
 * @brief Sets the style attributes for the views - namely any icons and CSS.
 */
void MainWindow::setStyling() {
  set_title(" KoMo2");

  // Sets the icon
  set_icon_from_file(getModel()->getAbsolutePathToProjectRoot() +
                     "res/komo2Logo.png");

  // Create a css provider, get the style context, load the css file
  auto ctx = get_style_context();
  auto css = Gtk::CssProvider::create();
  css->load_from_path(getModel()->getAbsolutePathToProjectRoot() +
                      "res/styles.css");

  // Adds a CSS class for the main window
  set_name("mainWindow");
  get_style_context()->add_class("mainWindow");

  // Adds a CSS class for the layouts
  programControlsContainer.set_name("layouts");
  controlsAndCompileBar.set_name("layouts");
  selectAndLoadContainer.set_name("layouts");
  programControlsContainer.get_style_context()->add_class("layouts");
  controlsAndCompileBar.get_style_context()->add_class("layouts");
  selectAndLoadContainer.get_style_context()->add_class("layouts");

  // Adds a CSS class for the compiler buttons
  compileAndLoadButton.set_name("compButtons");
  browseButton.set_name("compButtons");
  compileAndLoadButton.get_style_context()->add_class("compButtons");
  browseButton.get_style_context()->add_class("compButtons");

  // Adds a CSS class for the program running buttons
  helpButton.set_name("controlButtons");
  beginRunJimulatorButton.set_name("controlButtons");
  reloadJimulatorButton.set_name("controlButtons");
  pauseResumeButton.set_name("controlButtons");
  singleStepExecuteButton.set_name("controlButtons");
  haltExecutionButton.set_name("controlButtons");
  helpButton.get_style_context()->add_class("controlButtons");
  beginRunJimulatorButton.get_style_context()->add_class("controlButtons");
  reloadJimulatorButton.get_style_context()->add_class("controlButtons");
  pauseResumeButton.get_style_context()->add_class("controlButtons");
  singleStepExecuteButton.get_style_context()->add_class("controlButtons");
  haltExecutionButton.get_style_context()->add_class("controlButtons");

  // Adds a CSS class for the label
  selectedFileLabel.set_name("fileLabel");
  selectedFileLabel.get_style_context()->add_class("fileLabel");

  // Adds a CSS class for the container
  selectAndLoadContainer.set_name("selectLoadContainer");
  selectAndLoadContainer.get_style_context()->add_class("selectLoadContainer");

  // ! Add the CSS to the screen
  ctx->add_provider_for_screen(Gdk::Screen::get_default(), css,
                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

// ! Getters and setters.

/**
 * @brief Gets the `model` member variable.
 * @return KoMo2Model* A pointer to the `model` member variable.
 */
KoMo2Model* MainWindow::getModel() {
  return model;
}
/**
 * @brief Sets the `model` member variable.
 * @param val The pointer to set the model member variable to.
 */
void MainWindow::setModel(KoMo2Model* val) {
  model = val;
}
/**
 * @brief Gets the `compileAndLoadButton` member variable.
 * @return Gtk::Button* A pointer to the `compileAndLoadButton` member
 * variable.
 */
Gtk::Button* MainWindow::getCompileAndLoadButton() {
  return &compileAndLoadButton;
}
/**
 * @brief Gets the `browseButton` member variable.
 * @return Gtk::Button* A pointer to the `browseButton` member variable.
 */
Gtk::Button* MainWindow::getBrowseButton() {
  return &browseButton;
}
/**
 * @brief Gets the `selectedFileLabel` member variable.
 * @return Gtk::Label* A pointer to the `selectedFileLabel` member variable.
 */
Gtk::Label* MainWindow::getSelectedFileLabel() {
  return &selectedFileLabel;
}
/**
 * @brief Sets the text displayed by the `selectedFileLabel` member variable.
 * @param val The text to display in the `selectedFileLabel`.
 */
void MainWindow::setSelectedFileLabelText(std::string val) {
  getSelectedFileLabel()->set_text(val);
}
/**
 * @brief Gets the `helpButton` member variable.
 * @return Gtk::Button* A pointer to the `helpButton` member variable.
 */
Gtk::Button* MainWindow::getHelpButton() {
  return &helpButton;
}
/**
 * @brief Gets the `beginRunJimulatorButton` member variable.
 * @return Gtk::Button* A pointer to the `beginRunJimulatorButton` member
 * variable.
 */
Gtk::Button* MainWindow::getBeginRunJimulatorButton() {
  return &beginRunJimulatorButton;
}
/**
 * @brief Gets the `reloadJimulatorButton` member variable.
 * @return Gtk::Button* A pointer to the `reloadJimulatorButton` member
 * variable.
 */
Gtk::Button* MainWindow::getReloadJimulatorButton() {
  return &reloadJimulatorButton;
}
/**
 * @brief Gets the `pauseResumeButton` member variable.
 * @return Gtk::Button* A pointer to the `pauseResumeButton` member variable.
 */
Gtk::Button* MainWindow::getPauseResumeButton() {
  return &pauseResumeButton;
}
/**
 * @brief Gets the `singleStepExecuteButton` member variable.
 * @return Gtk::Button* A pointer to the `singleStepExecuteButton` member
 * variable.
 */
Gtk::Button* MainWindow::getSingleStepExecuteButton() {
  return &singleStepExecuteButton;
}
/**
 * @brief Gets the `haltExecutionButton` member variable.
 * @return Gtk::Button* A pointer to the `haltExecutionButton` member variable.
 */
Gtk::Button* MainWindow::getHaltExecutionButton() {
  return &haltExecutionButton;
}
