/**
 * @file Model.h
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief A file containing the declaration of the class `Model` and the
 * enumerable type `JimulatorState`.
 * @version 1.0.0
 * @date 10-04-2021
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

#include <gtkmm.h>
#include <gtkmm/button.h>
#include <gtkmm/image.h>
#include <string>
#include "../jimulatorInterface.h"

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
  friend class KoMo2Model;  // Only KoMo2Model can access setJimulatorState()

 public:
  Model(KoMo2Model* const parent);
  KoMo2Model* const getParent() const;

 protected:
  /**
   * @brief Connect any button to any member function of
   * @tparam T1 A pointer type to some object of any type.
   * @tparam T2 A pointer to a T1 member function.
   * @param button A pointer to the button to set the `onClick` event for.
   * @param b A pointer to some object.
   * @param c A pointer to some member function of the b object.
   */
  template <class T1, class T2>
  void setButtonListener(Gtk::Button* const button, const T1 b, const T2 c) {
    button->signal_clicked().connect(sigc::mem_fun(*b, c));
  }

  void setButtonState(Gtk::Button* const button,
                      const bool state,
                      Gtk::Image* const img = nullptr,
                      const std::string newTooltip = "",
                      const std::string newLabelText = "") const;

  /**
   * @brief Handles key presses for each model.
   * @param e The key press event.
   * @return bool If a key pressed.
   */
  virtual const bool handleKeyPress(const GdkEventKey* const e) = 0;

  /**
   * @brief Changes the state of Jimulator into newState.
   * This function does not necessarily need to be virtual, as at no point
   * should a child be stored as Model pointer. However, by making it a pure
   * virtual function, we ensure this class cannot be instantiated.
   * @param newState The state to change the program to.
   */
  virtual void changeJimulatorState(const JimulatorState newState) = 0;

  // Getters and setters

  JimulatorState getJimulatorState() const;

 private:
  /**
   * @brief All models have a parent model - KoMo2Model, the most senior model
   * in the hierarchy, sets its parent to `self`.
   */
  KoMo2Model* const parent;

  /**
   * @brief JimulatorState reflects the state in which Jimulator is operating,
   * which in turn affects the model of this process and it the views.
   * JimulatorState is static, so all subclasses of Model can simply call
   * `getState()` on themselves and have the expected value.
   */
  static JimulatorState jimulatorState;

  // Only friends can access
  void setJimulatorState(const JimulatorState val);

  // ! Deleted special member functions
  // stops these functions from being misused, creates a sensible error
  Model(const Model&) = delete;
  Model(const Model&&) = delete;
  Model& operator=(const Model&) = delete;
  Model& operator=(const Model&&) = delete;
};
