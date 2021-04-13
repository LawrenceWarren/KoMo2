/**
 * @file ControlsModel.h
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief A file containing the definition of the ControlsModel class.
 * @version 1.0.0
 * @date 10-04-2021
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