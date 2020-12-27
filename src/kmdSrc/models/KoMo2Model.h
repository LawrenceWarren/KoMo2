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
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * https://www.gnu.org/copyleft/gpl.html
 * @copyright Copyright (c) 2020
 */

#include "ControlsModel.h"

class MainWindowView;
class ControlsView;
class CompileLoadView;

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
class KoMo2Model : private Model {
 private:
  bool handleKeyPress(GdkEventKey* e);

  /**
   * @brief A pointer to the main window view.
   */
  MainWindowView* mainWindow;

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

 public:
  template <class T1, class T2>
  void setButtonListener(Gtk::Button* button, T1 b, T2 c);

  // Constructors
  KoMo2Model(MainWindowView* mainWindow, std::string argv0);
  ~KoMo2Model();

  // General functions
  void changeImage(Gtk::Image* toChange, Gtk::Image* newImg);
  void changeJimulatorState(JimulatorState newState);

  // Getters & setters
  CompileLoadModel* getCompileLoadModel();
  ControlsModel* getControlsModel();
  MainWindowView* getMainWindow();
  const std::string getAbsolutePathToProjectRoot();
};
