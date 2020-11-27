#include <gtkmm.h>
#include <gtkmm/button.h>

class CompileLoadButton;

/**
 * @brief The button for launching the file browser.
 */
class BrowseButton : public Gtk::Button {
 public:
  BrowseButton(const char* labelText, CompileLoadButton* compileLoad);

 private:
  CompileLoadButton* compileLoadButton;
  void onClick();
  void handleResult(int result, Gtk::FileChooserDialog* dialog);
};
