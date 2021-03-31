/**
 * @file KoMo2Model.h
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief A file containing the definition of the KoMo2Model class.
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

#include <gtkmm/filechooserdialog.h>
#include <string>
#include "DisassemblyModel.h"

class MainWindowView;
class ControlsView;
class CompileLoadView;
class RegistersView;
class TerminalView;
class DisassemblyView;

/**
 * @brief The logical model of the entire application. All other models should
 * be member variables of this model.
 *
 * This model exists in case separate models pertaining to different GUI
 * elements need to logically interact - for example, the control bar at the top
 * of the KoMo2 GUI window has buttons (controllers) which operate Jimulator,
 * which will cause some function to run in this class (model) which in turn
 * will have to affect the program memory window GUI element (view).
 *
 * This model is in keeping with the MVC design pattern, where this class is
 * the Model, and the main KoMo2 window contains all of the Controllers and
 * Views.
 */
class KoMo2Model : public Model {
 public:
  KoMo2Model(MainWindowView* const mainWindow, const std::string argv0);
  virtual void changeJimulatorState(const JimulatorState newState) override;
  const bool refreshViews();

  // Getters
  const std::string getAbsolutePathToProjectRoot() const;
  MainWindowView* const getMainWindow() const;
  CompileLoadModel* const getCompileLoadModel();
  ControlsModel* const getControlsModel();
  RegistersModel* const getRegistersModel();
  TerminalModel* const getTerminalModel();
  DisassemblyModel* const getDisassemblyModel();

 private:
  virtual const bool handleKeyPress(const GdkEventKey* const e) override;

  /**
   * @brief A pointer to the main window view.
   */
  MainWindowView* const mainWindow;

  /**
   * @brief The absolute path to the project root directory.
   */
  const std::string absolutePathToProjectRoot;

  /**
   * @brief The data model for the compile and load functionality of the
   * program, represented by the browse and compile and load buttons in the
   * view.
   */
  CompileLoadModel compileLoadModel;

  /**
   * @brief The data model for the controls and status functionality of the
   * program, represented by the series of buttons and labels running along
   * the top of the view.
   */
  ControlsModel controlsModel;

  /**
   * @brief The data model for the registers view. Represented by the table
   * of values on the left hand side of the main view.
   */
  RegistersModel registersModel;

  /**
   * @brief The data model associated with the terminal view. Represented by the
   * input box and text view at the bottom of the view.
   */
  TerminalModel terminalModel;

  /**
   * @brief The data model associated with the disassembly view. Represents the
   * rows of memory values that take up the majority of the view.
   */
  DisassemblyModel disassemblyModel;

  /**
   * @brief Defines how often the the refreshViews function should be called
   * when KoMo2 is in the RUNNING state.
   */
  const unsigned int refreshRate = 50;  // TODO: read this from an external file

  // ! Deleted special member functions
  // stops these functions from being misused, creates a sensible error
  KoMo2Model(const KoMo2Model&) = delete;
  KoMo2Model(const KoMo2Model&&) = delete;
  KoMo2Model& operator=(const KoMo2Model&) = delete;
  KoMo2Model& operator=(const KoMo2Model&&) = delete;
};
