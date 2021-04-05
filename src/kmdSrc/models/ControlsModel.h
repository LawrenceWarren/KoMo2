/**
 * @file ControlsModel.h
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief A file containing the definition of the ControlsModel class.
 * @version 0.1
 * @date 2020-12-22
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
 */

#include <gtkmm/button.h>
#include "CompileLoadModel.h"

class ControlsView;

/**
 * @brief The class definition of the ControlsModel class, a data model which
 * encapsulates all state, data and functionality of the Jimulator controls of
 * The KoMo2 GUI. This model is in keeping with the MVC design pattern, where
 * this class is a Model, the status display label is a View, and the Jimulator
 * control buttons are Controllers.
 */
class ControlsModel : private Model {
 public:
  ControlsModel(ControlsView* const view,
                const std::string manual,
                KoMo2Model* const parent);
  virtual const bool handleKeyPress(const GdkEventKey* const e) override;
  virtual void changeJimulatorState(const JimulatorState newState) override;

 private:
  /**
   * @brief A pointer to the view which this model represents.
   */
  ControlsView* const view;

  // Click handlers
  void onReloadJimulatorClick();
  void onPauseResumeClick();
  void onSingleStepExecuteClick();
  void onHaltExecutionClick();

  // ! Deleted special member functions
  // stops these functions from being misused, creates a sensible error
  ControlsModel(const ControlsModel&) = delete;
  ControlsModel(const ControlsModel&&) = delete;
  ControlsModel& operator=(const ControlsModel&) = delete;
  ControlsModel& operator=(const ControlsModel&&) = delete;
};