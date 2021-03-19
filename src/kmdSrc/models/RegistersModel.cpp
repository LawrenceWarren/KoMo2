#include "../views/RegistersView.h"
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
  return false;
}

/**
 * @brief Returns the view for this model.
 * @return RegistersView* The view for this model.
 */
RegistersView* const RegistersModel::getView() {
  return view;
}

/**
 * @brief Gets the register values out of Jimulator.
 * @return const std::array<std::string, 16> An array containing all of the
 * register values.
 */
const std::array<std::string, 16>
RegistersModel::getRegisterValueFromJimulator() const {
  switch (checkBoardState()) {
    case 0X44:
      std::cout << "program halted" << std::endl;
      getParent()->changeJimulatorState(UNLOADED);
      break;
    case 0X82:
      // std::cout << "awaiting input 0X82" << std::endl;
      // getParent()->changeJimulatorState(AWAITING_INPUT);
      break;
    case 0X80:
      // std::cout << "awaiting input 0X80" << std::endl;
      // getParent()->changeJimulatorState(AWAITING_INPUT);
      break;
    default:
      std::cout << "Nothing to report" << std::endl;
  }

  return getJimulatorRegisterValues();
}
