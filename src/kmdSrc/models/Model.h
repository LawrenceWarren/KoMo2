/**
 * @brief Describes the state of Jimulator.
 */
enum JimulatorState : int {
  UNLOADED = 0,  // Jimulator idle; empty
  LOADED,        // File just loaded for first time (not yet run)
  RUNNING,       // Jimulator is running
  PAUSED,        // Jimulator is paused
  INITIAL        // Starting state
};

class KoMo2Model;

class Model {
 public:
  virtual void changeJimulatorState(JimulatorState newState) = 0;

  // Getters and setters
  KoMo2Model* getParent();
  void setParent(KoMo2Model* val);
  JimulatorState getJimulatorState();
  void setJimulatorState(JimulatorState val);
  // TODO: Make setJimulatorState private, make KoMo2Model a friend

 private:
  KoMo2Model* parent;
  static JimulatorState jimulatorState;
};
