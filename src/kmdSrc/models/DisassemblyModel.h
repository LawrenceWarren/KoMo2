/**
 * @file DisassemblyModel.h
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief This file declares the class DisassemblyModel, which represents the
 * logical data in memory that relates to the memory window you see in the KoMo2
 * GUI. The view is represented in the file `DisassemblyView.cpp` and it's
 * header.
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

#include <string>
#include <vector>
#include "TerminalModel.h"

class DisassemblyView;
class DisassemblyRows;

/**
 * @brief The declaration of the DisassemblyModel class.
 */
class DisassemblyModel : private Model {
 public:
  DisassemblyModel(DisassemblyView* const view, KoMo2Model* const parent);
  void refreshViews();
  DisassemblyView* const getView();
  void setPCValue(const std::string val);

  // ! Virtual overrides
  virtual void changeJimulatorState(const JimulatorState newState) override;
  virtual const bool handleKeyPress(const GdkEventKey* const e) override;
  void setEnglishMnemonic(const bool val);

 private:
  /**
   * @brief The view this model represents.
   */
  DisassemblyView* const view;
  /**
   * @brief Fixed width integer representing the memory address of the view at
   * the top of the container.
   */
  static uint32_t memoryIndex;

  /**
   * @brief Stores the value currently in the program counter.
   */
  std::string PCValue = "0x00000000";

  /**
   * @brief The CSS state flags for an un-highlighted memory row.
   */
  const Gtk::StateFlags NORMAL = Gtk::STATE_FLAG_DIR_LTR;

  /**
   * @brief The CSS state flags for if the memory row is currently stored in the
   * Program Counter.
   */
  const Gtk::StateFlags PC_ADDRESS =
      Gtk::STATE_FLAG_DIR_LTR | Gtk::STATE_FLAG_ACTIVE;

  /**
   * @brief The CSS state flags for a memory row that has keyboard focus.
   */
  const Gtk::StateFlags FOCUSED =
      Gtk::STATE_FLAG_DIR_LTR | Gtk::STATE_FLAG_FOCUSED;

  bool englishMnemonic = false;

  /**
   * @brief The CSS state flags for e memory row that has keyboard focus and
   * is currently stored in the Program Counter.
   */
  const Gtk::StateFlags PC_ADDRESS_FOCUSED = Gtk::STATE_FLAG_DIR_LTR |
                                             Gtk::STATE_FLAG_FOCUSED |
                                             Gtk::STATE_FLAG_ACTIVE;

  const std::string intToFormattedHexString(const uint32_t formatMe) const;
  const bool handleScroll(GdkEventScroll* const e);
  void incrementMemoryIndex(const uint32_t val);
  void addScrollRecognition();
  const std::array<Jimulator::MemoryValues, 15> getMemoryValues() const;
  void onBreakpointToggle(DisassemblyRows* const row);
  void setupButtonHandlers();
  void updateCSSFlags(const Gtk::StateFlags state,
                      DisassemblyRows& row,
                      const uint32_t address);
  const std::string buildDisassemblyRowAccessibilityString(
      DisassemblyRows& val);
  const std::string convertMnemonicToEnglish(const std::string mnemonic) const;
  const std::string parseDEFB(const std::vector<std::string> v) const;
  const std::string parse1Param(std::vector<std::string> v) const;
  const std::string parse2Param(std::vector<std::string> v) const;
  const std::string parse3Param(std::vector<std::string> v) const;
  const std::string parse4Param(std::vector<std::string> v) const;
  const std::string sanitizeParamters(std::string param) const;

  // ! Deleted special member functions
  // stops these functions from being misused, creates a sensible error
  DisassemblyModel(const DisassemblyModel&) = delete;
  DisassemblyModel(const DisassemblyModel&&) = delete;
  DisassemblyModel& operator=(const DisassemblyModel&) = delete;
  DisassemblyModel& operator=(const DisassemblyModel&&) = delete;
};
