#include "../views/RegistersView.h"
#include "KoMo2Model.h"

RegistersModel::RegistersModel(RegistersView* view, KoMo2Model* parent)
    : Model(parent), view(view) {
      
  view->setModel(this);
}

RegistersModel::~RegistersModel() {}

void RegistersModel::changeJimulatorState(JimulatorState newState) {}

bool RegistersModel::handleKeyPress(GdkEventKey* e) {
  return false;
}