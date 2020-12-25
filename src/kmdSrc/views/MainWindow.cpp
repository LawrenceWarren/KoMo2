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
#include <gtkmm/image.h>
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
      helpButton(),
      reloadJimulatorButton(),
      pauseResumeButton(),
      singleStepExecuteButton(),
      haltExecutionButton() {
  set_border_width(4);
  set_default_size(x, y);  // ~16:9 ration

  setSizes(x, y);
  initSelectAndLoadContainer();
  initProgramControlsContainer();

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
MainWindow::~MainWindow() {}

/**
 * @brief Packs children into the selectAndLoadContainer, and sets the layouts
 * and size of it.
 */
void MainWindow::initSelectAndLoadContainer() {
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
 * @brief Packs children into the programControlsContainer, and sets the layouts
 * and size of it. Initialises the look images of buttons.
 */
void MainWindow::initProgramControlsContainer() {
  // Set halt button image
  haltExecutionButton.set_image_position(Gtk::POS_LEFT);
  haltExecutionButton.set_image(*new Gtk::Image("res/haltSymbol.png"));
  haltExecutionButton.set_tooltip_text("Halt Jimulator (F1)");

  // Set help button image
  helpButton.set_image_position(Gtk::POS_LEFT);
  helpButton.set_image(*new Gtk::Image("res/helpSymbol.png"));
  helpButton.set_tooltip_text("About KoMo2 (F12)");

  // Set the single step execution button image
  singleStepExecuteButton.set_image_position(Gtk::POS_LEFT);
  singleStepExecuteButton.set_image(
      *new Gtk::Image("res/singleStepSymbol.png"));
  singleStepExecuteButton.set_tooltip_text("Execute 1 instruction (F6)");

  // Set the reload button image
  reloadJimulatorButton.set_image_position(Gtk::POS_LEFT);
  reloadJimulatorButton.set_image(*new Gtk::Image("res/refreshSymbol.png"));
  reloadJimulatorButton.set_tooltip_text("Reload program (Ctrl+R)");

  // Pack buttons into a container
  programControlsContainer.set_layout(Gtk::BUTTONBOX_CENTER);
  programControlsContainer.pack_end(helpButton, false, false);
  programControlsContainer.pack_end(reloadJimulatorButton, false, false);
  programControlsContainer.pack_end(pauseResumeButton, false, false);
  programControlsContainer.pack_end(singleStepExecuteButton, false, false);
  programControlsContainer.pack_end(haltExecutionButton, false, false);
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
  helpButton.set_size_request(40, 40);
  reloadJimulatorButton.set_size_request(40, 40);
  pauseResumeButton.set_size_request(40, 40);
  singleStepExecuteButton.set_size_request(40, 40);
  haltExecutionButton.set_size_request(40, 40);

  // Layout sizes
  masterLayout.set_size_request(x, y);
  controlsAndCompileBar.set_size_request(x, 100);
  selectAndLoadContainer.set_size_request(100, 100);
  programControlsContainer.set_size_request(x - 100, 33);
}

/**
 * @brief Sets the style attributes for the views - namely any icons and CSS.
 */
void MainWindow::setStyling() {
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

  // Adds a CSS class for the program running buttons
  helpButton.get_style_context()->add_class("controlButtons");
  reloadJimulatorButton.get_style_context()->add_class("controlButtons");
  pauseResumeButton.get_style_context()->add_class("controlButtons");
  singleStepExecuteButton.get_style_context()->add_class("controlButtons");
  haltExecutionButton.get_style_context()->add_class("controlButtons");

  // Adds a CSS class for the label
  selectedFileLabel.get_style_context()->add_class("fileLabel");

  // Adds a CSS class for the container
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
