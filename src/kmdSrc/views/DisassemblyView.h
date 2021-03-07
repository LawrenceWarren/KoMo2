#include <gtkmm/buttonbox.h>
#include <gtkmm/textview.h>  // The main terminal view

class MainWindowView;
class DisassemblyModel;

class DisassemblyView : public Gtk::VButtonBox {
 public:
  DisassemblyView(MainWindowView* const parent);
  void setModel(DisassemblyModel* const val);

 private:
  /**
   * @brief A pointer to the parent view.
   */
  MainWindowView* parent;

  /**
   * @brief A pointer to the related model.
   */
  DisassemblyModel* model;

  // ! Deleted special member functions
  // stops these functions from being misused, creates a sensible error
  DisassemblyView(const DisassemblyView&) = delete;
  DisassemblyView(const DisassemblyView&&) = delete;
  DisassemblyView& operator=(const DisassemblyView&) = delete;
  DisassemblyView& operator=(const DisassemblyView&&) = delete;
};
