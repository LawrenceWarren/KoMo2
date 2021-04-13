/**
 * @file ControlsView.h
 * @author Lawrence Warren (lawrencewarren@gmail.com)
 * @brief A definition of the class `ControlsView`.
 * @version 1.0.0
 * @date 10-04-2021
 */

#include <gtkmm/button.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/linkbutton.h>

class MainWindowView;
class ControlsModel;

/**
 * @brief `ControlsView` is the visual aspect of the controls that run along the
 * top of the KoMo2 GUI. This class contains information and functionality
 * relating to the getting and setting of visual information, such as the layout
 * and styling. Little or no logic or data is kept in this class.
 */
class ControlsView : public Gtk::HButtonBox {
 public:
  // Constructors and destructors
  ControlsView(MainWindowView* const parent);

  // Getters and setters
  Gtk::LinkButton* const getHelpButton();
  Gtk::Button* const getReloadJimulatorButton();
  Gtk::Button* const getPauseResumeButton();
  Gtk::Button* const getSingleStepExecuteButton();
  Gtk::Button* const getHaltExecutionButton();
  void setModel(ControlsModel* const val);
  void setButtonImages(const std::string projectRoot);

 private:
  /**
   * @brief A pointer to the parent view.
   */
  MainWindowView* const parent;

  /**
   * @brief A pointer to the related model.
   */
  ControlsModel* model;

  /**
   * @brief A button which, when clicked, opens up an about/help window.
   */
  Gtk::LinkButton helpButton;

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

  // General functions
  void initProgramControlsContainer();
  void initHelpButton();
  void initHaltExecutionButton();
  void initSingleStepExecuteButton();
  void initReloadJimulatorButton();
  void initPauseResumeButton();

  // ! Deleted special member functions
  // stops these functions from being misused, creates a sensible error
  ControlsView(const ControlsView&) = delete;
  ControlsView(const ControlsView&&) = delete;
  ControlsView& operator=(const ControlsView&) = delete;
  ControlsView& operator=(const ControlsView&&) = delete;
};
