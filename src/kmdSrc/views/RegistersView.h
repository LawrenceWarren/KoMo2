/**
 * @file RegistersView.h
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief This file declares the class RegistersView, which represents the
 * viewable GUI element that displays information about the memory within
 * Jimulator. The model is represented in the file `RegistersModel.cpp` and it's
 * associated header.
 * @version 1.0.0
 * @date 10-04-2021
 */

#include <gtkmm/buttonbox.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <array>
#include <string>

class MainWindowView;
class RegistersModel;

/**
 * @brief Stores data and functions related to the displaying of the KoMo2 GUI
 * element - no particular logic or data should be stored here other than what
 * can be directly seen on screen.
 */
class RegistersView : public Gtk::VButtonBox {
 public:
  RegistersView(MainWindowView* const parent);

  // ! Getters and setters
  RegistersModel* const getModel() const;
  void setModel(RegistersModel* const val);
  std::array<std::array<Gtk::Label, 16>, 2>* const getLabels();

 private:
  // General functions
  void initRegisterViewContainer();
  void initLeftHandLabel(const int j);
  void initRightHandLabel(const int j);
  void initAllLabels(const int i, const int j);
  void initGrid();

  /**
   * @brief The layout that stores all of the labelArray members.
   */
  Gtk::Grid grid;
  /**
   * @brief An array of labels, with the left side representing the registers
   * name and the right side representing the registers value.
   */
  std::array<std::array<Gtk::Label, 16>, 2> labelArray;

  /**
   * @brief A pointer to the parent view.
   */
  MainWindowView* parent;

  /**
   * @brief A pointer to the related model.
   */
  RegistersModel* model;

  // ! Deleted special member functions
  // stops these functions from being misused, creates a sensible error
  RegistersView(const RegistersView&) = delete;
  RegistersView(const RegistersView&&) = delete;
  RegistersView& operator=(const RegistersView&) = delete;
  RegistersView& operator=(const RegistersView&&) = delete;
};
