#include <gtkmm/buttonbox.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <string>

class MainWindowView;
class RegistersModel;

class RegistersView : public Gtk::VButtonBox {
 public:
  RegistersView(MainWindowView* parent);
  void setModel(RegistersModel* val);
  void refreshViews();

 private:
  Gtk::Grid grid;
  Gtk::Label labelArray[2][16];
  char* something(int count, unsigned char* values);

  /**
   * @brief A pointer to the parent view.
   */
  MainWindowView* parent;

  /**
   * @brief A pointer to the related model.
   */
  RegistersModel* model;

  //   // ! Deleted special member functions
  // stops these functions from being misused, creates a sensible error
  RegistersView(const RegistersView&) = delete;
  RegistersView(const RegistersView&&) = delete;
  RegistersView& operator=(const RegistersView&) = delete;
  RegistersView& operator=(const RegistersView&&) = delete;
};
