# Tutorial: How to Use Multi-Head Networks with Metis-Core

## Introduction: What is a Multi-Head Architecture in Reinforcement Learning?

In traditional reinforcement learning (such as a standard DQN), the neural network takes a state vector as input and outputs a single value or action. But what happens when your agent needs to perform **several distinct tasks simultaneously**? 

Furthermore, what if you want **multiple agents to collaborate** to achieve a shared goal? A Multi-Head architecture—where a centralized network processes the environment and branches out into multiple "heads" to dictate simultaneous, independent actions—is a powerful solution. This approach avoids combinatorial explosion and can be applied to vastly different fields:

* **Search & Rescue, Drone Swarms, and Warehouse Automation**
  Imagine training two Imperial TIE Fighters that must escort an Imperial Shuttle traveling from Planet A to the Death Star. The fighters must collaborate seamlessly—maintaining their formation while simultaneously making independent decisions to break off and fire if a Rebel X-Wing gets too close.

* **Datacenter Energy Optimization**
  AI agents collaborate to keep server racks cool while minimizing electricity costs. For example, one head of the network might control the HVAC cooling output, another routes network traffic away from overheating servers, and a third puts idle hard drives into sleep mode. They all work together to balance temperature and power usage in real-time.

* **Video Game "AI Directors"**
  Similar to the dynamic difficulty system used in games like *Left 4 Dead*, a Multi-Head network can control the pacing of a game. One head manages enemy spawn rates, another controls loot drops (like ammo and health), and a third adjusts the background music to match the current tension level.

* **Quantitative Trading Bots**
  As demonstrated in academic research like *"DeepScalper: A Risk-Aware Reinforcement Learning Framework to Capture Fleeting Intraday Trading Opportunities"*, a network can use multiple heads to simultaneously decide the trading direction (buy/sell), the position size (how much capital to risk), and the pricing aggressiveness in high-frequency trading.


# Tutorial: Implementing Multi-Head Networks in Metis-Core

## The Imperial Swarm Defense Scenario
To demonstrate how to build and train a Multi-Head neural network using Metis-Core, I have implemented the "Imperial Swarm Defense" example.

**The Mission Context:**
An Imperial Shuttle, carrying a critical shipment of Beskar steel destined for the Death Star, is under fire. A squadron of Rebel X-Wings has intercepted the route, determined to prevent the shuttle from reaching its destination. 

The Imperial Shuttle's only hope rests on its two escort TIE Fighters. To succeed, the fighters must maintain tight formation and coordinate their combat maneuvers—independently deciding when to break formation to engage incoming X-Wings and when to return to their escort duties. 

This scenario serves as the perfect testbed for Multi-Head architectures, as the fighters must balance navigation, formation maintenance, and combat logic simultaneously.

![METIS-Core](img/ImperialSwarmDefense800px.png)


# Main Classes.

[TODO]


