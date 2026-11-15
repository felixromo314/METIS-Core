#include "GridAgent.h"

namespace smartgrid {

    // Copy WITHOUT touching the base IAgent::_pAgentImpl (the brain owner), exactly like
    // Spacecraft in the Kamikaze example: the base is default-constructed -> fresh impl.
    GridAgent::GridAgent(GridAgent& other) : Metis::IAgent()
    {
        rol = other.rol;
        busOHacker = other.busOHacker;
        _pEnv = other._pEnv;           // the environment clone re-points this to itself
        this->setID(other.getID());
    }

    GridAgent& GridAgent::operator=(GridAgent& other)
    {
        if (this != &other)
        {
            rol = other.rol;
            busOHacker = other.busOHacker;
            _pEnv = other._pEnv;
            this->setID(other.getID());
            // base IAgent::_pAgentImpl intentionally left untouched: this keeps its own.
        }
        return *this;
    }

    // --- bodies still empty (to be filled with the real logic later) ---
    void GridAgent::applyAction(int /*accionIdx*/)
    {
        // TODO: mark the agent's INTENT in the environment (_pEnv)
    }

    bool GridAgent::isValidAction(int /*accionIdx*/)
    {
        // TODO: state-dependent mask
        return true;
    }

} // namespace smartgrid