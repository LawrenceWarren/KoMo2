#include "ControlsModel.h"

class RegistersView;

class RegistersModel : private Model {
 public:
  RegistersModel(RegistersView* view, KoMo2Model* parent);

  virtual void changeJimulatorState(JimulatorState newState) override;
  virtual bool handleKeyPress(GdkEventKey* e) override;
  RegistersView* getView();

 private:
  /**
   * @brief The view this model represents.
   */
  RegistersView* view;

  // Deleted SMFS - stops these from being misused, creates a sensible error
  RegistersModel(const RegistersModel&) = delete;
  RegistersModel(const RegistersModel&&) = delete;
  RegistersModel& operator=(const RegistersModel&) = delete;
  RegistersModel& operator=(const RegistersModel&&) = delete;
};
