/**
 * @file compileLoadController.h
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief A file containing the definition of the CompileLoadModel class.
 * @version 0.1
 * @date 2020-12-22
 * @section LICENSE
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details at
 * https://www.gnu.org/copyleft/gpl.html
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
enum CompileLoadInnerState : int {
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
  void handleResult(const int result,
                    const Gtk::FileChooserDialog* const dialog);

  // ! Deleted special member functions
  // stops these functions from being misused, creates a sensible error
  CompileLoadModel(const CompileLoadModel&) = delete;
  CompileLoadModel(const CompileLoadModel&&) = delete;
  CompileLoadModel& operator=(const CompileLoadModel&) = delete;
  CompileLoadModel& operator=(const CompileLoadModel&&) = delete;
};
