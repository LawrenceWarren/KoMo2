/**
 * @file CompileLoadButton.h
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief A forward declaration of the `CompileLoadButton` class, which inherits
 * from `GTK::Button`.
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

/**
 * @brief The button for compiling and loading a .s file.
 */
class CompileLoadButton : public Gtk::Button {
 public:
  CompileLoadButton(const char* labelText, std::string absolutePathCalledFrom);
  void setAbsolutePathToSelectedFile(std::string val);
  std::string getAbsolutePathToSelectedFile();

 private:
  std::string absolutePathCalledFrom;
  std::string absolutePathToSelectedFile = "";
  std::string makeKmdPath(std::string absolutePath);
  void onClick();
};