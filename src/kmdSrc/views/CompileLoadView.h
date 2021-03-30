/**
 * @file CompileLoadView.h
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief A file containing the declaration of the class `CompileLoadView.`
 * @version 0.1
 * @date 2020-12-28
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
#include <gtkmm/buttonbox.h>
#include <gtkmm/label.h>

class MainWindowView;
class CompileLoadModel;

/**
 * @brief This class represents the visual aspects that make up the compile &
 * load section of the KoMo2 GUI. This means that this describes layout and
 * visual information, but contains little or no logical information or
 * functionality.
 */
class CompileLoadView : public Gtk::VButtonBox {
 public:
  // Constructors and destructors
  CompileLoadView(MainWindowView* const parent);

  // Getters and setters
  Gtk::Button* const getCompileAndLoadButton();
  Gtk::Button* const getBrowseButton();
  Gtk::Label* const getSelectedFileLabel();
  void setSelectedFileLabelText(const std::string val);
  void setModel(CompileLoadModel* const val);

 private:
  /**
   * @brief A pointer to the parent view.
   */
  MainWindowView* const parent;

  /**
   * @brief A pointer to the related model.
   */
  CompileLoadModel* model;

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

  // General functions
  void initSelectAndLoadContainer();
  void initBrowseButton();
  void initCompileAndLoadButton();
  void initSelectedFileLabel();

  // ! Deleted special member functions
  // stops these functions from being misused, creates a sensible error
  CompileLoadView(const CompileLoadView&) = delete;
  CompileLoadView(const CompileLoadView&&) = delete;
  CompileLoadView& operator=(const CompileLoadView&) = delete;
  CompileLoadView& operator=(const CompileLoadView&&) = delete;
};
