#include "CompileLoadModel.h"

class MainWindow;

class KoMo2Model {
 private:
  // members
  MainWindow* mainWindow;  // Main window view
  const std::string
      absolutePathToProjectRoot;      // absolute Path ./kmd was executed
  CompileLoadModel compileLoadModel;  // The model for compiling and loading

 public:
  // Constructors
  KoMo2Model(MainWindow* mainWindow, std::string argv0);
  ~KoMo2Model();

  // Getters
  CompileLoadModel* getCompileLoadModel();
  MainWindow* getMainWindow();
  const std::string getAbsolutePathToProjectRoot();
};
