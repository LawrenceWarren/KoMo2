#include <string>
#include <vector>
#include "TerminalModel.h"

class DisassemblyView;
class DisassemblyRows;

class RowModel {
 private:
  bool breakpoint;
  std::string address;
  std::string hex;
  std::string disassembly;

 public:
  RowModel();
  RowModel(const bool breakpoint,
           const std::string address,
           const std::string hex,
           const std::string disassembly);
  constexpr const bool getBreakpoint() const;
  const std::string getAddress() const;
  const std::string getHex() const;
  const std::string getDisassembly() const;
  void setBreakpoint(const bool toggle);
  void setAddress(const std::string text);
  void setHex(const std::string text);
  void setDisassembly(const std::string text);
};

class DisassemblyModel : private Model {
 public:
  DisassemblyModel(DisassemblyView* const view, KoMo2Model* const parent);
  virtual void changeJimulatorState(const JimulatorState newState) override;
  virtual const bool handleKeyPress(const GdkEventKey* const e) override;
  DisassemblyView* const getView();
  const bool handleScroll(GdkEventScroll* e);

  static int rowIDTail;
  static int rowIDHead;

 private:
  /**
   * @brief The view this model represents.
   */
  DisassemblyView* const view;
  std::vector<RowModel> rowModels{std::vector<RowModel>(15)};
  std::vector<DisassemblyRows*> rowViews{std::vector<DisassemblyRows*>(15)};

  // ! Deleted special member functions
  // stops these functions from being misused, creates a sensible error
  DisassemblyModel(const DisassemblyModel&) = delete;
  DisassemblyModel(const DisassemblyModel&&) = delete;
  DisassemblyModel& operator=(const DisassemblyModel&) = delete;
  DisassemblyModel& operator=(const DisassemblyModel&&) = delete;
};
