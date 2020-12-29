#include "ControlsModel.h"

class RegistersView;

class RegistersModel : private Model {
 public:
  RegistersModel(RegistersView* view, KoMo2Model* parent);
  ~RegistersModel();

  void changeJimulatorState(JimulatorState newState);
  bool handleKeyPress(GdkEventKey* e);
  RegistersView* getView();

 private:
  RegistersView* view;
};