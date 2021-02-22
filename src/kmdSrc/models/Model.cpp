/**
 * @file Model.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief Definitions of the functions and variables found in the class
 * declaration in `Model.h`.
 * @version 0.1
 * @date 2020-12-23
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

#include "Model.h"
#include <gtkmm/button.h>
#include <gtkmm/image.h>
#include <string>

// Sets the static jimulatorState to its initial value
JimulatorState Model::jimulatorState = INITIAL;

/**
 * @brief Constructs a new Model object - just assigns the parent variable.
 * @param parent The parent pointer.
 */
Model::Model(KoMo2Model* const parent) : parent(parent) {}

/**
 * @brief Sets the state of a button to some boolean - the assumption is that if
 * a button is not meant to be sensitive (`get_sensitive() == false`) then it
 * should also not have a tooltip or many other attributes.
 * You can also optionally update the buttons image, tooltip text, and label
 * text.
 * @param button The button to set the attributes of.
 * @param state What value to set the attributes to - specifically, if the
 * button should be sensitive or display a tooltip.
 * @param newTooltip The new tooltip text to display.
 * @param img The new image to display.
 * @param newLabelText The new label text to display.
 */
void Model::setButtonState(Gtk::Button* const button,
                           const bool state,
                           Gtk::Image* const img,
                           const std::string newTooltip,
                           const std::string newLabelText) const {
  button->set_sensitive(state);
  button->set_has_tooltip(state);

  // If newToolTip isn't the default parameter, update the tooltip text
  if (newTooltip != "") {
    button->set_tooltip_text(newTooltip);
  }

  // If img isn't the default parameter, update the button image
  if (img != nullptr) {
    button->set_image(*img);
  }

  // If newLabelText isn't the default parameter, update the button label
  if (newLabelText != "") {
    button->set_label(newLabelText);
  }
}

// ! Getters and setters

/**
 * @brief Returns the parent pointer.
 * @return KoMo2Model* The parent pointer.
 */
KoMo2Model* const Model::getParent() const {
  return parent;
}

/**
 * @brief Return the jimulatorState member object.
 * @return JimulatorState the jimulatorState member.
 */
JimulatorState Model::getJimulatorState() const {
  return Model::jimulatorState;
}

/**
 * @brief set the JimulatorState member object.
 * @param val The value to set JimulatorState to.
 */
void Model::setJimulatorState(const JimulatorState val) {
  Model::jimulatorState = val;
}
