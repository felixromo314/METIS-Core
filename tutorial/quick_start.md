# Introduction to Reinforcement Learning (RL) with METIS-Core

In the context of **METIS-Core**, the training of an AI agent follows the fundamental Reinforcement Learning paradigm. This process is defined by the continuous interaction between three core elements:

### 1. The Environment (`Environment`)
The Environment represents the world or simulation where the agent operates. It defines the "physics" and rules of the scenario.
*   **Role:** It provides the agent with its current status and evaluates the results of its movements.
*   **In METIS-Core:** You define this by inheriting from `Metis::Environment` (e.g., an urban traffic map or a robotic warehouse).

### 2. The Agent (`Agent`)
The Agent is the autonomous entity we aim to train. It acts as the "brain" that learns from experience.
*   **Role:** It perceives the current state and decides which action to take to maximize its success over time.
*   **Goal:** To discover an optimal policy (strategy) through trial and error.

### 3. Actions (`Actions`)
Actions are the set of all possible moves or decisions available to the Agent within the Environment.
*   **Discrete Actions:** Specific choices like "Turn Left," "Turn Right," or "Accelerate."
*   **Continuous Actions:** Values such as "Steering Angle: 15.5°."

---

## The RL Feedback Loop

The learning process is a cycle that repeats millions of times during training: 

1.  **Observation:** The Agent observes the current **State** of the Environment.
2.  **Decision:** Based on its neural network, the Agent chooses an **Action**.
3.  **Impact:** The Environment updates, moving the Agent to a new State.
4.  **Reward:** The Environment gives the Agent a **Reward** (feedback).
    *   *Positive (+):* If the Police Car gets closer to the target.
    *   *Negative (-):* If the Agent crashes or loses time.

> **Key takeaway:** Through this loop, **METIS-Core** optimizes the Agent’s neural network until it can perform complex tasks—like high-speed dynamic pursuit—with precision and autonomy.

---

## Implementing Dynamic Pursuit (DQN) with METIS-Core

In this section, we will use METIS-Core to train a Police Car (Agent) to intercept a fleeing Thief Car (Target) using a Deep Q-Network (DQN).

### Step 1: Defining the Agent

In METIS-Core, every actor in the simulation must inherit from the base `Metis::Agent` class. 
```cpp
class Car : public Metis::Agent
{
public:
    // Overrides from Metis-Core
    virtual int getActionProcedural(Metis::State& state) override;
    virtual int update(double delta_time) override;
};
```
### Step 2: Defining the Environment

```cpp
typedef struct _UrbanState
{
    Metis::Vector2D policeSpeed;
    Metis::Vector2D relativeThiefPos;
    Metis::Vector2D thiefSpeed;
    double thiefDistanceToTarget;
	
    _UrbanState() {
        memset(this, 0, sizeof(_UrbanState));
    }
} TURBANSTATE;

class UrbanEnvironment : public Metis::Environment
{
public:
    // Overrides from METIS-Core
    virtual void setNumActions(int numActions) override {};
    virtual void getState(Metis::State* pState) override;
    virtual void serializeState(void* state, std::vector<float>* stateVector) override;
    virtual void applyAction(Metis::IAgent* pAgent, int actionId) override;
    virtual bool isEpisodeDone() override;
};
```

The getState function extracts the specific data from the environment and copies it to the METIS-Core framework using the command: pState->copyState(&urbanState, sizeof(TURBANSTATE));
```cpp
void UrbanEnvironment::getState(Metis::State *pState)
{
    Car* pPoliceCar = (Car*)getAgentFromID(0);
    Car* pThiefCar = (Car*)getAgentFromID(1);

    Vector2D posPolice = pPoliceCar->getPosition();
    Vector2D posThief = pThiefCar->getPosition();
    
    // Calculate relative metrics
    Vector2D relaPos = posThief - posPolice;

    Vector2D speedPolice;
    pPoliceCar->getSpeed(&speedPolice);

    Vector2D thiefSpeed;
    pThiefCar->getSpeed(&thiefSpeed);

    double halfWindow = WINDOW_WIDTH / 2.0;
    Vector2D escapeTarget(halfWindow, 0.0);
    Vector2D r = posThief - escapeTarget;
    double distanceThiefToTarget = r.magnitude();

    // Fill the state structure
    TURBANSTATE urbanState;
    urbanState.policeSpeed = speedPolice;
    urbanState.relativeThiefPos = relaPos;
    urbanState.thiefDistanceToTarget = distanceThiefToTarget;
    urbanState.thiefSpeed = thiefSpeed;

    // Copy the state of the urban environment into METIS-Core
    pState->copyState(&urbanState, sizeof(TURBANSTATE)); 
}
```
The void UrbanEnvironment::serializeState(void* state, std::vector<float>* stateVector) function is necessary to map all the variables from the urban environment into a flat vector array, which is required to feed the neural network.
```cpp
void UrbanEnvironment::serializeState(void* state, std::vector<float>* stateVector)
{
    Metis::State* pState = (Metis::State *)state;
    TURBANSTATE* pUrbanState = (TURBANSTATE*)pState->_pState;
	
    // Normalize speed between 0.0 and 1.0 based on max allowed speed
    double vPoliceSpeedNormX = pUrbanState->policeSpeed.x / (double)MAX_CAR_VELOCITY;
    stateVector->push_back(vPoliceSpeedNormX);
    
    double vPoliceSpeedNormY = pUrbanState->policeSpeed.y / (double)MAX_CAR_VELOCITY;
    stateVector->push_back(vPoliceSpeedNormY);
    
    // ... continue appending other variables ...
}
```
This method is where the action chosen by the agent (whether determined procedurally, randomly, or by the neural network) is actually applied to the Environment. This could be anything from steering left or right in a vehicle, to raising or lowering a thermostat in a smart building.

```cpp
void UrbanEnvironment::applyAction(Metis::IAgent *pAgent, int actionId)
{
    Car* pCar = (Car*)pAgent;
    
    switch (actionId)
    {
        case ACTION_TURN_LEFT:
        {
            pCar->TurnLeft();
            break;
        }
        case ACTION_TURN_RIGHT:
        {
            pCar->TurnRight();
            break;
        }
        // ... other actions ...
    }
}
```
Now that we have defined our Agent and our Environment, let’s proceed to instantiate them.
```cpp
MyView::MyView(wxFrame* parent) : wxPanel(parent) 
{
    // Initialize custom environment
    _pUrbanEnv = new UrbanEnvironment();
    
    // Initialize Agents
    _policeCar = new Car();
    _thiefCar = new Car();

    // Register agents into the Environment (which links them to METIS-Core)
    _pUrbanEnv->addAgent(_policeCar); // The agent we will train
    _pUrbanEnv->addAgent(_thiefCar);  // The opponent driven by procedural logic

    _pUrbanEnv->reset();
}
```
Now that the agents are integrated into the environment, we are ready to implement the DQN (Deep Q-Network) learning algorithm.
With just these four lines of code, the training process will begin:
```cpp
void MyView::StartTrainingPursuit()
{
    // Create the training orchestrator
    Metis::AgentTrainerDQN agentTrainer; 

    _doTraining = true;
    
    // Assign simulation callbacks
    agentTrainer.setCallbackPerStep(onStepTraining);    // Triggered after every action step
    agentTrainer.setCallbackEndEpisode(onEndEpisode);   // Triggered when an episode concludes (e.g., collision/escape)
    
    // Launch the training process
    agentTrainer.training(_pUrbanEnv, _policeCar, _thiefCar);
}
```