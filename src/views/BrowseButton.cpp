#include "BrowseButton.h"
#include <iostream>
#include "CompileLoadButton.h"

/**
 * @brief Construct a new BrowseButton object which calls it's superconstructor
 * and set's the `onClick` event.
 * @param labelText The text to display on the button.
 */
BrowseButton::BrowseButton(const char* labelText,
                           CompileLoadButton* compileLoad)
    : Gtk::Button(labelText), compileLoadButton(compileLoad) {
  signal_clicked().connect(sigc::mem_fun(*this, &BrowseButton::onClick));
}

/**
 * @brief Opens a file selection dialog upon the `BrowseButton` being clicked.
 */
void BrowseButton::onClick() {
  // Creates a new file browser dialogue box.
  Gtk::FileChooserDialog dialog("Please choose a file",
                                Gtk::FILE_CHOOSER_ACTION_OPEN);

  // Gets the parent of the dialogue box.
  Gtk::Window* parent = dynamic_cast<Gtk::Window*>(this->get_toplevel());
  dialog.set_transient_for(*parent);

  // Add response buttons the the dialog:
  dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dialog.add_button("_Open", Gtk::RESPONSE_OK);

  // Creates a filter for what type of files can be selected.
  auto assemblyFilter = Gtk::FileFilter::create();
  assemblyFilter->set_name("ARM assembly files");
  assemblyFilter->add_pattern("*.s");
  dialog.add_filter(assemblyFilter);

  // Show the dialog and wait for a user response, then handle the result.
  handleResult(dialog.run(), &dialog);
}

/**
 * @brief Handles the result of the file browser dialog box being closed.
 * @param result The result of the file browser closing (i.e. was a file
 * selected, was the dialog box cancelled, did something unexpected happen,
 * etc.)
 * @param dialog A pointer to the dialog box itself - frees itself in its
 * destructor.
 */
void BrowseButton::handleResult(int result, Gtk::FileChooserDialog* dialog) {
  switch (result) {
    case (Gtk::RESPONSE_OK): {
      compileLoadButton->setAbsolutePathToSelectedFile(dialog->get_filename());
      break;
    }
    case (Gtk::RESPONSE_CANCEL): {
      // Cancel behaviour
      break;
    }
    default: {
      // Some unexpected behaviour
      break;
    }
  }
}
