/**
 * @file MainWindowView.h
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief A class declaration for the `MainWindowView` class, which inherits
 * from `GTK::Window`.
 * @version 1.0.0
 * @date 10-04-2021
 */

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/window.h>
#include <string>
#include "CompileLoadView.h"
#include "ControlsView.h"
#include "DisassemblyView.h"
#include "RegistersView.h"
#include "TerminalView.h"

class KoMo2Model;

/**
 * @brief The class definition of the main window of the program. This main
 * window is the mater view of the program. Within this class are all of the
 * child views, controllers, and a pointer to the master model, as keeping
 * with MVC.
 */
class MainWindowView : public Gtk::Window {
 public:
  // Constructors
  MainWindowView(const int x, const int y);

  // Getters and setters
  void setModel(KoMo2Model* const val);
  KoMo2Model* const getModel() const;

  // Get components
  CompileLoadView* const getCompileLoadView();
  ControlsView* const getControlsView();
  RegistersView* const getRegistersView();
  TerminalView* const getTerminalView();
  DisassemblyView* const getDisassemblyView();
  void setStyling();

 private:
  // Get layouts
  Gtk::HButtonBox* const getControlsAndCompileBar();
  Gtk::HButtonBox* const getRegistersAndDisassemblyBar();
  Gtk::Box* const getMasterLayout();

  void initControlsAndCompileBar();
  void initRegistersAndDisassemblyBar();
  void initMasterLayout();

  // ! Layouts

  /**
   * @brief The master layout - every other view or layout should be nested
   * within this layout.
   */
  Gtk::Box masterLayout;
  /**
   * @brief The layout for the top bar running along the screen. Contains the
   * compile and load layout, and the program controls layout.
   */
  Gtk::HButtonBox controlsAndCompileBar;
  /**
   * @brief The container that contains the source/disassembly view and the
   * registers view.
   */
  Gtk::HButtonBox registersAndDisassemblyBar;

  // ! Components

  /**
   * @brief A box containing the browse button, the compile and load button, and
   * the selected file label.
   */
  CompileLoadView compileLoadView;
  /**
   * @brief A box containing all of the programs running controls.
   */
  ControlsView controlsView;
  /**
   * @brief A box containing all of the registers
   */
  RegistersView registersView;
  /**
   * @brief A box contaning the input/output terminal.
   */
  TerminalView terminalView;
  /**
   * @brief A box containing the disassembly and source views.
   */
  DisassemblyView disassemblyView;

  // ! Other

  /**
   * @brief A pointer to the main model of the program.
   */
  KoMo2Model* model;

  // ! Deleted special member functions
  // stops these functions from being misused, creates a sensible error
  MainWindowView(const MainWindowView&) = delete;
  MainWindowView(const MainWindowView&&) = delete;
  MainWindowView& operator=(const MainWindowView&) = delete;
  MainWindowView& operator=(const MainWindowView&&) = delete;
};
