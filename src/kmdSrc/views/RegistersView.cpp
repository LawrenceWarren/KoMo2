/**
 * @file RegistersView.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief This file defines the class RegistersView, which represents the
 * viewable GUI element that displays information about the memory within
 * Jimulator. The model is represented in the file `RegistersModel.cpp` and it's
 * associated header.
 * @version 0.1
 * @date 2021-03-18
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

#include "RegistersView.h"
#include <iostream>
#include <string>
#include "../models/KoMo2Model.h"

/**
 * @brief Constructs a new RegisterView object.
 * @param parent A pointer to the parent view.
 */
RegistersView::RegistersView(MainWindowView* const parent)
    : grid(), parent(parent) {
  initRegisterViewContainer();
}

/**
 * @brief Initialises the register view.
 */
void RegistersView::initRegisterViewContainer() {
  // Sets up each member of the array
  for (long unsigned int i = 0; i < labelArray.size(); i++) {
    for (long unsigned int j = 0; j < labelArray[i].size(); j++) {
      if (i == 0) {
        initLeftHandLabel(j);
      } else {
        initRightHandLabel(j);
      }

      initAllLabels(i, j);
    }
  }

  initGrid();
}

/**
 * @brief Initialises the left hand labels within the array.
 * @param j The index of the inner label array.
 */
void RegistersView::initLeftHandLabel(const int j) {
  labelArray[0][j].set_size_request(70, 22);

  // Get some text for the left hand label
  const auto s = j != 15 ? "R" + std::to_string(j) : "PC";
  labelArray[0][j].set_text(s);
  labelArray[0][j].get_accessible()->set_name(s);
}

/**
 * @brief Initialises the right hand labels within the array.
 * @param j The index of the inner label array.
 */
void RegistersView::initRightHandLabel(const int j) {
  labelArray[1][j].set_size_request(120, 22);
  labelArray[1][j].set_xalign(0.1);
}

/**
 * @brief Initialisations that have to happen for both sides of the table.
 * @param i The index of the outer label array.
 * @param j The index of the inner label array.
 */
void RegistersView::initAllLabels(const int i, const int j) {
  labelArray[i][j].set_yalign(1);
  labelArray[i][j].get_style_context()->add_class("tableLabels");
  grid.attach(labelArray[i][j], i, j, 1, 1);
}

/**
 * @brief Initialisation instructions for the grid.
 */
void RegistersView::initGrid() {
  grid.set_column_homogeneous(false);
  grid.set_column_spacing(3);
  grid.set_row_spacing(3);
  grid.get_style_context()->add_class("registerTable");
  pack_start(grid, true, true);
  show_all_children();
}

// ! Getters and setters

/**
 * @brief Sets the model for this view.
 * @param val The model.
 */
void RegistersView::setModel(RegistersModel* const val) {
  model = val;
}
/**
 * @brief gets the model for this view.
 * @return RegistersModel* The model.
 */
RegistersModel* const RegistersView::getModel() const {
  return model;
}
/**
 * @brief Return a reference to the member array of labels, `labelArray`.
 * @return std::array<std::array<Gtk::Label, 16>, 2>* const A constant pointer
 * the member `labelArray`.
 */
std::array<std::array<Gtk::Label, 16>, 2>* const RegistersView::getLabels() {
  return &labelArray;
}
