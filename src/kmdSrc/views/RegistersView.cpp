#include "RegistersView.h"
#include <string>

RegistersView::RegistersView(MainWindowView* parent) : grid(), parent(parent) {
  grid.set_column_homogeneous(false);
  grid.set_column_spacing(3);
  grid.set_row_spacing(3);

  // Sets up each member of the array
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 18; j++) {
      // Set the left hand labels
      if (i == 0) {
        labelArray[i][j].set_size_request(70, 25);

        // First 15 are R registers
        if (j < 15) {
          labelArray[i][j].set_text("R" + std::to_string(j));
        }

        // 16th is the Program Counter
        else if (j == 15) {
          labelArray[i][j].set_text("PC");
        }

        // 17th is CPSR
        else if (j == 16) {
          labelArray[i][j].set_text("CPSR");
        }

        // 18th is SPSR
        else {
          labelArray[i][j].set_text("SPSR");
        }
      }

      // Right hand column
      else {
        labelArray[i][j].set_text("0xFEDCBA98");
        labelArray[i][j].set_size_request(120, 25);
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

RegistersView::~RegistersView() {}

void RegistersView::setModel(RegistersModel* val) {
  model = val;
}
