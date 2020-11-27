#include <gtkmm/application.h>
#include "views/MainWindow.h"

int main(int argc, char* argv[]) {
  auto app = Gtk::Application::create(argc, argv, "uon.komodo");
  MainWindow KoMo2;
  return app->run(KoMo2);
}
