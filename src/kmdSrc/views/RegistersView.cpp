#include "RegistersView.h"
#include <string>

RegistersView::RegistersView(MainWindowView* parent) : grid(), parent(parent) {
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 18; j++) {
      // Set the left hand labels
      if (i == 0) {
        if (j < 15) {
          labelArray[i][j].set_text("R" + std::to_string(j));
        } else if (j == 15) {
          labelArray[i][j].set_text("PC");
        } else if (j == 16) {
          labelArray[i][j].set_text("CPSR");
        } else {
          labelArray[i][j].set_text("SPSR");
        }
      } else {
        labelArray[i][j].set_text("DEMO");
      }

      grid.attach(labelArray[i][j], i, j, 1, 1);
    }
  }

  grid.get_style_context()->add_class("grid");

  pack_start(grid);
  show_all_children();
}

RegistersView::~RegistersView() {}

void RegistersView::setModel(RegistersModel* val) {
  model = val;
}
