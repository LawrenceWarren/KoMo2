#include <gtkmm/buttonbox.h>
#include <gtkmm/window.h>
#include "BrowseButton.h"
#include "CompileLoadButton.h"

class MainWindow : public Gtk::Window {
 public:
  MainWindow();
  virtual ~MainWindow();

  // Child widgets & layouts
 protected:
  Gtk::ButtonBox selectAndLoadContainer;
  CompileLoadButton compileAndLoad;
  BrowseButton fileSelector;
};
