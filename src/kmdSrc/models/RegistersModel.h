/**
 * @file RegistersModel.h
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief This file declares the class RegistersModel, which represents the
 * logical data in memory that relates to the register view you see in the KoMo2
 * GUI. The view is represented in the file `RegistersView.cpp` and it's
 * associated header.
 * @version 1.0.0
 * @date 10-04-2021
 */

#include "ControlsModel.h"

class RegistersView;

/**
 * @brief Stores data and functions relating to the tracking and manipulation of
 * data within the RegistersView view object.
 */
class RegistersModel : private Model {
 public:
  RegistersModel(RegistersView* const view, KoMo2Model* const parent);
  virtual void changeJimulatorState(const JimulatorState newState) override;
  virtual const bool handleKeyPress(const GdkEventKey* const e) override;
  RegistersView* const getView() const;
  void refreshViews();

 private:
  /**
   * @brief The view this model represents.
   */
  RegistersView* const view;

  const std::array<std::string, 16> getRegisterValueFromJimulator() const;

  // ! Deleted special member functions
  // stops these functions from being misused, creates a sensible error
  RegistersModel(const RegistersModel&) = delete;
  RegistersModel(const RegistersModel&&) = delete;
  RegistersModel& operator=(const RegistersModel&) = delete;
  RegistersModel& operator=(const RegistersModel&&) = delete;
};
