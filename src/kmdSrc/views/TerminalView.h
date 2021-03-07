#include <gtkmm/buttonbox.h>
#include <gtkmm/textview.h>  // The main terminal view

class MainWindowView;
class TerminalModel;

class TerminalView : public Gtk::VButtonBox {
 public:
  TerminalView(MainWindowView* const parent);
  void setModel(TerminalModel* const val);

 private:
  /**
   * @brief A pointer to the parent view.
   */
  MainWindowView* parent;

  /**
   * @brief A pointer to the related model.
   */
  TerminalModel* model;

  // ! Deleted special member functions
  // stops these functions from being misused, creates a sensible error
  TerminalView(const TerminalView&) = delete;
  TerminalView(const TerminalView&&) = delete;
  TerminalView& operator=(const TerminalView&) = delete;
  TerminalView& operator=(const TerminalView&&) = delete;
};
