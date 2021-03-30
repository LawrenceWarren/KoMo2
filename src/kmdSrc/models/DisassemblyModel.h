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

  // ! Virtual overrides
  virtual void changeJimulatorState(const JimulatorState newState) override;
  virtual const bool handleKeyPress(const GdkEventKey* const e) override;

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

  const std::string intToFormattedHexString(const uint32_t formatMe) const;
  const bool handleScroll(GdkEventScroll* const e);
  void incrementMemoryIndex(const uint32_t val);
  void addScrollRecognition();
  const std::array<Jimulator::MemoryValues, 15> getMemoryValues() const;
  void onBreakpointToggle(DisassemblyRows* const row) const;
  void setupButtonHandlers();

  // ! Deleted special member functions
  // stops these functions from being misused, creates a sensible error
  DisassemblyModel(const DisassemblyModel&) = delete;
  DisassemblyModel(const DisassemblyModel&&) = delete;
  DisassemblyModel& operator=(const DisassemblyModel&) = delete;
  DisassemblyModel& operator=(const DisassemblyModel&&) = delete;
};
