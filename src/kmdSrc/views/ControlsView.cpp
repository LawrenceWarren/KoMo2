/**
 * @file ControlsView.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief A class that defines the class declared in the file `ControlsView.h`.
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

#include "ControlsView.h"
#include <gtkmm/button.h>
#include <gtkmm/image.h>

/**
 * @brief Construct a new Controls View.
 * @param parent
 */
ControlsView::ControlsView(MainWindowView* parent)
    : parent(parent),
      helpButton(),
      reloadJimulatorButton(),
      pauseResumeButton(),
      singleStepExecuteButton(),
      haltExecutionButton() {
  initProgramControlsContainer();
}

/**
 * @brief Destroy a Controls View object.
 */
ControlsView::~ControlsView() {}

/**
 * @brief Packs children into the programControlsContainer, and sets the layouts
 * and size of it. Initialises the look images of buttons.
 */
void ControlsView::initProgramControlsContainer() {
  // Set halt button image
  getHaltExecutionButton()->set_image_position(Gtk::POS_LEFT);
  getHaltExecutionButton()->set_tooltip_text("Halt Jimulator (F1)");

  // Set help button image
  // TODO: does this accessibility work?
  getHelpButton()->set_image_position(Gtk::POS_LEFT);
  getHelpButton()->set_tooltip_text("About KoMo2 (F12)");
  getHelpButton()->get_accessible()->set_name("Help Button");
  getHelpButton()->get_accessible()->set_description(
      "This button will display a help window.");

  // TODO: help button is focused by default. Stop that
  // TODO: the focus outline shows up NO MATTER WHAT. stop that

  // Set the single step execution button image
  getSingleStepExecuteButton()->set_image_position(Gtk::POS_LEFT);
  getSingleStepExecuteButton()->set_tooltip_text("Execute 1 instruction (F6)");

  // Set the reload button image
  getReloadJimulatorButton()->set_image_position(Gtk::POS_LEFT);
  getReloadJimulatorButton()->set_tooltip_text("Reload program (Ctrl+R)");

  // Set sizes
  getHelpButton()->set_size_request(40, 40);
  getReloadJimulatorButton()->set_size_request(40, 40);
  getPauseResumeButton()->set_size_request(40, 40);
  getSingleStepExecuteButton()->set_size_request(40, 40);
  getHaltExecutionButton()->set_size_request(40, 40);

  // Adds a CSS class for the program running buttons
  getHelpButton()->get_style_context()->add_class("controlButtons");
  getReloadJimulatorButton()->get_style_context()->add_class("controlButtons");
  getPauseResumeButton()->get_style_context()->add_class("controlButtons");
  getSingleStepExecuteButton()->get_style_context()->add_class(
      "controlButtons");
  getHaltExecutionButton()->get_style_context()->add_class("controlButtons");

  // Pack buttons into a container
  this->set_layout(Gtk::BUTTONBOX_CENTER);
  this->pack_end(helpButton, false, false);
  this->pack_end(reloadJimulatorButton, false, false);
  this->pack_end(pauseResumeButton, false, false);
  this->pack_end(singleStepExecuteButton, false, false);
  this->pack_end(haltExecutionButton, false, false);
  this->show_all_children();
  this->show();
}

// ! Getters and setters

/**
 * @brief Set the model member variable. Also sets the initial button states.
 * @param val The value to set model to.
 * @param projectRoot An absolute path to the root of the project.
 */
void ControlsView::setModel(ControlsModel* val, std::string projectRoot) {
  model = val;

  getHaltExecutionButton()->set_image(
      *new Gtk::Image(projectRoot + "/res/haltSymbol.png"));

  getHelpButton()->set_image(
      *new Gtk::Image(projectRoot + "/res/helpSymbol.png"));

  getSingleStepExecuteButton()->set_image(
      *new Gtk::Image(projectRoot + "/res/singleStepSymbol.png"));

  getReloadJimulatorButton()->set_image(
      *new Gtk::Image(projectRoot + "/res/refreshSymbol.png"));
}

/**
 * @brief Gets the `helpButton` member variable.
 * @return Gtk::Button* A pointer to the `helpButton` member variable.
 */
Gtk::Button* ControlsView::getHelpButton() {
  return &helpButton;
}
/**
 * @brief Gets the `reloadJimulatorButton` member variable.
 * @return Gtk::Button* A pointer to the `reloadJimulatorButton` member
 * variable.
 */
Gtk::Button* ControlsView::getReloadJimulatorButton() {
  return &reloadJimulatorButton;
}
/**
 * @brief Gets the `pauseResumeButton` member variable.
 * @return Gtk::Button* A pointer to the `pauseResumeButton` member variable.
 */
Gtk::Button* ControlsView::getPauseResumeButton() {
  return &pauseResumeButton;
}
/**
 * @brief Gets the `singleStepExecuteButton` member variable.
 * @return Gtk::Button* A pointer to the `singleStepExecuteButton` member
 * variable.
 */
Gtk::Button* ControlsView::getSingleStepExecuteButton() {
  return &singleStepExecuteButton;
}
/**
 * @brief Gets the `haltExecutionButton` member variable.
 * @return Gtk::Button* A pointer to the `haltExecutionButton` member variable.
 */
Gtk::Button* ControlsView::getHaltExecutionButton() {
  return &haltExecutionButton;
}