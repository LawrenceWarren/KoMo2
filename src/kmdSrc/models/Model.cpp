/**
 * @file Model.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief Definitions of the functions and variables found in the class
 * declaration in `Model.h`.
 * @version 0.1
 * @date 2020-12-23
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

#include "Model.h"

// Sets the static jimulatorState to its initial value
JimulatorState Model::jimulatorState = INITIAL;

/**
 * @brief Returns the parent pointer.
 * @return KoMo2Model* The parent pointer.
 */
KoMo2Model* Model::getParent() {
  return parent;
}

/**
 * @brief Constructs a new Model object - just assigns the parent variable.
 * @param parent The parent pointer.
 */
Model::Model(KoMo2Model* parent) : parent(parent) {}

/**
 * @brief Return the jimulatorState member object.
 * @return JimulatorState the jimulatorState member.
 */
JimulatorState Model::getJimulatorState() {
  return Model::jimulatorState;
}

/**
 * @brief set the JimulatorState member object.
 * @param val The value to set JimulatorState to.
 */
void Model::setJimulatorState(JimulatorState val) {
  Model::jimulatorState = val;
}
