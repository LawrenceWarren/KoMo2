#include "ControlsModel.h"

class RegistersView;

class RegistersModel : private Model {
 public:
  RegistersModel(RegistersView* view, KoMo2Model* parent);

  virtual void changeJimulatorState(const JimulatorState newState) override;
  virtual const bool handleKeyPress(const GdkEventKey* const e) override;
  RegistersView* getView();

 private:
  /**
   * @brief The view this model represents.
   */
  RegistersView* view;

  // ! Deleted special member functions
  // stops these functions from being misused, creates a sensible error
  RegistersModel(const RegistersModel&) = delete;
  RegistersModel(const RegistersModel&&) = delete;
  RegistersModel& operator=(const RegistersModel&) = delete;
  RegistersModel& operator=(const RegistersModel&&) = delete;
};
