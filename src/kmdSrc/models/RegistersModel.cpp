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
  // MOD1_MASK is the alt key - if not pressed, return
  if ((e->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK)) !=
      GDK_MOD1_MASK) {
    return false;
  }

  auto* const labelArray = getView()->getLabels();
  Glib::RefPtr<Atk::Object> accessible;
  int index;

  switch (e->keyval) {
    case GDK_KEY_0:
      index = 0;
      accessible = (*labelArray)[1][0].get_accessible();
      break;

    case GDK_KEY_1:
      index = 1;
      accessible = (*labelArray)[1][1].get_accessible();
      break;

    case GDK_KEY_2:
      index = 2;
      accessible = (*labelArray)[1][2].get_accessible();
      break;

    case GDK_KEY_3:
      index = 3;
      accessible = (*labelArray)[1][3].get_accessible();
      break;

    case GDK_KEY_4:
      index = 4;
      accessible = (*labelArray)[1][4].get_accessible();
      break;

    case GDK_KEY_5:
      index = 5;
      accessible = (*labelArray)[1][5].get_accessible();
      break;

    case GDK_KEY_6:
      index = 6;
      accessible = (*labelArray)[1][6].get_accessible();
      break;

    case GDK_KEY_7:
      index = 7;
      accessible = (*labelArray)[1][7].get_accessible();
      break;

    case GDK_KEY_8:
      index = 8;
      accessible = (*labelArray)[1][8].get_accessible();
      break;

    case GDK_KEY_9:
      index = 9;
      accessible = (*labelArray)[1][9].get_accessible();
      break;

    case GDK_KEY_a:
    case GDK_KEY_A:
      index = 10;
      accessible = (*labelArray)[1][10].get_accessible();
      break;

    case GDK_KEY_b:
    case GDK_KEY_B:
      index = 11;
      accessible = (*labelArray)[1][11].get_accessible();
      break;

    case GDK_KEY_c:
    case GDK_KEY_C:
      index = 12;
      accessible = (*labelArray)[1][12].get_accessible();
      break;

    case GDK_KEY_d:
    case GDK_KEY_D:
      index = 13;
      accessible = (*labelArray)[1][13].get_accessible();
      break;

    case GDK_KEY_e:
    case GDK_KEY_E:
      index = 14;
      accessible = (*labelArray)[1][14].get_accessible();
      break;

    case GDK_KEY_p:
    case GDK_KEY_P:
      index = 15;
      accessible = (*labelArray)[1][15].get_accessible();
      break;

    // any other key
    default:
      return false;
  }

  if ((*labelArray)[1][index].get_can_focus()) {
    std::cout << "Can focus already" << std::endl;
  } else {
    std::cout << "Can't focus already" << std::endl;
  }

  (*labelArray)[1][index].set_can_focus(true);
  (*labelArray)[1][index].grab_focus();

  if ((*labelArray)[1][index].get_can_focus()) {
    std::cout << "Can focus now" << std::endl;
  } else {
    std::cout << "Can't focus now" << std::endl;
  }

  getParent()->getMainWindow()->get_accessible()->get_parent();

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
    (*labelArray)[1][i].get_accessible()->set_name(newValues[i]);
  }

  // Send the new program counter value to disassembly model
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
