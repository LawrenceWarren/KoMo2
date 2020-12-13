/**
 * @file BrowseButton.h
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief A forward declaration of the `BrowseButton` class.
 * @version 0.1
 * @date 2020-11-27
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * https://www.gnu.org/copyleft/gpl.html
 *
 * @copyright Copyright (c) 2020
 *
 */

#include <gtkmm.h>
#include <gtkmm/button.h>

class CompileLoadButton;

/**
 * @brief The button for launching the file browser.
 */
class BrowseButton : public Gtk::Button {
 public:
  BrowseButton(const char* labelText, CompileLoadButton* compileLoad);

 private:
  CompileLoadButton* compileLoadButton;
  void onClick();
  void handleResult(int result, Gtk::FileChooserDialog* dialog);
};
