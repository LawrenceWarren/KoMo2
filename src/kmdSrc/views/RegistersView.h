#include <gtkmm/buttonbox.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>

class MainWindowView;
class RegistersModel;

class RegistersView : public Gtk::VButtonBox {
 public:
  RegistersView(MainWindowView* parent);
  ~RegistersView();
  void setModel(RegistersModel* val);

 private:
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
