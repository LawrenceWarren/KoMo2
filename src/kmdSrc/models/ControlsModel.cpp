/**
 * @file ControlsModel.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief Definitions of the functions declared in the class declaration, found
 * in the file `ControlsModel.h`.
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
 */

#include <gtkmm.h>
#include <iostream>
#include <string>
#include "KoMo2Model.h"

/**
 * @brief Construct a new ControlsModel object.
 * @param helpButton A pointer to the helpButton.
 * @param beginRunJimulatorButton A pointer to the beginRunJimulatorButton.
 * @param reloadJimulatorButton A pointer to the reloadJimulatorButton.
 * @param pauseResumeButton A pointer to the pauseResumeButton.
 * @param singleStepExecuteButton A pointer to the singleStepExecutionButton.
 * @param haltExecutionButton A pointer to the haltExecutionButton.
 * @param parent A pointer to the parent model, KoMo2Model.
 */
ControlsModel::ControlsModel(Gtk::Button* helpButton,
                             Gtk::Button* beginRunJimulatorButton,
                             Gtk::Button* reloadJimulatorButton,
                             Gtk::Button* pauseResumeButton,
                             Gtk::Button* singleStepExecuteButton,
                             Gtk::Button* haltExecutionButton,
                             KoMo2Model* parent)
    : helpButton(helpButton),
      beginRunJimulatorButton(beginRunJimulatorButton),
      reloadJimulatorButton(reloadJimulatorButton),
      pauseResumeButton(pauseResumeButton),
      singleStepExecuteButton(singleStepExecuteButton),
      haltExecutionButton(haltExecutionButton),
      parent(parent) {}

/**
 * @brief Destroys a ControlModel object.
 */
ControlsModel::~ControlsModel() {}

/**
 * @brief Handles the `helpButton` click events.
 */
void ControlsModel::onHelpClick() {
  std::cout << "Help Button click!" << std::endl;
}

/**
 * @brief Handles the `beginRunJimulatorButton` click events.
 */
void ControlsModel::onBeginRunJimulatorClick() {
  std::cout << "begin Run Jimulator Button Click!" << std::endl;
}

/**
 * @brief Handles the `reloadJimulatorButton` click events.
 */
void ControlsModel::onReloadJimulatorClick() {
  std::cout << "Reload Jimulator Button Click!" << std::endl;
}

/**
 * @brief Handles the `pauseResumeButton` click events.
 */
void ControlsModel::onPauseResumeClick() {
  std::cout << "pause/Resume Button Click!" << std::endl;
}

/**
 * @brief Handles the `singleStepExecuteButton` click events.
 */
void ControlsModel::onSingleStepExecuteClick() {
  std::cout << "Single Step execution Button Click!" << std::endl;
}

/**
 * @brief Handles the `haltExecutionButton` click events.
 */
void ControlsModel::onHaltExecutionClick() {
  std::cout << "Halt Execution Button Click!" << std::endl;
}
