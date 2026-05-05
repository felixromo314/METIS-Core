# 🌌 Metis-Core `v0.1.1-alpha`

* **Features added:**
* **Agent can now load the AI model via the function:**

```cpp
Agent::loadIAModel(char *pszIAModel);
```

* **License message displayed when Meta-Core.dll is loaded into memory by the host application.**



# 🌌 Metis-Core `v0.1.0-alpha`

**First release** of the high-performance C++ Deep Reinforcement Learning framework.
---
### 🚀 Key Features

*   **DQN Core:** Full implementation of Deep Q-Networks.
*   **Prioritized Experience Replay (PER):** TD-error based sampling for faster convergence.
*   **Neural Architecture:**
    *   **Structure:** 1x64 Hidden Layer.
    *   **Activation:** `Leaky ReLU`.
    *   **Target Update ($\tau$):** 0.001.
    *   **Learning Rate ($\alpha$):** 0.001.
*   **Size maximum of the State:** 1000-byte.

---

### 📅 Next Improvements (Roadmap)

*   [ ] **Multi-Agent Reinforcement Learning (MARL):** Support for collaborative/competitive environments.

---
