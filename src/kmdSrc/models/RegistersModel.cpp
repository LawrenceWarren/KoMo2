#include "../views/RegistersView.h"
#include "KoMo2Model.h"

/**
 * @brief Constructs a new registers model object.
 * @param view A pointer to the view this register represents.
 * @param parent A pointer to the parent model.
 */
RegistersModel::RegistersModel(RegistersView* view, KoMo2Model* parent)
    : Model(parent), view(view) {
  view->setModel(this);
}

/**
 * @brief Destroys an existing registers model object.
 */
RegistersModel::~RegistersModel() {}

/**
 * @brief Handles changes in the Jimulator state.
 * @param newState The state being changed into.
 */
void RegistersModel::changeJimulatorState(JimulatorState newState) {}

/**
 * @brief Handles any key press events.
 * @param e The key press event.
 * @return bool true if the key press was handled.
 */
bool RegistersModel::handleKeyPress(GdkEventKey* e) {
  return false;
}

/**
 * @brief Returns the view for this model.
 * @return RegistersView* The view for this model.
 */
RegistersView* RegistersModel::getView() {
  return view;
}