#include <gtkmm.h>
#include <gtkmm/button.h>

class BrowseButton : public Gtk::Button {
 public:
  BrowseButton(const char* labelText);
  void createFileSelector();
  std::string absolutePathToSelectedFile = NULL;

 private:
  void onClick();
  void handleResult(int result, Gtk::FileChooserDialog* dialog);
};
