/**
 * @file ControlsModel.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief Definitions of the functions declared in the class declaration, found
 * in the file `ControlsModel.h`.
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

#include <iostream>
#include <string>
#include "../views/ControlsView.h"
#include "KoMo2Model.h"

/**
 * @brief Construct a new ControlsModel::ControlsModel object.
 * @param view A pointer to the accompanying view object.
 * @param parent A pointer to the parent model.
 * @param refreshRate An integer that describes how many milliseconds should be
 * taken between refreshes when KoMo2 is in the `JimulatorState::RUNNING` state.
 */
ControlsModel::ControlsModel(ControlsView* const view,
                             const std::string manual,
                             KoMo2Model* const parent)
    : Model(parent), view(view) {
  view->getHelpButton()->set_uri(manual);

  setButtonListener(view->getPauseResumeButton(), this,
                    &ControlsModel::onPauseResumeClick);

  setButtonListener(view->getHaltExecutionButton(), this,
                    &ControlsModel::onHaltExecutionClick);

  setButtonListener(view->getReloadJimulatorButton(), this,
                    &ControlsModel::onReloadJimulatorClick);

  setButtonListener(view->getSingleStepExecuteButton(), this,
                    &ControlsModel::onSingleStepExecuteClick);

  // Set the model & images of the view.
  view->setModel(this);
  view->setButtonImages(getParent()->getAbsolutePathToProjectRoot());
}

// ! Button event handlers

/**
 * @brief Handles the `haltExecutionButton` click events -  changes
 * `JimulatorState` to "JimulatorState::UNLOADED".
 */
void ControlsModel::onHaltExecutionClick() {
  getParent()->changeJimulatorState(JimulatorState::UNLOADED);
}

/**
 * @brief Handles the `reloadJimulatorButton` click events - sends a command to
 * Jimulator and changes `JimulatorState` to "JimulatorState::LOADED".
 */
void ControlsModel::onReloadJimulatorClick() {
  Jimulator::resetJimulator();
  getParent()->changeJimulatorState(JimulatorState::LOADED);
}

/**
 * @brief Handles the `pauseResumeButton` click events - changes
 * `JimulatorState` to "JimulatorState::RUNNING" if currently
 * JimulatorState::PAUSED, and "JimulatorState::PAUSED" if currently
 * JimulatorState::RUNNING. Sends a command to Jimulator in every case.
 */
void ControlsModel::onPauseResumeClick() {
  switch (getJimulatorState()) {
    case JimulatorState::RUNNING:
      Jimulator::pauseJimulator();
      getParent()->changeJimulatorState(JimulatorState::PAUSED);
      break;
    case JimulatorState::PAUSED:
      Jimulator::continueJimulator();
      getParent()->changeJimulatorState(JimulatorState::RUNNING);
      break;
    case JimulatorState::LOADED:
      Jimulator::startJimulator(0);
      getParent()->changeJimulatorState(JimulatorState::RUNNING);
      break;
    default:
      // TODO: Handle error state gracefully
      break;
  }
}

/**
 * @brief Handles the `singleStepExecuteButton` click events - changes
 * `JimulatorState` to "JimulatorState::PAUSED" if state is already LOADED, and
 * sends a command to Jimulator.
 */
void ControlsModel::onSingleStepExecuteClick() {
  Jimulator::startJimulator(1);
  getParent()->refreshViews();

  if (getJimulatorState() == JimulatorState::LOADED) {
    getParent()->changeJimulatorState(JimulatorState::PAUSED);
  }
}

// ! Virtual functions

/**
 * @brief Handles a key press event pertaining to this model.
 * @param e The key press event.
 * @return bool Was a key pressed or not?
 */
const bool ControlsModel::handleKeyPress(const GdkEventKey* const e) {
  switch (e->keyval) {
    case GDK_KEY_F5:
      if (getJimulatorState() != JimulatorState::UNLOADED) {
        onPauseResumeClick();
      }
      return true;
    case GDK_KEY_F6:
      if (getJimulatorState() == JimulatorState::LOADED ||
          getJimulatorState() == JimulatorState::PAUSED) {
        onSingleStepExecuteClick();
      }
      return true;
    case GDK_KEY_F1:
      if (getJimulatorState() == JimulatorState::RUNNING ||
          getJimulatorState() == JimulatorState::PAUSED) {
        onHaltExecutionClick();
      }
      return true;
    case GDK_KEY_F12:
      view->getHelpButton()->clicked();
      return true;
    default:
      return false;
  }

  return false;
}

/**
 * @brief Handles the Jimulator state change for this model. For each value of
 * `newState`, it will update the state of the control buttons & potentially
 * update their images.
 * @param newState The state that was changed into.
 */
void ControlsModel::changeJimulatorState(const JimulatorState newState) {
  switch (newState) {
    // some unloaded state
    case JimulatorState::UNLOADED: {
      setButtonState(view->getHelpButton(), true);
      setButtonState(view->getReloadJimulatorButton(), false);
      setButtonState(
          view->getPauseResumeButton(), false,
          new Gtk::Image(getParent()->getAbsolutePathToProjectRoot() +
                         "res/img/commenceSymbol.png"),
          "Commence execution (F5)");
      setButtonState(view->getHaltExecutionButton(), false);
      setButtonState(view->getSingleStepExecuteButton(), false);

      // Set accessibility options for the pause/resume button
      auto pauseResume = view->getPauseResumeButton()->get_accessible();
      pauseResume->set_name("Play/pause execution");
      pauseResume->set_description("Plays and pauses execution of program");
      return;
    }
    // loaded, not yet run state
    case JimulatorState::LOADED: {
      setButtonState(view->getHelpButton(), true);
      setButtonState(view->getReloadJimulatorButton(), false);
      setButtonState(
          view->getPauseResumeButton(), true,
          new Gtk::Image(getParent()->getAbsolutePathToProjectRoot() +
                         "res/img/commenceSymbol.png"),
          "Commence execution (F5)");
      setButtonState(view->getSingleStepExecuteButton(), true);
      setButtonState(view->getHaltExecutionButton(), false);

      // Set accessibility options for the pause/resume button
      auto pauseResume = view->getPauseResumeButton();
      pauseResume->set_tooltip_text("Commence execution (F5)");
      pauseResume->get_accessible()->set_name("Commence execution");
      pauseResume->get_accessible()->set_description(
          "Commence execution of the loaded program");
      return;
    }
    // Currently running
    case JimulatorState::RUNNING: {
      setButtonState(view->getHelpButton(), true);
      setButtonState(view->getReloadJimulatorButton(), false);
      setButtonState(
          view->getPauseResumeButton(), true,
          new Gtk::Image(getParent()->getAbsolutePathToProjectRoot() +
                         "res/img/pauseSymbol.png"),
          "Pause execution (F5)");
      setButtonState(view->getSingleStepExecuteButton(), false);
      setButtonState(view->getHaltExecutionButton(), true);

      // Set accessibility options for the pause/resume button
      auto pauseResume = view->getPauseResumeButton();
      pauseResume->set_tooltip_text("Pause execution (F5)");
      pauseResume->get_accessible()->set_name("Pause execution");
      pauseResume->get_accessible()->set_description(
          "Pause execution of the running program");
      return;
    }
    // Has been running; is paused
    case JimulatorState::PAUSED: {
      setButtonState(view->getHelpButton(), true);
      setButtonState(view->getReloadJimulatorButton(), true);
      setButtonState(
          view->getPauseResumeButton(), true,
          new Gtk::Image(getParent()->getAbsolutePathToProjectRoot() +
                         "res/img/playSymbol.png"),
          "Resume execution (F5)");
      setButtonState(view->getSingleStepExecuteButton(), true);
      setButtonState(view->getHaltExecutionButton(), true);

      // Set accessibility options for the pause/resume button
      auto pauseResume = view->getPauseResumeButton();
      pauseResume->set_tooltip_text("Resume execution (F5)");
      pauseResume->get_accessible()->set_name("Resume execution");
      pauseResume->get_accessible()->set_description(
          "Resume execution of the paused program");
      return;
    }
    // Error state
    default:
      // TODO: Handle the error state gracefully
      return;
  }
}
