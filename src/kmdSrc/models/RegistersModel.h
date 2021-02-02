#include "ControlsModel.h"

class RegistersView;

class RegistersModel : private Model {
 public:
  RegistersModel(RegistersView* const view, KoMo2Model* const parent);
  virtual void changeJimulatorState(const JimulatorState newState) override;
  virtual const bool handleKeyPress(const GdkEventKey* const e) override;
  RegistersView* const getView();

 private:
  /**
   * @brief The view this model represents.
   */
  RegistersView* const view;

  // ! Deleted special member functions
  // stops these functions from being misused, creates a sensible error
  RegistersModel(const RegistersModel&) = delete;
  RegistersModel(const RegistersModel&&) = delete;
  RegistersModel& operator=(const RegistersModel&) = delete;
  RegistersModel& operator=(const RegistersModel&&) = delete;
};
