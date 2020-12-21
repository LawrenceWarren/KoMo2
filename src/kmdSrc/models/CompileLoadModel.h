/**
 * @file compileLoadController.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2020-12-21
 * @copyright Copyright (c) 2020
 */

#include <gtkmm/button.h>
#include <gtkmm/filechooserdialog.h>
#include <string>

class KoMo2Model;

class CompileLoadModel {
 public:
  // Constructors
  CompileLoadModel(Gtk::Button* compileLoadButton,
                   Gtk::Button* browseButton,
                   KoMo2Model* parent);
  ~CompileLoadModel();

  // Click handlers
  void onCompileLoadClick();
  void onBrowseClick();

  // Getters and setters
  void setAbsolutePathToSelectedFile(std::string val);
  std::string getAbsolutePathToSelectedFile();
  KoMo2Model* getParent();

 private:
  // The path to selected file
  std::string absolutePathToSelectedFile;
  // A pointer to the parent
  KoMo2Model* parent;
  // Button pointers
  Gtk::Button* compileLoadButton;
  Gtk::Button* browseButton;

  // General functions
  std::string makeKmdPath(std::string absolutePath);
  void handleResult(int result, Gtk::FileChooserDialog* dialog);
};
