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
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details at
 * https://www.gnu.org/copyleft/gpl.html
 */

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/window.h>
#include <string>
#include "CompileLoadView.h"
#include "ControlsView.h"
#include "RegistersView.h"

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

  // Getters and setters
  void setModel(KoMo2Model* val);
  KoMo2Model* getModel();
  void setStyling();
  CompileLoadView* getCompileLoadView();
  ControlsView* getControlsView();
  RegistersView* getRegistersView();

 private:
  void setSizes(int x, int y);

  // ! Layouts

  /**
   * @brief The master layout - every other view or layout should be nested
   * within this layout.
   */
  Gtk::VButtonBox masterLayout;

  /**
   * @brief The layout for the top bar running along the screen. Contains the
   * compile and load layout, and the program controls layout.
   */
  Gtk::HButtonBox controlsAndCompileBar;

  /**
   * @brief A box containing the browse button, the compile and load button, and
   * the selected file label.
   */
  CompileLoadView selectAndLoadContainer;

  /**
   * @brief A box containing all of the programs running controls.
   */
  ControlsView programControlsContainer;

  /**
   * @brief A box containing all of the registers
   */
  RegistersView registersView;

  // ! Other

  /**
   * @brief A pointer to the main model of the program.
   */
  KoMo2Model* model;

  //   // ! Deleted special member functions
  // stops these functions from being misused, creates a sensible error
  MainWindowView(const MainWindowView&) = delete;
  MainWindowView(const MainWindowView&&) = delete;
  MainWindowView& operator=(const MainWindowView&) = delete;
  MainWindowView& operator=(const MainWindowView&&) = delete;
};
