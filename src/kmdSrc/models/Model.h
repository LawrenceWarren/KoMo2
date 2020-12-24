/**
 * @file Model.h
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief A file containing the declaration of the class `Model` and the
 * enumerable type `JimulatorState`.
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

/**
 * @brief Describe the 5 states of Jimulator
 */
enum JimulatorState : int {
  INITIAL = -1,  // Jimulator starting state
  UNLOADED,      // Jimulator idle; needs a program to be loaded in.
  LOADED,        // File just loaded for first time (not yet run)
  RUNNING,       // Jimulator is running
  PAUSED,        // Jimulator is paused
};

class KoMo2Model;

/**
 * @brief The superclass for all other Model classes. Uses a pure virtual
 * function, so is abastract. Keeps KoMo2Model as a friend so it alone can call
 * `setJimulatorState`.
 * This class provides basic data that are needed by all other
 */
class Model {
 public:
  Model(KoMo2Model* parent);
  friend class KoMo2Model;  // Only KoMo2Model can access setJimulatorState()

  /**
   * @brief Changes the state of Jimulator into newState.
   * This function does not necessarily need to be virtual, as at no point
   * should a child be stored as Model pointer. However, by making it a pure
   * virtual function, we ensure this class cannot be instantiated.
   * @param newState The state to change the program to.
   */
  virtual void changeJimulatorState(JimulatorState newState) = 0;

  // Getters and setters
  KoMo2Model* getParent();
  JimulatorState getJimulatorState();

 private:
  void setJimulatorState(JimulatorState val);  // Only friends can access

  /**
   * @brief All models have a parent model - KoMo2Model, the most senior model
   * in the hierarchy, sets its parent to `self`.
   */
  KoMo2Model* parent;

  /**
   * @brief JimulatorState reflects the state in which Jimulator is operating,
   * which in turn affects the model of this process and it the views.
   * JimulatorState is static, so all subclasses of Model can simply call
   * `getState()` on themselves and have the expected value.
   */
  static JimulatorState jimulatorState;
};
