/**
 * @file compileLoadController.h
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief A file containing the definition of the CompileLoadModel class.
 * @version 1.0.0
 * @date 10-04-2021
 */

#include <gtkmm/button.h>
#include <gtkmm/filechooserdialog.h>
#include <string>
#include "Model.h"

class CompileLoadView;

/**
 * @brief An enum indicating the state of the specific compile and load section
 * of the GUI - specifically, whether a file had been selected or not.
 */
enum class CompileLoadInnerState : int {
  FILE_SELECTED = 0,  // A file has been selected.
  NO_FILE             // No file has been selected.
};

/**
 * @brief the class definition of the compileLoadModel class, a data model which
 * encapsulates any statefullness and logical operations associated with the
 * compile and loading section of the KoMo2 GUI. This Model is in keeping with
 * the MVC design pattern, with this class is the Model, the file display Label
 * is the View, and the compiling and file browsing buttons are the Controller.
 */
class CompileLoadModel : private Model {
 public:
  CompileLoadModel(CompileLoadView* const view, KoMo2Model* const parent);
  virtual void changeJimulatorState(const JimulatorState newState) override;
  virtual const bool handleKeyPress(const GdkEventKey* const e) override;

 private:
  /**
   * @brief A pointer to the view that this model represents.
   */
  CompileLoadView* const view;

  /**
   * @brief Stores the state of the compile and load section of the GUI.
   */
  CompileLoadInnerState innerState;

  /**
   * @brief State - stores the value of the absolute file path to a `.s`
   * file, as chosen by the file browser component.
   */
  std::string absolutePathToSelectedFile;

  // ! Getters and setters
  // Inner state
  const CompileLoadInnerState getInnerState() const;
  void setInnerState(const CompileLoadInnerState newState);

  // absolute path to selected file
  const std::string getAbsolutePathToSelectedFile() const;
  void setAbsolutePathToSelectedFile(const std::string val);

  // ! General functions
  void onBrowseClick();
  void onCompileLoadClick() const;
  const std::string makeKmdPath(const std::string absolutePath) const;
  void handleResultFromFileBrowser(const int result,
                                   const Gtk::FileChooserDialog* const dialog);

  // ! Deleted special member functions
  // stops these functions from being misused, creates a sensible error
  CompileLoadModel(const CompileLoadModel&) = delete;
  CompileLoadModel(const CompileLoadModel&&) = delete;
  CompileLoadModel& operator=(const CompileLoadModel&) = delete;
  CompileLoadModel& operator=(const CompileLoadModel&&) = delete;
};
