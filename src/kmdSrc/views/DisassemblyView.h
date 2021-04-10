/**
 * @file DisassemblyView.h
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief This file contains the header file for the DisassemblyRows and
 * DisassemblyView classes.
 * @version 1.0.0
 * @date 10-04-2021
 * @section LICENSE
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details at
 * https://www.gnu.org/copyleft/gpl.html
 */

#include <gtkmm/box.h>
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

/**
 * @brief A single instance of this class represents a single read memory
 * address and the associated data - this is shown as a single row in the
 * disassembly window.
 */
class DisassemblyRows : public Gtk::Box {
 public:
  DisassemblyRows();

  // SET
  void setBreakpoint(const bool text);
  void setAddress(const std::string text);
  void setHex(const std::string text);
  void setDisassembly(const std::string text);
  void setAddressVal(const uint32_t val);
  void setModel(DisassemblyModel* const val);

  // GET
  Gtk::ToggleButton* const getButton();
  const uint32_t getAddressVal() const;
  const bool getBreakpoint();
  const std::string getDisassembly();
  const std::string getAddress() const;

 private:
  /**
   * @brief Setting exact size of widgets is hard; this container is needed
   * to set the size of the button.
   */
  Gtk::HBox buttonSizer;
  /**
   * @brief The debugging breakpoint button.
   */
  Gtk::ToggleButton breakpoint;
  /**
   * @brief Displays the address of the current memory value.
   */
  Gtk::Label address;
  /**
   * @brief Displays the current hexadecimal value of the memory.
   */
  Gtk::Label hex;
  /**
   * @brief Displays the current disassembly value of the memory.
   */
  Gtk::Label disassembly;

  /**
   * @brief The address pointed to, as an integer.
   */
  uint32_t addressVal;

  /**
   * @brief A pointer to the model.
   */
  DisassemblyModel* model;

  void initBreakpoint();
  void initAddress();
  void initHex();
  void initDisassembly();

  // ! Deleted special member functions
  // stops these functions from being misused, creates a sensible error
  DisassemblyRows(const DisassemblyRows&) = delete;
  DisassemblyRows(const DisassemblyRows&&) = delete;
  DisassemblyRows& operator=(const DisassemblyRows&) = delete;
  DisassemblyRows& operator=(const DisassemblyRows&&) = delete;
};

/**
 * @brief Represents the entire disassembly window in the overall KoMo2 GUI.
 * Contains several rows of memory values, buttons for navigation, and buttons
 * for debugging.
 * This class inherits from EventBox, which allows for interception of scroll
 * events, and therefore programmer defined handling of these events. This
 * handling of scroll events is done in the function `handleScroll`.
 */
class DisassemblyView : public Gtk::EventBox {
 public:
  DisassemblyView(MainWindowView* const parent);

  // ! Getters and setters

  void setModel(DisassemblyModel* const val);
  DisassemblyModel* const getModel() const;
  std::vector<DisassemblyRows>* const getRows();
  MainWindowView* const getParent() const;

 private:
  // ! Member children
  /**
   * @brief A pointer to the parent view.
   */
  MainWindowView* const parent;
  /**
   * @brief A pointer to the related model.
   */
  DisassemblyModel* model;

  // ! GUI children
  /**
   * @brief A container for the disassembly rows.
   */
  Gtk::VButtonBox disassemblyContainer;
  /**
   * @brief An array of row views. Each array entry represents a row of memory
   * values within the memory window.
   */
  std::vector<DisassemblyRows> rows{std::vector<DisassemblyRows>(13)};

  // ! Functions

  void initDisassemblyContainer();
  void initDisassemblyRows();

  // ! Deleted special member functions
  // stops these functions from being misused, creates a sensible error
  DisassemblyView(const DisassemblyView&) = delete;
  DisassemblyView(const DisassemblyView&&) = delete;
  DisassemblyView& operator=(const DisassemblyView&) = delete;
  DisassemblyView& operator=(const DisassemblyView&&) = delete;
};
