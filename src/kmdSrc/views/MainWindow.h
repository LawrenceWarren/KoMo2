/**
 * @file MainWindow.h
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief A class declaration for the `MainWindow` class, which inherits from
 * `GTK::Window`.
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

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/label.h>
#include <gtkmm/window.h>
#include <string>

class KoMo2Model;

/**
 * @brief The class definition of the main window of the program. This main
 * window is the mater view of the program. Within this class are all of the
 * child views, controllers, and a pointer to the master model, as keeping
 * with MVC.
 */
class MainWindow : public Gtk::Window {
 public:
  // Constructors
  MainWindow(int x, int y);
  virtual ~MainWindow();

  // Getters and setters
  Gtk::Button* getCompileAndLoadButton();
  Gtk::Button* getBrowseButton();
  void setModel(KoMo2Model* val);
  KoMo2Model* getModel();
  Gtk::Label* getSelectedFileLabel();
  void setSelectedFileLabelText(std::string val);
  void setStyling();
  Gtk::Button* getHelpButton();
  Gtk::Button* getBeginRunJimulatorButton();
  Gtk::Button* getReloadJimulatorButton();
  Gtk::Button* getPauseResumeButton();
  Gtk::Button* getSingleStepExecuteButton();
  Gtk::Button* getHaltExecutionButton();

 private:
  void initSelectAndLoadContainer();
  void initProgramControlsContainer();
  void setSizes(int x, int y);

  // ! Layouts

  /**
   * @brief The master layout - every other view or layout should be nested
   * within this layout.
   */
  Gtk::Box masterLayout;

  /**
   * @brief A box containing the browse button, the compile and load button, and
   * the selected file label.
   */
  Gtk::VButtonBox selectAndLoadContainer;

  /**
   * @brief The layout for the top bar running along the screen. Contains the
   * compile and load layout, and the program controls layout.
   */
  Gtk::HButtonBox controlsAndCompileBar;

  /**
   * @brief A box containing all of the programs running controls.
   */
  Gtk::HButtonBox programControlsContainer;

  // ! Views

  /**
   * @brief A label which displays whatever file has been selected by the
   * browse button, to be compiled & loaded.
   */
  Gtk::Label selectedFileLabel;

  /**
   * @brief A button which, when clicked, will read a .s file, compile it to
   * .kmd, and the load it into Jimulator.
   */
  Gtk::Button compileAndLoadButton;

  /**
   * @brief A button which allows you to browse the file system for .s files.
   */
  Gtk::Button browseButton;

  /**
   * @brief A button which, when clicked, opens up an about/help window.
   */
  Gtk::Button helpButton;

  /**
   * @brief A button which, when clicked, commences running of the Jimulator
   * program.
   */
  Gtk::Button beginRunJimulatorButton;

  /**
   * @brief A button which, when clicked, reloads the program into Jimulator
   * again.
   */
  Gtk::Button reloadJimulatorButton;

  /**
   * @brief A button which, when clicked, toggles between playing and pausing
   * the execution of Jimulator (i.e. if currently paused, play, and vice-versa)
   */
  Gtk::Button pauseResumeButton;

  /**
   * @brief A button which, when clicked, performs a single-step of execution IF
   * Jimulator is already paused.
   */
  Gtk::Button singleStepExecuteButton;

  /**
   * @brief A button which, when clicked, halts the current execution of
   * Jimulator.
   */
  Gtk::Button haltExecutionButton;

  // ! Other

  /**
   * @brief A pointer to the main model of the program.
   */
  KoMo2Model* model;
};
