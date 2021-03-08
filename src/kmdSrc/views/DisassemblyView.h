#include <gtkmm/buttonbox.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <gtkmm/togglebutton.h>

class MainWindowView;
class DisassemblyModel;

class DisassemblyView : public Gtk::EventBox {
 public:
  DisassemblyView(MainWindowView* const parent);
  void setModel(DisassemblyModel* const val);

 private:
  /**
   * @brief A pointer to the parent view.
   */
  MainWindowView* parent;

  /**
   * @brief A container for the 6 navigation buttons.
   */
  Gtk::VButtonBox navigationButtons;
  Gtk::Grid disassemblyRows;
  Gtk::HButtonBox container;

  /**
   * @brief A pointer to the related model.
   */
  DisassemblyModel* model;

  void initDisassemblyContainer();

  // ! Deleted special member functions
  // stops these functions from being misused, creates a sensible error
  DisassemblyView(const DisassemblyView&) = delete;
  DisassemblyView(const DisassemblyView&&) = delete;
  DisassemblyView& operator=(const DisassemblyView&) = delete;
  DisassemblyView& operator=(const DisassemblyView&&) = delete;

  class DisassemblyRows : public Gtk::HButtonBox {
   public:
    DisassemblyRows();

   private:
    Gtk::ToggleButton breakpoint;
    Gtk::Label address;
    Gtk::Label hex;
    Gtk::Label disassembly;

    // ! Deleted special member functions
    // stops these functions from being misused, creates a sensible error
    DisassemblyRows(const DisassemblyRows&) = delete;
    DisassemblyRows(const DisassemblyRows&&) = delete;
    DisassemblyRows& operator=(const DisassemblyRows&) = delete;
    DisassemblyRows& operator=(const DisassemblyRows&&) = delete;
  };
};
