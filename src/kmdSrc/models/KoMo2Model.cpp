/**
 * @file KoMo2Model.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief Definitions of the functions declared in the class definition, found
 * at `KoMo2Model.h`.
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

#include "KoMo2Model.h"
#include "../views/MainWindow.h"

/**
 * @brief Construct a new KoMo2Model - this constructor initialises the
 * mainWindow pointer, as well as the absolutePathToProjectRoot member. It then
 * constructs a member compileLoadModel, and sets the compile buttons on click
 * events.
 * @param mainWindow A pointer to the mainWindow view object.
 * @param argv0 The absolutePathToProjectRoot - parsed from argv[0].
 */
KoMo2Model::KoMo2Model(MainWindow* mainWindow, std::string argv0)
    : mainWindow(mainWindow),
      absolutePathToProjectRoot(argv0),
      compileLoadModel(mainWindow->getCompileAndLoadButton(),
                       mainWindow->getBrowseButton(),
                       this) {
  // Updates the main window to have a pointer to its model, sets its CSS.
  getMainWindow()->setModel(this);
  getMainWindow()->setCSS();

  // Set the onClick events for the browse and compile and load buttons to
  // be wired to CompileLoadModel member functions.
  getMainWindow()->getBrowseButton()->signal_clicked().connect(
      sigc::mem_fun(*getCompileLoadModel(), &CompileLoadModel::onBrowseClick));

  getMainWindow()->getCompileAndLoadButton()->signal_clicked().connect(
      sigc::mem_fun(*getCompileLoadModel(),
                    &CompileLoadModel::onCompileLoadClick));
}

/**
 * @brief Destroys a KoMo2Model.
 */
KoMo2Model::~KoMo2Model() {}

// ! Getter functions

/**
 * @brief Gets the `mainWindow` member variable.
 * @return MainWindow* A pointer to the `MainWindow.`
 */
MainWindow* KoMo2Model::getMainWindow() {
  return mainWindow;
}
/**
 * @brief Gets the `absolutePathToProjectRoot` member variable.
 * @return const std::string The absolute path to the project root.
 */
const std::string KoMo2Model::getAbsolutePathToProjectRoot() {
  return absolutePathToProjectRoot;
}
/**
 * @brief Gets the `compileLoadModel` member variable.
 * @return CompileLoadModel* A pointer to the `compileLoadModel`.
 */
CompileLoadModel* KoMo2Model::getCompileLoadModel() {
  return &compileLoadModel;
}
