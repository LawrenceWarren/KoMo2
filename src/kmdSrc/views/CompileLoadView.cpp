/**
 * @file CompileLoadView.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief A file containing definitions of the class declared in
 * `CompileLoadView.h`
 * @version 1.0.0
 * @date 10-04-2021
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
  initBrowseButton();
  initCompileAndLoadButton();
  initSelectedFileLabel();
  initSelectAndLoadContainer();
}

/**
 * @brief Packs children into the `compileLoadView,` and sets its layout and
 * size.
 */
void CompileLoadView::initSelectAndLoadContainer() {
  set_layout(Gtk::BUTTONBOX_END);
  pack_end(browseButton, false, false);
  pack_end(selectedFileLabel, false, false);
  pack_end(compileAndLoadButton, false, false);
  show_all_children();
  show();
}

/**
 * @brief Sets up the initial information about the browse file button.
 */
void CompileLoadView::initBrowseButton() {
  getBrowseButton()->set_tooltip_text(
      "Browse for an ARM assembly file (CTRL+L)");
  getBrowseButton()->get_accessible()->set_name("Browse files");
  getBrowseButton()->get_accessible()->set_description(
      "Open a file browser window.");
  getBrowseButton()->set_size_request(100, 33);
  getBrowseButton()->get_style_context()->add_class("compButtons");
}

/**
 * @brief Sets up the initial information about the compile & load button.
 */
void CompileLoadView::initCompileAndLoadButton() {
  getCompileAndLoadButton()->set_tooltip_text(
      "Compile and load your file into Jimulator (CTRL+R)");
  getCompileAndLoadButton()->get_accessible()->set_name("Compile & load file");
  getCompileAndLoadButton()->get_accessible()->set_description(
      "Compiles and loads the selected file into Jimulator.");
  getCompileAndLoadButton()->set_size_request(100, 33);
  getCompileAndLoadButton()->get_style_context()->add_class("compButtons");
}

/**
 * @brief Sets up the initial information about the selected file label.
 */
void CompileLoadView::initSelectedFileLabel() {
  getSelectedFileLabel()->set_size_request(100, 33);
  getSelectedFileLabel()->get_style_context()->add_class("fileLabel");
  getSelectedFileLabel()->get_accessible()->set_name("Selected file");
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
  getSelectedFileLabel()->get_accessible()->set_description(val);
}
/**
 * @brief Set the model pointer.
 * @param val The value to set the pointer to.
 */
void CompileLoadView::setModel(CompileLoadModel* const val) {
  model = val;
}
