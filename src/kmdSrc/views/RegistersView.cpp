#include "RegistersView.h"
#include <sstream>
#include <string>
#include "../jimulatorInterface.h"

RegistersView::RegistersView(MainWindowView* parent) : grid(), parent(parent) {
  grid.set_column_homogeneous(false);
  grid.set_column_spacing(3);
  grid.set_row_spacing(3);

  // TODO: refactor this

  // Sets up each member of the array
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 18; j++) {
      // Set the left hand labels
      if (i == 0) {
        labelArray[i][j].set_size_request(70, 22);

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

RegistersView::~RegistersView() {}

void RegistersView::setModel(RegistersModel* val) {
  model = val;
}

/**
 * @brief Pads a string representation of a hexadecimal number. It is assumed
 * the hexadecimal number represents a 32-bit integer (or 4-byte), and therefore
 * will be padded to be 8 characters long (1 hex character can encode 4-bits
 * worth of data, 4-bit * 8 = 32-bit)
 * It will also append the hexadecimal number notation ('0x') to the front.
 * @param hex The string representation of a hexidecimal number, which will be
 * padded to the left.
 * @return std::string The padded string.
 */
std::string RegistersView::padHexToEightDigits(std::string hex) {
  int toPad = 8 - hex.length();

  // If somehow is too long
  if (toPad < 0) {
    // TODO: handle graceful failure state
    return "0x🙃FAIL🙃";
  }

  hex = hex.insert(0, std::string(toPad, '0'));
  return hex.insert(0, "0x");
}

/**
 * @brief
 */
void RegistersView::refreshViews() {
  std::stringstream ss;

  for (int i = 0; i < 18; i++) {
    ss << std::hex << getRegisterValueFromJimulator(9);
    labelArray[1][i].set_text(padHexToEightDigits(ss.str()));
  }
}
