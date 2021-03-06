#include <string>
#include <vector>
#include "TerminalModel.h"

class DisassemblyView;
class DisassemblyRows;

class RowModel {
 private:
  bool breakpoint;
  uint32_t address;
  std::string hex;
  std::string disassembly;

 public:
  RowModel();
  RowModel(const bool breakpoint,
           const uint32_t address,
           const std::string hex,
           const std::string disassembly);
  constexpr const bool getBreakpoint() const;
  const uint32_t getAddress() const;
  const std::string getHex() const;
  const std::string getDisassembly() const;
  void setBreakpoint(const bool toggle);
  void setAddress(const uint32_t text);
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

 private:
  /**
   * @brief The view this model represents.
   */
  DisassemblyView* const view;

  /**
   * @brief Fixed width integer representing the memory address at the tail
   * (top) of the container.
   */
  static uint32_t rowIDTail;

  /**
   * @brief Fixed width integer representing the memory address at the head
   * (bottom) of the container.
   */
  static uint32_t rowIDHead;

  /**
   * @brief A vector of 15 rowModels, representing the 15 views within the
   * container.
   */
  std::vector<RowModel> rowModels{std::vector<RowModel>(15)};

  void adjustListPointers(const uint32_t val);
  const std::string intToFormattedHexString(const uint32_t formatMe) const;
  void reorderViews(const int order);
  void addScrollRecognition();
  void initialiseRowViews();

  // ! Deleted special member functions
  // stops these functions from being misused, creates a sensible error
  DisassemblyModel(const DisassemblyModel&) = delete;
  DisassemblyModel(const DisassemblyModel&&) = delete;
  DisassemblyModel& operator=(const DisassemblyModel&) = delete;
  DisassemblyModel& operator=(const DisassemblyModel&&) = delete;
};
