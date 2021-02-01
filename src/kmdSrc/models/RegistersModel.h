#include "ControlsModel.h"

class RegistersView;

class RegistersModel : private Model {
 public:
  RegistersModel(RegistersView* view, KoMo2Model* parent);
  ~RegistersModel();

  virtual void changeJimulatorState(JimulatorState newState) override;
  virtual bool handleKeyPress(GdkEventKey* e) override;
  RegistersView* getView();

 private:
  /**
   * @brief The view this model represents.
   */
  RegistersView* view;
};