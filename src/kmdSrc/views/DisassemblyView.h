#include <gtkmm/buttonbox.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <gtkmm/togglebutton.h>
#include <iostream>
#include <string>
#include <vector>

class MainWindowView;
class DisassemblyModel;

class DisassemblyRows : public Gtk::HButtonBox {
 public:
  DisassemblyRows();
  void setBreakpoint(const bool text);
  void setAddress(const std::string text);
  void setHex(const std::string text);
  void setDisassembly(const std::string text);
  Gtk::ToggleButton breakpoint;
  Gtk::Label address;
  Gtk::Label hex;
  Gtk::Label disassembly;

 private:
  // ! Deleted special member functions
  // stops these functions from being misused, creates a sensible error
  // DisassemblyRows(const DisassemblyRows&) = delete;
  // DisassemblyRows(const DisassemblyRows&&) = delete;
  // DisassemblyRows& operator=(const DisassemblyRows&) = delete;
  // DisassemblyRows& operator=(const DisassemblyRows&&) = delete;
};

class DisassemblyView : public Gtk::EventBox {
 public:
  DisassemblyView(MainWindowView* const parent);
  void setModel(DisassemblyModel* const val);
  DisassemblyModel* const getModel() const;
  Gtk::VButtonBox* const getNavigationButtons();
  Gtk::VButtonBox* const getDisassemblyContainer();
  Gtk::HButtonBox* const getContainer();
  std::vector<DisassemblyRows>* const getRows();
  void packView(const bool emptyChild);

 private:
  /**
   * @brief A pointer to the parent view.
   */
  MainWindowView* parent;

  /**
   * @brief A container for the 6 navigation buttons.
   */
  Gtk::VButtonBox navigationButtons;
  Gtk::VButtonBox disassemblyContainer;
  Gtk::HButtonBox container;
  std::vector<DisassemblyRows> rows{std::vector<DisassemblyRows>(15)};

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
};
