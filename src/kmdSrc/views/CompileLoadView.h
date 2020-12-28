
#include <gtkmm/button.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/label.h>

class MainWindowView;
class CompileLoadModel;

class CompileLoadView : public Gtk::VButtonBox {
 private:
  void initSelectAndLoadContainer();
  MainWindowView* parent;
  CompileLoadModel* model;

  /**
   * @brief A label which displays whatever file has been selected by the
   * browse button, to be compiled & loaded.
   */
  Gtk::Label selectedFileLabel;

  /**
   * @brief A button which, when clicked, will read a .s file, compile it to
   * .kmd, and the load it into Jimulator.
   */
  Gtk::Button compileAndLoadButton;

  /**
   * @brief A button which allows you to browse the file system for .s files.
   */
  Gtk::Button browseButton;

 public:
  void setModel(CompileLoadModel* val);
  CompileLoadView(MainWindowView* parent);
  ~CompileLoadView();
  Gtk::Button* getCompileAndLoadButton();
  Gtk::Button* getBrowseButton();
  Gtk::Label* getSelectedFileLabel();
  void setSelectedFileLabelText(std::string val);
};
