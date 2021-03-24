#include "RegistersModel.h"

class TerminalView;

class TerminalModel : private Model {
 public:
  TerminalModel(TerminalView* const view, KoMo2Model* const parent);
  virtual void changeJimulatorState(const JimulatorState newState) override;
  virtual const bool handleKeyPress(const GdkEventKey* const e) override;
  TerminalView* const getView();
  void appendTextToTextView(std::string text);
  const std::string readJimulator();

 private:
  /**
   * @brief The view this model represents.
   */
  TerminalView* const view;

  void onClearClick();

  // ! Deleted special member functions
  // stops these functions from being misused, creates a sensible error
  TerminalModel(const TerminalModel&) = delete;
  TerminalModel(const TerminalModel&&) = delete;
  TerminalModel& operator=(const TerminalModel&) = delete;
  TerminalModel& operator=(const TerminalModel&&) = delete;
};
