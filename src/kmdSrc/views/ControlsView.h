
#include <gtkmm/button.h>
#include <gtkmm/buttonbox.h>

class MainWindowView;
class ControlsViewModel;

class ControlsView : public Gtk::HButtonBox {
 private:
  MainWindowView* parent;
  ControlsViewModel* model;

  /**
   * @brief A button which, when clicked, opens up an about/help window.
   */
  Gtk::Button helpButton;

  /**
   * @brief A button which, when clicked, reloads the program into Jimulator
   * again.
   */
  Gtk::Button reloadJimulatorButton;

  /**
   * @brief A button which, when clicked, toggles between playing and pausing
   * the execution of Jimulator (i.e. if currently paused, play, and
   * vice-versa)
   */
  Gtk::Button pauseResumeButton;

  /**
   * @brief A button which, when clicked, performs a single-step of execution
   * IF Jimulator is already paused.
   */
  Gtk::Button singleStepExecuteButton;

  /**
   * @brief A button which, when clicked, halts the current execution of
   * Jimulator.
   */
  Gtk::Button haltExecutionButton;

 public:
  void initProgramControlsContainer();
  ControlsView(MainWindowView* parent);
  ~ControlsView();

  Gtk::Button* getHelpButton();
  Gtk::Button* getReloadJimulatorButton();
  Gtk::Button* getPauseResumeButton();
  Gtk::Button* getSingleStepExecuteButton();
  Gtk::Button* getHaltExecutionButton();

  void setModel(ControlsViewModel* val);
};