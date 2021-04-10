/**
 * @file TerminalView.h
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief Declares the TerminalView class, which is an element of the KoMo2 GUI
 * which represents the input & output text boxes at the bottom of the screen.
 * Any data, and functions related to the manipulation of data, can be found in
 * the accompanying TerminalModel class.
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

#include <gtkmm/button.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/entry.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/textview.h>

class MainWindowView;
class TerminalModel;

/**
 * @brief The GUI element which encapsulates the "clear" button, the output text
 * box, and the input text box at the bottom of the KoMo2 GUI.
 */
class TerminalView : public Gtk::VButtonBox {
 public:
  TerminalView(MainWindowView* const parent);
  void setModel(TerminalModel* const val);

  // ! Getters amd setters

  Gtk::TextView* const getTextView();
  Gtk::Button* const getClearButton();
  Gtk::Entry* const getInputBox();

 private:
  /**
   * @brief The button which clears the terminal when clicked.
   */
  Gtk::Button clearButton;

  /**
   * @brief A container class for the `textView` member, which allows the text
   * box to be scrolled.
   */
  Gtk::ScrolledWindow scroll;

  /**
   * @brief A TextView which displays any output from Jimulator and KoMo2.
   */
  Gtk::TextView textView;

  /**
   * @brief An input box which directly pipes information into Jimulator.
   */
  Gtk::Entry inputBox;

  /**
   * @brief A layout which allows for the inputBox and clearButton to be placed
   * side-by-side.
   */
  Gtk::HButtonBox layout;

  /**
   * @brief A pointer to the parent view.
   */
  MainWindowView* parent;

  /**
   * @brief A pointer to the related model.
   */
  TerminalModel* model;

  void initClearButton();
  void initTextView();
  void initInputBox();
  void initScrollView();
  void packChildren();

  // ! Deleted special member functions
  // stops these functions from being misused, creates a sensible error
  TerminalView(const TerminalView&) = delete;
  TerminalView(const TerminalView&&) = delete;
  TerminalView& operator=(const TerminalView&) = delete;
  TerminalView& operator=(const TerminalView&&) = delete;
};
