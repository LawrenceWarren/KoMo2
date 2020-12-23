#include "Model.h"
#include <iostream>

JimulatorState Model::jimulatorState = INITIAL;

KoMo2Model* Model::getParent() {
  return parent;
}

void Model::setParent(KoMo2Model* val) {
  parent = val;
}

JimulatorState Model::getJimulatorState() {
  return Model::jimulatorState;
}

void Model::setJimulatorState(JimulatorState val) {
  std::cout << "STATE CHANGE " << val << std::endl;
  Model::jimulatorState = val;
}
