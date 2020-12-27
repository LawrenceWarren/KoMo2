/**
 * @file MainWindowView.h
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief A class declaration for the `MainWindowView` class, which inherits
 * from `GTK::Window`.
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
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/window.h>
#include <string>
#include "ControlsView.h"

class KoMo2Model;

/**
 * @brief The class definition of the main window of the program. This main
 * window is the mater view of the program. Within this class are all of the
 * child views, controllers, and a pointer to the master model, as keeping
 * with MVC.
 */
class MainWindowView : public Gtk::Window {
 public:
  // Constructors
  MainWindowView(int x, int y);
  virtual ~MainWindowView();

  // Getters and setters
  Gtk::Button* getCompileAndLoadButton();
  Gtk::Button* getBrowseButton();
  void setModel(KoMo2Model* val);
  KoMo2Model* getModel();
  Gtk::Label* getSelectedFileLabel();
  void setSelectedFileLabelText(std::string val);
  void setStyling();
  ControlsView* getControlsView();

 private:
  void initSelectAndLoadContainer();
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
  ControlsView programControlsContainer;

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

  // ! Other

  /**
   * @brief A pointer to the main model of the program.
   */
  KoMo2Model* model;
};
