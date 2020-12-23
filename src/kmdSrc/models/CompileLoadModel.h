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
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * https://www.gnu.org/copyleft/gpl.html
 * @copyright Copyright (c) 2020
 */

#include <gtkmm/button.h>
#include <gtkmm/filechooserdialog.h>
#include <string>
#include "Model.h"

enum CompileLoadInnerState { FILE_SELECTED = 0, NO_FILE };

/**
 * @brief the class definition of the compileLoadModel class, a data model which
 * encapsulates any statefullness and logical operations associated with the
 * compile and loading section of the KoMo2 GUI. This Model is in keeping with
 * the MVC design pattern, with this class is the Model, the file display Label
 * is the View, and the compiling and file browsing buttons are the Controller.
 */
class CompileLoadModel : private Model {
 public:
  void changeJimulatorState(JimulatorState newState);
  void changeInnerState(CompileLoadInnerState newState);

  // Constructors
  CompileLoadModel(Gtk::Button* compileLoadButton,
                   Gtk::Button* browseButton,
                   KoMo2Model* parent);
  ~CompileLoadModel();

  // Click handlers
  void onCompileLoadClick();
  void onBrowseClick();

  // Getters and setters
  void setInnerState(CompileLoadInnerState val);
  CompileLoadInnerState getInnerState();
  void setAbsolutePathToSelectedFile(std::string val);
  std::string getAbsolutePathToSelectedFile();

 private:
  CompileLoadInnerState innerState;

  /**
   * @brief State - stores the value of the absolute file path to a `.s`
   * file, as chosen by the file browser component.
   */
  std::string absolutePathToSelectedFile;

  /**
   * @brief A pointer to the compile & load button, a controller which causes
   * this model to perform some events.
   */
  Gtk::Button* compileLoadButton;

  /**
   * @brief A pointer to the browse button, a controller which causes this model
   * to perform some event.
   */
  Gtk::Button* browseButton;

  // ! General functions
  std::string makeKmdPath(std::string absolutePath);
  void handleResult(int result, Gtk::FileChooserDialog* dialog);
};
