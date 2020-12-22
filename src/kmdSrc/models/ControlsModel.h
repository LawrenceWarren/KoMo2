#include <gtkmm/button.h>

class KoMo2Model;

class ControlsModel {
 public:
  ControlsModel(Gtk::Button* helpButton,
                Gtk::Button* beginRunJimulatorButton,
                Gtk::Button* reloadJimulatorButton,
                Gtk::Button* pauseResumeButton,
                Gtk::Button* singleStepExecuteButton,
                Gtk::Button* haltExecutionButton,
                KoMo2Model* parent);
  ~ControlsModel();

  void onHelpClick();
  void onBeginRunJimulatorClick();
  void onReloadJimulatorClick();
  void onPauseResumeClick();
  void onSingleStepExecuteClick();
  void onHaltExecutionClick();

 private:

  Gtk::Button* helpButton;
  Gtk::Button* beginRunJimulatorButton;
  Gtk::Button* reloadJimulatorButton;
  Gtk::Button* pauseResumeButton;
  Gtk::Button* singleStepExecuteButton;
  Gtk::Button* haltExecutionButton;
  KoMo2Model* parent;
};