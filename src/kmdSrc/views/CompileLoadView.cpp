/**
 * @file CompileLoadView.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief A file containing definitions of the class declared in
 * `CompileLoadView.h`
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

#include "CompileLoadView.h"

/**
 * @brief Construct a new CompileLoadView.
 * @param parent A pointer to the parent.
 */
CompileLoadView::CompileLoadView(MainWindowView* const parent)
    : parent(parent),
      selectedFileLabel("File: "),
      compileAndLoadButton("Compile & Load"),
      browseButton("Select File") {
  initSelectAndLoadContainer();
}

/**
 * @brief Packs children into the compileLoadView, and sets the
 * layouts and size of it.
 */
void CompileLoadView::initSelectAndLoadContainer() {
  // Set button tooltips
  getBrowseButton()->set_tooltip_text(
      "Browse for an ARM assembly file (CTRL+L)");
  getCompileAndLoadButton()->set_tooltip_text(
      "Compile and load your file into Jimulator (CTRL+R)");

  // Set accessibility
  getBrowseButton()->get_accessible()->set_name("Browse files");
  getBrowseButton()->get_accessible()->set_description(
      "Open a file browser window.");

  getCompileAndLoadButton()->get_accessible()->set_name("Compile & load file");
  getCompileAndLoadButton()->get_accessible()->set_description(
      "Compiles and loads the selected file into Jimulator.");

  // button sizes
  getBrowseButton()->set_size_request(100, 33);
  getCompileAndLoadButton()->set_size_request(100, 33);
  getSelectedFileLabel()->set_size_request(100, 33);

  // Sets CSS
  // Adds a CSS class for the compiler buttons
  getCompileAndLoadButton()->get_style_context()->add_class("compButtons");
  getBrowseButton()->get_style_context()->add_class("compButtons");

  // Adds a CSS class for the label
  getSelectedFileLabel()->get_style_context()->add_class("fileLabel");

  // Packs into layout
  set_layout(Gtk::BUTTONBOX_END);
  pack_end(browseButton, false, false);
  pack_end(selectedFileLabel, false, false);
  pack_end(compileAndLoadButton, false, false);
  show_all_children();
  show();
}

/**
 * @brief Gets the `compileAndLoadButton` member variable.
 * @return Gtk::Button* A pointer to the `compileAndLoadButton` member
 * variable.
 */
Gtk::Button* const CompileLoadView::getCompileAndLoadButton() {
  return &compileAndLoadButton;
}
/**
 * @brief Gets the `browseButton` member variable.
 * @return Gtk::Button* A pointer to the `browseButton` member variable.
 */
Gtk::Button* const CompileLoadView::getBrowseButton() {
  return &browseButton;
}
/**
 * @brief Gets the `selectedFileLabel` member variable.
 * @return Gtk::Label* A pointer to the `selectedFileLabel` member variable.
 */
Gtk::Label* const CompileLoadView::getSelectedFileLabel() {
  return &selectedFileLabel;
}
/**
 * @brief Sets the text displayed by the `selectedFileLabel` member variable.
 * @param val The text to display in the `selectedFileLabel`.
 */
void CompileLoadView::setSelectedFileLabelText(const std::string val) {
  getSelectedFileLabel()->set_text(val);
}
/**
 * @brief Set the model pointer.
 * @param val The value to set the pointer to.
 */
void CompileLoadView::setModel(CompileLoadModel* const val) {
  model = val;
}
