#include <gtkmm/buttonbox.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/textview.h>  // The main terminal view

class MainWindowView;
class TerminalModel;

class TerminalView : public Gtk::VButtonBox {
 public:
  TerminalView(MainWindowView* const parent);
  void setModel(TerminalModel* const val);
  const bool isFocused();
  Glib::RefPtr<Gtk::TextBuffer> getCurrentText();
  Gtk::TextView* const getTextView();

 private:
  Gtk::ScrolledWindow scroll;
  Gtk::TextView textView;

  /**
   * @brief A pointer to the parent view.
   */
  MainWindowView* parent;

  /**
   * @brief A pointer to the related model.
   */
  TerminalModel* model;

  bool handleTextChange();

  // ! Deleted special member functions
  // stops these functions from being misused, creates a sensible error
  TerminalView(const TerminalView&) = delete;
  TerminalView(const TerminalView&&) = delete;
  TerminalView& operator=(const TerminalView&) = delete;
  TerminalView& operator=(const TerminalView&&) = delete;
};
