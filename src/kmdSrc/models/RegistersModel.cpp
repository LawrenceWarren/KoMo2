/**
 * @file RegistersModel.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief This file defines the class RegistersModel, which represents the
 * logical data  that relates to the register view you see in the KoMo2 GUI. The
 * view is represented in the file `RegistersView.cpp` and it's associated
 * header.
 * @version 0.1
 * @date 2021-03-18
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

#include "../views/MainWindowView.h"
#include "KoMo2Model.h"
#include "iostream"

/**
 * @brief Constructs a new registers model object.
 * @param view A pointer to the view this register represents.
 * @param parent A pointer to the parent model.
 */
RegistersModel::RegistersModel(RegistersView* const view,
                               KoMo2Model* const parent)
    : Model(parent), view(view) {
  view->setModel(this);
}

/**
 * @brief Handles changes in the Jimulator state.
 * @param newState The state being changed into.
 */
void RegistersModel::changeJimulatorState(const JimulatorState newState) {}

/**
 * @brief Handles any key press events.
 * @param e The key press event.
 * @return bool true if the key press was handled.
 */
const bool RegistersModel::handleKeyPress(const GdkEventKey* const e) {
  // TODO: readout label values upon Alt+0-9 OR Alt+A-F?
  return false;
}

/**
 * @brief Handles updating this particular view.
 * Reads register values from Jimulator, sets the label values of this view
 * to reflect those values.
 */
void RegistersModel::refreshViews() {
  const auto newValues = getRegisterValueFromJimulator();
  auto* const labelArray = getView()->getLabels();

  for (long unsigned int i = 0; i < 16; i++) {
    (*labelArray)[1][i].set_text(newValues[i]);
  }

  getParent()->getDisassemblyModel()->setPCValue(
      newValues[newValues.size() - 1]);
}

// ! Getters and setters

/**
 * @brief Returns the view for this model.
 * @return RegistersView* The view for this model.
 */
RegistersView* const RegistersModel::getView() const {
  return view;
}

/**
 * @brief Gets the register values out of Jimulator.
 * @return const std::array<std::string, 16> An array containing all of the
 * register values.
 */
const std::array<std::string, 16>
RegistersModel::getRegisterValueFromJimulator() const {
  return Jimulator::getJimulatorRegisterValues();
}
