/**
 * @file ControlsView.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief A class that defines the class declared in the file `ControlsView.h`.
 * @version 1.0.0
 * @date 10-04-2021
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
 * @param parent A reference to the parent view.
 */
ControlsView::ControlsView(MainWindowView* const parent)
    : parent(parent),
      helpButton(),
      reloadJimulatorButton(),
      pauseResumeButton(),
      singleStepExecuteButton(),
      haltExecutionButton() {
  initHelpButton();
  initHaltExecutionButton();
  initSingleStepExecuteButton();
  initReloadJimulatorButton();
  initPauseResumeButton();
  initProgramControlsContainer();
}

/**
 * @brief Packs children into the `controlsView`, and sets the layout and size
 * of it.
 */
void ControlsView::initProgramControlsContainer() {
  set_layout(Gtk::BUTTONBOX_END);
  pack_end(helpButton, false, false);
  pack_end(reloadJimulatorButton, false, false);
  pack_end(pauseResumeButton, false, false);
  pack_end(singleStepExecuteButton, false, false);
  pack_end(haltExecutionButton, false, false);
  show_all_children();
  show();
}

/**
 * @brief Setup all of the information about the help button.
 */
void ControlsView::initHelpButton() {
  getHelpButton()->set_tooltip_text("About KoMo2 (F12)");
  getHelpButton()->get_accessible()->set_name("Help");
  getHelpButton()->get_accessible()->set_description("Displays a help window.");
  getHelpButton()->set_size_request(102, 102);
  getHelpButton()->get_style_context()->add_class("controlButtons");
}

/**
 * @brief Sets up all of the information about the halt execution button.
 */
void ControlsView::initHaltExecutionButton() {
  getHaltExecutionButton()->set_tooltip_text("Halt Jimulator (F1)");
  getHaltExecutionButton()->get_accessible()->set_name("Halt execution");
  getHaltExecutionButton()->get_accessible()->set_description(
      "Halt execution of the loaded program.");
  getHaltExecutionButton()->set_size_request(102, 102);
  getHaltExecutionButton()->get_style_context()->add_class("controlButtons");
}

/**
 * @brief Sets up all of the information about the single step execution button.
 */
void ControlsView::initSingleStepExecuteButton() {
  getSingleStepExecuteButton()->set_tooltip_text("Execute 1 instruction (F6)");
  getSingleStepExecuteButton()->get_accessible()->set_name(
      "Single step execute");
  getSingleStepExecuteButton()->get_accessible()->set_description(
      "Execute a single line of the loaded program.");
  getSingleStepExecuteButton()->set_size_request(102, 102);
  getSingleStepExecuteButton()->get_style_context()->add_class(
      "controlButtons");
}

/**
 * @brief Sets up all of the information about the reload Jimulator button.
 */
void ControlsView::initReloadJimulatorButton() {
  getReloadJimulatorButton()->set_tooltip_text("Reload program (Ctrl+R)");
  getReloadJimulatorButton()->get_accessible()->set_name("Reload program");
  getReloadJimulatorButton()->get_accessible()->set_description(
      "Reloads the currently loaded program.");
  getReloadJimulatorButton()->set_size_request(102, 102);
  getReloadJimulatorButton()->get_style_context()->add_class("controlButtons");
}

/**
 * @brief Sets up all of the information about the pause & resume button. This
 * is emptier than its sibling functions, as a significant amount of the
 * information about this button changes at run time.
 */
void ControlsView::initPauseResumeButton() {
  getPauseResumeButton()->set_size_request(102, 102);
  getPauseResumeButton()->get_style_context()->add_class("controlButtons");
}

// ! Getters and setters

/**
 * @brief Set the model member variable.
 * @param val The value to set model to.
 */
void ControlsView::setModel(ControlsModel* const val) {
  model = val;
}
/**
 * @brief Sets the images for 4 of the buttons.
 * @param projectRoot An absolute path to the root of the project.
 */
void ControlsView::setButtonImages(const std::string projectRoot) {
  getHaltExecutionButton()->set_image(
      *new Gtk::Image(projectRoot + "/res/img/haltSymbol.png"));

  getHelpButton()->set_image(
      *new Gtk::Image(projectRoot + "/res/img/helpSymbol.png"));

  getSingleStepExecuteButton()->set_image(
      *new Gtk::Image(projectRoot + "/res/img/singleStepSymbol.png"));

  getReloadJimulatorButton()->set_image(
      *new Gtk::Image(projectRoot + "/res/img/refreshSymbol.png"));
}
/**
 * @brief Gets the `helpButton` member variable.
 * @return Gtk::Button* A pointer to the `helpButton` member variable.
 */
Gtk::LinkButton* const ControlsView::getHelpButton() {
  return &helpButton;
}
/**
 * @brief Gets the `reloadJimulatorButton` member variable.
 * @return Gtk::Button* A pointer to the `reloadJimulatorButton` member
 * variable.
 */
Gtk::Button* const ControlsView::getReloadJimulatorButton() {
  return &reloadJimulatorButton;
}
/**
 * @brief Gets the `pauseResumeButton` member variable.
 * @return Gtk::Button* A pointer to the `pauseResumeButton` member variable.
 */
Gtk::Button* const ControlsView::getPauseResumeButton() {
  return &pauseResumeButton;
}
/**
 * @brief Gets the `singleStepExecuteButton` member variable.
 * @return Gtk::Button* A pointer to the `singleStepExecuteButton` member
 * variable.
 */
Gtk::Button* const ControlsView::getSingleStepExecuteButton() {
  return &singleStepExecuteButton;
}
/**
 * @brief Gets the `haltExecutionButton` member variable.
 * @return Gtk::Button* A pointer to the `haltExecutionButton` member variable.
 */
Gtk::Button* const ControlsView::getHaltExecutionButton() {
  return &haltExecutionButton;
}
