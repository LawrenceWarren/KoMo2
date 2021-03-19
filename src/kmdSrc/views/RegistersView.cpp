#include "RegistersView.h"
#include <string>

/**
 * @brief Constructs a new RegisterView object.
 * @param parent A pointer to the parent view.
 */
RegistersView::RegistersView(MainWindowView* const parent)
    : grid(), parent(parent) {
  initRegisterViewContainer();
}

void RegistersView::initRegisterViewContainer() {
  grid.set_column_homogeneous(false);
  grid.set_column_spacing(3);
  grid.set_row_spacing(3);

  // Sets up each member of the array
  for (long unsigned int i = 0; i < labelArray.size(); i++) {
    for (long unsigned int j = 0; j < labelArray[i].size(); j++) {
      // Set the left hand labels
      if (i == 0) {
        labelArray[i][j].set_size_request(70, 22);

        // First 0 through 14 are R registers
        if (j != 15) {
          labelArray[i][j].set_text("R" + std::to_string(j));
        }

        // final is the Program Counter
        else {
          labelArray[i][j].set_text("PC");
        }
      }

      // Set the right hand labels
      else {
        labelArray[i][j].set_text("0x00000000");
        labelArray[i][j].set_size_request(120, 22);
        labelArray[i][j].set_xalign(0.1);
      }

      // Give each label styling and attach it to the grid

      labelArray[i][j].set_yalign(1);
      labelArray[i][j].get_style_context()->add_class("tableLabels");
      grid.attach(labelArray[i][j], i, j, 1, 1);
    }
  }

  // Adds and shows everything into the grid
  grid.get_style_context()->add_class("grid");
  pack_start(grid, true, true);
  show_all_children();
}

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
 * @brief Handles updating this particular view.
 * Reads register values from Jimulator, sets the label values of this view
 * to reflect those values.
 */
void RegistersView::refreshViews(const std::array<std::string, 16> a) {
  for (int i = 0; i < 16; i++) {
    labelArray[1][i].set_text(a[i]);
  }
}
