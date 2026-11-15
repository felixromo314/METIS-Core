#pragma once
// Agent of the environment (derives from Metis::IAgent), analogous to the Kamikaze
// example's Spacecraft. A single type with a different ROL: substation (Blue) or hacker (Red).

#include "METIS-Core.h"   // real header in Visual Studio
#include "Actions.h"

namespace smartgrid {

    enum class Rol : uint8_t { Subestacion, StrikerHacker, SpooferHacker };

    class GridEnvironment;   // fwd

    class GridAgent : public Metis::IAgent {
    public:
        Rol  rol = Rol::Subestacion;
        int  busOHacker = -1;                 // bus (if substation) or 0/1 (if hacker)
        GridEnvironment* _pEnv = nullptr;     // re-pointed when cloning (critical for MCTS)

        // --- constructors / assignment ---
        // IMPORTANT: copy must NOT touch the base IAgent::_pAgentImpl (the brain owner),
        // exactly like Spacecraft: the base is default-constructed -> fresh impl, so cloning
        // the environment never shares/double-frees the brain.
        GridAgent() = default;
        GridAgent(GridAgent& other);
        GridAgent& operator=(GridAgent& other);

        // Applies the action index. Does not touch global physics: it only marks the agent's
        // INTENT in the environment (the environment resolves the flow in update()).
        void applyAction(int accionIdx);

        // --- framework overrides ---
        int  update(double /*dt*/) override { return 0; }   // physics lives in the environment
        bool isValidAction(int accionIdx) override;
        void endStep() override {}

        bool operator!=(const GridAgent& o) const
        {
            return rol != o.rol || busOHacker != o.busOHacker;
        }
    };

} // namespace smartgrid