#include <gtkmm/buttonbox.h>
#include <gtkmm/entry.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/textview.h>  // The main terminal view

class MainWindowView;
class TerminalModel;

class TerminalView : public Gtk::VButtonBox {
 public:
  TerminalView(MainWindowView* const parent);
  void setModel(TerminalModel* const val);
  const bool isFocused();
  const std::string getCurrentText();
  Gtk::TextView* const getTextView();
  void clearInputBox();

 private:
  Gtk::ScrolledWindow scroll;
  Gtk::TextView textView;
  Gtk::Entry inputBox;

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
