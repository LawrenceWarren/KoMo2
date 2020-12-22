#include <gtkmm.h>
#include <iostream>
#include <string>
#include "KoMo2Model.h"

ControlsModel::ControlsModel(Gtk::Button* helpButton,
                             Gtk::Button* beginRunJimulatorButton,
                             Gtk::Button* reloadJimulatorButton,
                             Gtk::Button* pauseResumeButton,
                             Gtk::Button* singleStepExecuteButton,
                             Gtk::Button* haltExecutionButton,
                             KoMo2Model* parent)
    : helpButton(helpButton),
      beginRunJimulatorButton(beginRunJimulatorButton),
      reloadJimulatorButton(reloadJimulatorButton),
      pauseResumeButton(pauseResumeButton),
      singleStepExecuteButton(singleStepExecuteButton),
      haltExecutionButton(haltExecutionButton),
      parent(parent) {}

ControlsModel::~ControlsModel() {}

void ControlsModel::onHelpClick() {
  std::cout << "Help Button click!" << std::endl;
}

void ControlsModel::onBeginRunJimulatorClick() {
  std::cout << "begin Run Jimulator Button Click!" << std::endl;
}

void ControlsModel::onReloadJimulatorClick() {
  std::cout << "Reload Jimulator Button Click!" << std::endl;
}

void ControlsModel::onPauseResumeClick() {
  std::cout << "pause/Resume Button Click!" << std::endl;
}

void ControlsModel::onSingleStepExecuteClick() {
  std::cout << "Single Step execution Button Click!" << std::endl;
}

void ControlsModel::onHaltExecutionClick() {
  std::cout << "Halt Execution Button Click!" << std::endl;
}
