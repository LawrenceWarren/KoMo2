#include <gtkmm/buttonbox.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <string>

class MainWindowView;
class RegistersModel;

class RegistersView : public Gtk::VButtonBox {
 public:
  RegistersView(MainWindowView* parent);
  ~RegistersView();
  void setModel(RegistersModel* val);
  void refreshViews();

 private:
  std::string padHex(std::string hex);
  Gtk::Grid grid;
  Gtk::Label labelArray[2][18];

  /**
   * @brief A pointer to the parent view.
   */
  MainWindowView* parent;

  /**
   * @brief A pointer to the related model.
   */
  RegistersModel* model;
};
