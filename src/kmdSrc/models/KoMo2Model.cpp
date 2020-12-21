#include "KoMo2Model.h"
#include "../views/MainWindow.h"

/**
 * @brief Construct a new KoMo2Model - this constructor initialises the
 * mainWindow pointer, as well as the absolutePathToProjectRoot member. It then
 * constructs a member compileLoadModel, and sets the compile buttons on click
 * events.
 * @param mainWindow A pointer to the mainWindow view object.
 * @param argv0 The absolutePathToProjectRoot - parsed from argv[0].
 */
KoMo2Model::KoMo2Model(MainWindow* mainWindow, std::string argv0)
    : mainWindow(mainWindow),
      absolutePathToProjectRoot(argv0),
      compileLoadModel(mainWindow->getCompileAndLoadButton(),
                       mainWindow->getBrowseButton(),
                       this) {
  getMainWindow()->setModel(this);
  getMainWindow()->setCSS();

  getMainWindow()->getBrowseButton()->signal_clicked().connect(
      sigc::mem_fun(*getCompileLoadModel(), &CompileLoadModel::onBrowseClick));

  getMainWindow()->getCompileAndLoadButton()->signal_clicked().connect(
      sigc::mem_fun(*getCompileLoadModel(),
                    &CompileLoadModel::onCompileLoadClick));
}

KoMo2Model::~KoMo2Model() {}

// ! Getter functions

MainWindow* KoMo2Model::getMainWindow() {
  return mainWindow;
}
const std::string KoMo2Model::getAbsolutePathToProjectRoot() {
  return absolutePathToProjectRoot;
}
CompileLoadModel* KoMo2Model::getCompileLoadModel() {
  return &compileLoadModel;
}
