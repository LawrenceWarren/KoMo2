#include <gtkmm/buttonbox.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <array>
#include <string>

class MainWindowView;
class RegistersModel;

class RegistersView : public Gtk::VButtonBox {
 public:
  RegistersView(MainWindowView* const parent);
  void refreshViews(const std::array<std::string, 16> a);

  RegistersModel* const getModel() const;
  void setModel(RegistersModel* const val);

 private:
  Gtk::Grid grid;
  std::array<std::array<Gtk::Label, 16>, 2> labelArray;

  /**
   * @brief A pointer to the parent view.
   */
  MainWindowView* parent;

  /**
   * @brief A pointer to the related model.
   */
  RegistersModel* model;

  // ! Deleted special member functions
  // stops these functions from being misused, creates a sensible error
  RegistersView(const RegistersView&) = delete;
  RegistersView(const RegistersView&&) = delete;
  RegistersView& operator=(const RegistersView&) = delete;
  RegistersView& operator=(const RegistersView&&) = delete;
};
