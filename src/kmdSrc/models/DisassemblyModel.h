/**
 * @file DisassemblyModel.h
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief This file declares the class DisassemblyModel, which represents the
 * logical data in memory that relates to the memory window you see in the KoMo2
 * GUI. The view is represented in the file `DisassemblyView.cpp` and it's
 * header.
 * @version 1.0.0
 * @date 10-04-2021
 */

#include <string>
#include <unordered_map>
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

  /**
   * @brief The CSS state flags for e memory row that has keyboard focus and
   * is currently stored in the Program Counter.
   */
  const Gtk::StateFlags PC_ADDRESS_FOCUSED = Gtk::STATE_FLAG_DIR_LTR |
                                             Gtk::STATE_FLAG_FOCUSED |
                                             Gtk::STATE_FLAG_ACTIVE;

  /**
   * @brief A map pairing ARM mnemonic commands with the English translation
   * string associated with them for the screenreader.
   * For ARM commands that take paramters, the English equivalent string has a
   * paramter notation that is regexed away. This notation is:
   * ?'paramter_number'?.
   *
   * For example, in the command "ADD R1, R2, R3", R1 is paramter 1, R2 is
   * paramter 2, R3 is paramter 3. So the associated English string "Add ?3? to
   * ?2? and store in ?1?" swaps ?1? with R1, ?2? with R2, and ?3? with R3, to
   * result in the string "Add R3 to R2 and store in R1."
   */
  const std::unordered_map<std::string, std::string> mnemonicsMap = {
      // SWI
      {"swi 0", "Printing character"},
      {"swi 1", "Reading character"},
      {"swi 2", "Halting execution"},
      {"swi 3", "Printing string"},
      {"swi 4", "Printing integer"},
      // 1 paramter
      {"defw", "defined as integer ?1?"},
      {"defb", "Defined as string ?1?"},
      {"beq", "Branch to label ?1? if equal"},
      {"blt", "Branch to label ?1? if less than"},
      {"bne", "Branch to label ?1? if not equal"},
      {"bgt", "Branch to label ?1? if greater than"},
      {"b", "Branch to label ?1?"},
      // 2 paramters
      {"mov", "Move ?2? into ?1?"},
      {"adr", "Value at ?2? moves into ?1?"},
      {"adrl", "Value at ?2? moves into ?1?"},
      {"cmp", "Compare ?1? to ?2?"},
      {"cmn", "Negatively compare ?1? to ?2?"},
      {"str", "Store ?1? in ?2?"},
      {"ldr", "Stores ?2? in ?1?"},
      // 3 paramters
      {"sub", "Subtract ?3? from ?2? and store in ?1?"},
      {"add", "Add ?3? to ?2? and store in ?1?"},
      {"mul", "Multiply ?3? with ?2? and store in ?1?"},
      {"and", "bitwise and ?2? with ?3? and store in ?1?"},
      {"orr", "bitwise or ?2? with ?3? and store in ?1?"},
      // 4 paramters
      {"mla", "Multiply ?2? with ?3? , add ?4? and store in ?1?"},
      {"mls", "Multiply ?2? with ?3? , subtract ?4? and store in ?1?"}};

  /**
   * @brief Whether or not ARM mnemonics should be read in English when being
   * read by a screenreader, or if they should be left as ARM mnemonics.
   */
  bool englishMnemonic = false;

  const std::string intToFormattedHexString(const uint32_t formatMe) const;
  const bool handleScroll(GdkEventScroll* const e);
  void incrementMemoryIndex(const uint32_t val);
  void addScrollRecognition();
  const std::array<Jimulator::MemoryValues, 13> getMemoryValues() const;
  void onBreakpointToggle(DisassemblyRows* const row);
  void setupButtonHandlers();
  void updateCSSFlags(const Gtk::StateFlags state,
                      DisassemblyRows& row,
                      const uint32_t address);
  const std::string buildDisassemblyRowAccessibilityString(
      DisassemblyRows& val);
  const std::string convertMnemonicToEnglish(const std::string mnemonic) const;
  const std::string sanitizeParamters(std::string param) const;
  const std::string toLowerCase(std::string s) const;
  const std::vector<std::string> parseDEFB(std::vector<std::string> v) const;
  const std::vector<std::string> parseSWI(std::vector<std::string> m) const;
  const std::pair<std::vector<std::string>, std::string> parseLabel(
      std::vector<std::string> m) const;
  const std::string buildMnemonicString(std::string s,
                                        std::vector<std::string> m) const;

  // ! Deleted special member functions
  // stops these functions from being misused, creates a sensible error
  DisassemblyModel(const DisassemblyModel&) = delete;
  DisassemblyModel(const DisassemblyModel&&) = delete;
  DisassemblyModel& operator=(const DisassemblyModel&) = delete;
  DisassemblyModel& operator=(const DisassemblyModel&&) = delete;
};
