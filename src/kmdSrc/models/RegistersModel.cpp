/**
 * @file RegistersModel.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief This file defines the class RegistersModel, which represents the
 * logical data  that relates to the register view you see in the KoMo2 GUI. The
 * view is represented in the file `RegistersView.cpp` and it's associated
 * header.
 * @version 1.0.0
 * @date 10-04-2021
 */

#include <gdkmm/event.h>
#include <regex>
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
  int index;

  switch (e->keyval) {
    case GDK_KEY_0:
      index = 0;
      break;

    case GDK_KEY_1:
      index = 1;
      break;

    case GDK_KEY_2:
      index = 2;
      break;

    case GDK_KEY_3:
      index = 3;
      break;

    case GDK_KEY_4:
      index = 4;
      break;

    case GDK_KEY_5:
      index = 5;
      break;

    case GDK_KEY_6:
      index = 6;
      break;

    case GDK_KEY_7:
      index = 7;
      break;

    case GDK_KEY_8:
      index = 8;
      break;

    case GDK_KEY_9:
      index = 9;
      break;

    case GDK_KEY_a:
    case GDK_KEY_A:
      index = 10;
      break;

    case GDK_KEY_b:
    case GDK_KEY_B:
      index = 11;
      break;

    case GDK_KEY_c:
    case GDK_KEY_C:
      index = 12;
      break;

    case GDK_KEY_d:
    case GDK_KEY_D:
      index = 13;
      break;

    case GDK_KEY_e:
    case GDK_KEY_E:
      index = 14;
      break;

    case GDK_KEY_p:
    case GDK_KEY_P:
      index = 15;
      break;

    // any other key
    default:
      return false;
  }

  // Get the correct accessible object
  auto accessible = (*labelArray)[1][index].get_accessible();

  // Build a notification of the object and send the notification to the
  // screenreader
  accessible->set_role(Atk::ROLE_NOTIFICATION);
  accessible->notify_state_change(ATK_STATE_SHOWING, true);

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

    // A string describing the register

    std::string reg = i != 15 ? std::string("Register ")
                                    .append(std::to_string(i))
                                    .append(" stores ")
                              : "Program Counter stores ";

    // Set the accessibility object to describe
    (*labelArray)[1][i].get_accessible()->set_name(
        reg + std::regex_replace(newValues[i], std::regex("^0x0{0,7}"), ""));
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
