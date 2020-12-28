#include "CompileLoadView.h"

CompileLoadView::CompileLoadView(MainWindowView* parent)
    : selectedFileLabel("File: "),
      compileAndLoadButton("Compile & Load"),
      browseButton("Select File"),
      parent(parent) {
  initSelectAndLoadContainer();
}

CompileLoadView::~CompileLoadView() {}

/**
 * @brief Packs children into the selectAndLoadContainer, and sets the
 * layouts and size of it.
 */
void CompileLoadView::initSelectAndLoadContainer() {
  browseButton.set_tooltip_text("Browse for an ARM assembly file (CTRL+L)");
  compileAndLoadButton.set_tooltip_text(
      "Compile and load your file into Jimulator (CTRL+R)");

  // button sizes
  browseButton.set_size_request(100, 33);
  compileAndLoadButton.set_size_request(100, 33);
  selectedFileLabel.set_size_request(100, 33);

  // Sets CSS
  // Adds a CSS class for the compiler buttons
  compileAndLoadButton.get_style_context()->add_class("compButtons");
  browseButton.get_style_context()->add_class("compButtons");

  // Adds a CSS class for the label
  selectedFileLabel.get_style_context()->add_class("fileLabel");

  // Packs in buttons
  this->set_layout(Gtk::BUTTONBOX_END);
  this->pack_end(*getBrowseButton(), false, false);
  this->pack_end(*getSelectedFileLabel(), false, false);
  this->pack_end(*getCompileAndLoadButton(), false, false);
  this->show_all_children();
  this->show();
}

void CompileLoadView::setModel(CompileLoadModel* val) {
  model = val;
}

/**
 * @brief Gets the `compileAndLoadButton` member variable.
 * @return Gtk::Button* A pointer to the `compileAndLoadButton` member
 * variable.
 */
Gtk::Button* CompileLoadView::getCompileAndLoadButton() {
  return &compileAndLoadButton;
}
/**
 * @brief Gets the `browseButton` member variable.
 * @return Gtk::Button* A pointer to the `browseButton` member variable.
 */
Gtk::Button* CompileLoadView::getBrowseButton() {
  return &browseButton;
}
/**
 * @brief Gets the `selectedFileLabel` member variable.
 * @return Gtk::Label* A pointer to the `selectedFileLabel` member variable.
 */
Gtk::Label* CompileLoadView::getSelectedFileLabel() {
  return &selectedFileLabel;
}
/**
 * @brief Sets the text displayed by the `selectedFileLabel` member variable.
 * @param val The text to display in the `selectedFileLabel`.
 */
void CompileLoadView::setSelectedFileLabelText(std::string val) {
  getSelectedFileLabel()->set_text(val);
}