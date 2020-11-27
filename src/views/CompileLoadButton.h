#include <gtkmm.h>
#include <gtkmm/button.h>

/**
 * @brief The button for compiling and loading a .s file.
 */
class CompileLoadButton : public Gtk::Button {
 public:
  CompileLoadButton(const char* labelText);
  void setAbsolutePathToSelectedFile(std::string val);
  std::string getAbsolutePathToSelectedFile();

 private:
  std::string absolutePathToSelectedFile = "";
  void onClick();
};