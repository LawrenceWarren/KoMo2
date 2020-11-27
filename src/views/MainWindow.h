#include <gtkmm/window.h>
#include "BrowseButton.h"

class MainWindow : public Gtk::Window {
 public:
  MainWindow();
  virtual ~MainWindow();

 protected:
  BrowseButton fileSelector;
};
