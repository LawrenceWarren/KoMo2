#include "ControlsView.h"
#include <gtkmm/button.h>
#include <gtkmm/image.h>

ControlsView::ControlsView(MainWindowView* parent)
    : parent(parent),
      helpButton(),
      reloadJimulatorButton(),
      pauseResumeButton(),
      singleStepExecuteButton(),
      haltExecutionButton() {
  initProgramControlsContainer();
}

ControlsView::~ControlsView() {}

/**
 * @brief Packs children into the programControlsContainer, and sets the layouts
 * and size of it. Initialises the look images of buttons.
 */
void ControlsView::initProgramControlsContainer() {
  // Set halt button image
  haltExecutionButton.set_image_position(Gtk::POS_LEFT);
  haltExecutionButton.set_image(*new Gtk::Image("res/haltSymbol.png"));
  haltExecutionButton.set_tooltip_text("Halt Jimulator (F1)");

  // Set help button image
  helpButton.set_image_position(Gtk::POS_LEFT);
  helpButton.set_image(*new Gtk::Image("res/helpSymbol.png"));
  helpButton.set_tooltip_text("About KoMo2 (F12)");
  // TODO: does this accessibility work?
  helpButton.get_accessible()->set_name("Help Button");
  helpButton.get_accessible()->set_description(
      "This button will display a help window.");

  // TODO: help button is focused by default. Stop that
  // TODO: the focus outline shows up NO MATTER WHAT. stop that

  // Set the single step execution button image
  singleStepExecuteButton.set_image_position(Gtk::POS_LEFT);
  singleStepExecuteButton.set_image(
      *new Gtk::Image("res/singleStepSymbol.png"));
  singleStepExecuteButton.set_tooltip_text("Execute 1 instruction (F6)");

  // Set the reload button image
  reloadJimulatorButton.set_image_position(Gtk::POS_LEFT);
  reloadJimulatorButton.set_image(*new Gtk::Image("res/refreshSymbol.png"));
  reloadJimulatorButton.set_tooltip_text("Reload program (Ctrl+R)");

  // Set sizes
  helpButton.set_size_request(40, 40);
  reloadJimulatorButton.set_size_request(40, 40);
  pauseResumeButton.set_size_request(40, 40);
  singleStepExecuteButton.set_size_request(40, 40);
  haltExecutionButton.set_size_request(40, 40);

  // Adds a CSS class for the program running buttons
  helpButton.get_style_context()->add_class("controlButtons");
  reloadJimulatorButton.get_style_context()->add_class("controlButtons");
  pauseResumeButton.get_style_context()->add_class("controlButtons");
  singleStepExecuteButton.get_style_context()->add_class("controlButtons");
  haltExecutionButton.get_style_context()->add_class("controlButtons");

  // Pack buttons into a container
  this->set_layout(Gtk::BUTTONBOX_CENTER);
  this->pack_end(helpButton, false, false);
  this->pack_end(reloadJimulatorButton, false, false);
  this->pack_end(pauseResumeButton, false, false);
  this->pack_end(singleStepExecuteButton, false, false);
  this->pack_end(haltExecutionButton, false, false);
  this->show_all_children();
  this->show();
}

void ControlsView::setModel(ControlsModel* val) {
  model = val;
}

/**
 * @brief Gets the `helpButton` member variable.
 * @return Gtk::Button* A pointer to the `helpButton` member variable.
 */
Gtk::Button* ControlsView::getHelpButton() {
  return &helpButton;
}

/**
 * @brief Gets the `reloadJimulatorButton` member variable.
 * @return Gtk::Button* A pointer to the `reloadJimulatorButton` member
 * variable.
 */
Gtk::Button* ControlsView::getReloadJimulatorButton() {
  return &reloadJimulatorButton;
}
/**
 * @brief Gets the `pauseResumeButton` member variable.
 * @return Gtk::Button* A pointer to the `pauseResumeButton` member variable.
 */
Gtk::Button* ControlsView::getPauseResumeButton() {
  return &pauseResumeButton;
}
/**
 * @brief Gets the `singleStepExecuteButton` member variable.
 * @return Gtk::Button* A pointer to the `singleStepExecuteButton` member
 * variable.
 */
Gtk::Button* ControlsView::getSingleStepExecuteButton() {
  return &singleStepExecuteButton;
}
/**
 * @brief Gets the `haltExecutionButton` member variable.
 * @return Gtk::Button* A pointer to the `haltExecutionButton` member variable.
 */
Gtk::Button* ControlsView::getHaltExecutionButton() {
  return &haltExecutionButton;
}