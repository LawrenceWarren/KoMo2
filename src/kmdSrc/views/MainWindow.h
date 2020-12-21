/**
 * @file MainWindow.h
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief A class declaration for the `MainWindow` GTKMM object, which inherits
 * from `GTK::Window`.
 * @version 0.1
 * @date 2020-11-27
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * https://www.gnu.org/copyleft/gpl.html
 *
 * @copyright Copyright (c) 2020
 *
 */

#include <gtkmm/button.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/label.h>
#include <gtkmm/window.h>
#include <string>

class KoMo2Model;

/**
 * @brief The mainWindow of the program.
 */
class MainWindow : public Gtk::Window {
 public:
  // Constructors
  MainWindow();
  virtual ~MainWindow();

  // Getters and setters
  Gtk::Button* getCompileAndLoadButton();
  Gtk::Button* getBrowseButton();
  void setModel(KoMo2Model* val);
  KoMo2Model* getModel();
  Gtk::Label* getSelectedFileLabel();
  void setSelectedFileLabel(std::string val);
  void setCSS();

  // Child widgets & layouts
 private:
  // Models
  KoMo2Model* model;

  // Views
  Gtk::VButtonBox selectAndLoadContainer;
  Gtk::Label selectedFileLabel;
  Gtk::Button compileAndLoadButton;
  Gtk::Button browseButton;
};
