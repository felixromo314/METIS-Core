
#include "RenderModel.h"

#include "GridEnvironment.h"
#include <algorithm>


// Solves A*x = b by Gaussian elimination with partial pivoting. Returns false if singular.
static bool solveLinear(std::vector<std::vector<double>> A, std::vector<double> b, std::vector<double>& x)
{
    const int n = (int)b.size();
    for (int c = 0; c < n; ++c)
    {
        int piv = c;
        for (int r = c + 1; r < n; ++r)
            if (std::fabs(A[r][c]) > std::fabs(A[piv][c])) piv = r;
        if (std::fabs(A[piv][c]) < 1e-12) return false;
        std::swap(A[c], A[piv]);
        std::swap(b[c], b[piv]);
        for (int r = c + 1; r < n; ++r)
        {
            const double f = A[r][c] / A[c][c];
            for (int k = c; k < n; ++k) A[r][k] -= f * A[c][k];
            b[r] -= f * b[c];
        }
    }
    x.assign(n, 0.0);
    for (int r = n - 1; r >= 0; --r)
    {
        double s = b[r];
        for (int k = r + 1; k < n; ++k) s -= A[r][k] * x[k];
        x[r] = s / A[r][r];
    }
    return true;
}

namespace smartgrid {

// NOTE: original scaffold note (bodies were empty on purpose at first).
// Logic (physics, turns, observations, reward) is now implemented below.



// One decision round == one simulation tick.
// MAX_STEPS steps span a full day, so each tick is 1440/MAX_STEPS minutes (5 min for 288).
    static constexpr float kMinutesPerStep = 1440.0f / float(MAX_STEPS);

    // Daily load multiplier (~0.60 night .. ~1.05 evening) as a function of the step within the day.
    // Deterministic on purpose: MCTS clones must reproduce the same transition, so no RNG here
    // (per-bus noise and events are added separately on top of this base curve).
    static float dailyLoadProfile(int step) 
    {
        const float frac = float(step % MAX_STEPS) / float(MAX_STEPS);   // 0..1 over the day
        const float hour = frac * 24.0f;
        const float midday = 0.30f * std::exp(-0.5f * std::pow((hour - 13.0f) / 2.5f, 2.0f));
        const float evening = 0.45f * std::exp(-0.5f * std::pow((hour - 20.0f) / 2.0f, 2.0f));
        return 0.60f + midday + evening;
    }

// --- constructor: registers the 6 players (like the drones example) ---
GridEnvironment::GridEnvironment() {
    _modelo = crearRedOpcionA();

    for (int i = 0; i < kNumBuses; ++i) {
        _subestacion[i].setID(i);
        _subestacion[i].rol        = Rol::Subestacion;
        _subestacion[i].busOHacker = i;
        _subestacion[i]._pEnv      = this;
        addAgent(&_subestacion[i]);
    }
    _striker.setID(PID_STRIKER);
    _striker.rol        = Rol::StrikerHacker;
    _striker.busOHacker = 0;
    _striker._pEnv      = this;
    addAgent(&_striker);

    _spoofer.setID(PID_SPOOFER);
    _spoofer.rol        = Rol::SpooferHacker;
    _spoofer.busOHacker = 1;
    _spoofer._pEnv      = this;
    addAgent(&_spoofer);

    _lineaCerrada.assign(_modelo.nLineas(), true);
    _flujoCongelado.assign(_modelo.nLineas(), false);
    _flujoMW.assign(_modelo.nLineas(), 0.0f);        
    _lecturaFlujoCongeladaMW.assign(_modelo.nLineas(), 0.0f);
    reset(0);
}

// --- deep copy: re-points pointers and re-registers the CLONE's agents (critical for MCTS) ---
GridEnvironment::GridEnvironment(GridEnvironment& o) {
    _modelo         = o._modelo;
    _currentPlayer  = o._currentPlayer;
    _step           = o._step;
    _winner         = o._winner;
    _demandaConsumidoresMW  = o._demandaConsumidoresMW;
    _deslastradoMW  = o._deslastradoMW;
    _botnetMW       = o._botnetMW;
    _enCuarentena   = o._enCuarentena;
    _lineaCerrada   = o._lineaCerrada;
    _flujoMW        = o._flujoMW;
    _presupuestoStriker = o._presupuestoStriker;
    _presupuestoSpoofer = o._presupuestoSpoofer;
    _contadorCongelado  = o._contadorCongelado;
    _flujoCongelado     = o._flujoCongelado;
    _lecturaFlujoCongeladaMW = o._lecturaFlujoCongeladaMW;
    _falsaAlarma        = o._falsaAlarma;
    _lecturaCongeladaMW = o._lecturaCongeladaMW;

    _stepBotnet = o._stepBotnet;
    _stepFreezeMeter = o._stepFreezeMeter;
    _stepTrip = o._stepTrip;
    _stepFreezeFlow = o._stepFreezeFlow;

    for (int i = 0; i < kNumBuses; ++i) {
        _subestacion[i]       = o._subestacion[i];
        _subestacion[i]._pEnv = this;            // re-point to the clone
        addAgent(&_subestacion[i]);              // register the clone's agents
    }
    _striker       = o._striker;  _striker._pEnv = this;  addAgent(&_striker);
    _spoofer       = o._spoofer;  _spoofer._pEnv = this;  addAgent(&_spoofer);

    _demandaRealDia = o._demandaRealDia;
    _generacionDia = o._generacionDia;
    _eventoStrikerDia = o._eventoStrikerDia;
    _eventoSpooferDia = o._eventoSpooferDia;
    _pendStriker = o._pendStriker;
    _pendSpoofer = o._pendSpoofer;
    _eventoSyncDia = o._eventoSyncDia;
    _pendSync = o._pendSync;
    _realDemandMW = o._realDemandMW;
    _realServedMW = o._realServedMW;
    _sumaFraccionServida = o._sumaFraccionServida;
    _rondasContadas = o._rondasContadas;

    _fraccionServida = o._fraccionServida;
    for (int p = 0; p < MAX_PLAYERS; ++p) _ultimaAccion[p] = o._ultimaAccion[p];
}

// --- lifecycle ---
void GridEnvironment::reset(int /*tipo*/) {
    _step   = 0;
    _winner = TEAM_DRAW_ID;
    _currentPlayer  = PID_STRIKER;            // the Red Team moves first
    _presupuestoStriker = kStrikerPresupuestoEpisodio;
    _presupuestoSpoofer = kSpooferPresupuestoEpisodio;
    
    _fraccionServida = 1.0f;                        // IMPORTANT: prevents the blackout from leaking into the next episode
    _demandaRealDia.assign(MAX_STEPS, -1.0f);
    _generacionDia.assign(MAX_STEPS, -1.0f);
    _eventoStrikerDia.assign(MAX_STEPS, 0.0f);
    _eventoSpooferDia.assign(MAX_STEPS, 0.0f);
    _eventoSyncDia.assign(MAX_STEPS, 0.0f);
    _pendStriker = 0.0f;
    _pendSpoofer = 0.0f;
    _pendSync = 0;

    _demandaConsumidoresMW = _modelo.cargaBaseMW;   // size it
    const float prof0 = dailyLoadProfile(0);
    for (int i = 0; i < kNumBuses; ++i) _demandaConsumidoresMW[i] *= prof0;

    _realDemandMW = 0.0f;
    for (int i = 0; i < kNumBuses; ++i) _realDemandMW += _demandaConsumidoresMW[i];
    _realServedMW = _realDemandMW;   // before the first attack, everything is served
    _sumaFraccionServida = 0.0;
    _rondasContadas = 0;

    _deslastradoMW.fill(0.0f);
    _botnetMW.fill(0.0f);
    _enCuarentena.fill(false);
    _contadorCongelado.fill(false);
    _falsaAlarma.fill(false);
    _lecturaCongeladaMW.fill(0.0f);
    std::fill(_lineaCerrada.begin(),   _lineaCerrada.end(),   true);
    std::fill(_flujoCongelado.begin(), _flujoCongelado.end(), false);
    std::fill(_lecturaFlujoCongeladaMW.begin(), _lecturaFlujoCongeladaMW.end(), 0.0f);

    _stepBotnet.fill(-1);
    _stepFreezeMeter.fill(-1);
    _stepTrip.fill(-1);
    _stepFreezeFlow.fill(-1);
}

void GridEnvironment::update(float /*dt*/)
{
    if (_currentPlayer == PID_STRIKER)
    {
        resolverFisicaFinDeRonda();   // per-round tick: solve flows, cascade, blackout for this round
        // advance the day (one tick == kMinutesPerStep, 5 min)
        ++_step;

        if (_step % kRedRecargaCadaSteps == 0) {   // each hacker ADDS points gradually (accumulative, up to a cap)
            _presupuestoStriker += kRedRecargaPorTramo;
            if (_presupuestoStriker > kRedPresupuestoTope) _presupuestoStriker = kRedPresupuestoTope;
            _presupuestoSpoofer += kRedRecargaPorTramo;
            if (_presupuestoSpoofer > kRedPresupuestoTope) _presupuestoSpoofer = kRedPresupuestoTope;
        }

        const float profile = dailyLoadProfile(_step);
        for (int i = 0; i < kNumBuses; ++i)
            _demandaConsumidoresMW[i] = _modelo.cargaBaseMW[i] * profile;
    }
    
    
}
bool GridEnvironment::isEqualState(Metis::IAlphaGoZeroStateBase* /*p*/) 
{
    // TODO: compare what affects the future of the game
    return false;
}
// Fills the shared grid block. measured=true -> Blue's spoofable view; false -> the truth (Red).
void GridEnvironment::fillGridBlock(GridBlockObs& g, bool measured) const
{
    const float gmax = (_modelo.genMaxMW > 0.0f) ? _modelo.genMaxMW : 1.0f;
    float demandaTotal = 0.0f;

    // --- per bus ---
    for (int i = 0; i < kNumBuses; ++i)
    {
        float demReal = _demandaConsumidoresMW[i] + _botnetMW[i] - _deslastradoMW[i];   // real demand
        if (demReal < 0.0f) demReal = 0.0f;

        // Spoofer froze the meter -> Blue reads the last normal value, not the truth.
        float demObs = (measured && _contadorCongelado[i]) ? _lecturaCongeladaMW[i] : demReal;

        g.buses[i].demanda = demObs / gmax;                 // normalized
        g.buses[i].deslastrado = _deslastradoMW[i] / gmax;
        g.buses[i].cuarentena = _enCuarentena[i] ? 1.0f : 0.0f;

        demandaTotal += measured ? demObs : demReal;            // Blue sums what it MEASURES (consistent with the deception); Red sums the truth
    }

    // --- per line ---
    for (int l = 0; l < _modelo.nLineas(); ++l)
    {
        const float cap = _modelo.lineas[l].capacidadMW;
        // Spoofer froze the flow -> Blue reads the last normal value, not the real one.
        const float flujoObs = (measured && _flujoCongelado[l]) ? _lecturaFlujoCongeladaMW[l] : _flujoMW[l];

        g.lineas[l].flujo = flujoObs / gmax;
        g.lineas[l].ratio = (cap > 0.0f) ? std::fabs(flujoObs) / cap : 0.0f;
        g.lineas[l].cerrada = _lineaCerrada[l] ? 1.0f : 0.0f;
    }

    // False alarm (Blue's view only): fake an overload on the lines touching the alarmed bus.
    if (measured)
    {
        for (int i = 0; i < kNumBuses; ++i)
            if (_falsaAlarma[i])
                for (int l = 0; l < _modelo.nLineas(); ++l)
                    if (_modelo.lineas[l].from == i || _modelo.lineas[l].to == i)
                        g.lineas[l].ratio = 1.10f;
    }

    // --- global ---
    float gen = demandaTotal;                                   // slack follows the load...
    if (gen > _modelo.genMaxMW) gen = _modelo.genMaxMW;         // ...capped at genMax
    g.generacion = gen / gmax;
    g.demandaTotal = demandaTotal / gmax;

    const float ang = 2.0f * 3.14159265f * (float(_step % MAX_STEPS) / float(MAX_STEPS));
    g.faseSin = std::sin(ang);                                  // time of day
    g.faseCos = std::cos(ang);
}

std::vector<float> GridEnvironment::getState(int currentPlayer)
{
    std::vector<float> spaceVector;

    if (teamDe(currentPlayer) == TEAM_BLUE_ID)
    {
        BlueObs aBlueObs;
        fillGridBlock(aBlueObs.grid, /*measured=*/true);        // Blue sees the spoofable view
        aBlueObs.oneHotSub[currentPlayer] = 1.0f;               // currentPlayer 0..3 == bus A..D

        spaceVector.reserve(TEAM_BLUE_INPUTS);
        aBlueObs.toVector(spaceVector);                          // -> 32 floats
    }
    else
    {
        RedObs aRedObs;
        fillGridBlock(aRedObs.grid, /*measured=*/false);        // Red sees the truth
        aRedObs.presupuesto = (currentPlayer == PID_STRIKER)
            ? float(_presupuestoStriker) / float(kRedPresupuestoTope)
            : float(_presupuestoSpoofer) / float(kRedPresupuestoTope);
        aRedObs.oneHotHacker[currentPlayer == PID_STRIKER ? 0 : 1] = 1.0f;

        // sync: which bus the Striker hit and how long ago (so the Spoofer can freeze THAT bus in time)
        for (int i = 0; i < kNumBuses; ++i)
        {
            if (_botnetMW[i] > 1.0f && _stepBotnet[i] >= 0)
            {
                int dt = _step - _stepBotnet[i];
                if (dt < 0) dt = 0;
                aRedObs.botnetReciente[i] = std::exp(-float(dt) / 12.0f);   // dt=0 -> 1.0 ; dt=12 -> 0.37 ; dt=48 -> 0.02
            }
        }

        spaceVector.reserve(TEAM_RED_INPUTS);
        aRedObs.toVector(spaceVector);                           // -> 35 floats
    }

    return spaceVector;
}


// --- actions and turns ---
void GridEnvironment::applyAction(int accionIdx)
{
    const int pid = _currentPlayer;
    _ultimaAccion[pid] = accionIdx;

    if (teamDe(pid) == TEAM_BLUE_ID)
    {
        const Bus sub = busDeSubestacion(pid);
        const int i = int(sub);
        const BlueEfecto e = blueDecodificar(_modelo, sub, accionIdx);
        switch (e.op)
        {
        case BlueOp::Shed:
            _deslastradoMW[i] += e.fraccion * _modelo.cargaBaseMW[i];
            if (_deslastradoMW[i] > _modelo.cargaBaseMW[i]) _deslastradoMW[i] = _modelo.cargaBaseMW[i];
            break;
        case BlueOp::Restore:
            _deslastradoMW[i] = 0.0f;
            _enCuarentena[i] = false;          // Restore is also the way out of quarantine
            break;
        case BlueOp::ToggleLinea:
            if (e.linea >= 0) _lineaCerrada[e.linea] = !_lineaCerrada[e.linea];
            break;
        case BlueOp::Quarantine:
            _enCuarentena[i] = true;
            break;
        case BlueOp::Verify:
            _contadorCongelado[i] = false;      // revealing the truth clears the spoof on this bus
            _falsaAlarma[i] = false;
            // Verify also clears the frozen flow of the lines attached to this bus
            for (int l = 0; l < _modelo.nLineas(); ++l)
                if (_modelo.lineas[l].from == i || _modelo.lineas[l].to == i)
                    _flujoCongelado[l] = false;
            break;
        case BlueOp::NoOp:
        default:
            break;
        }
    }
    else if (pid == PID_STRIKER)
    {
        const StrikerEfecto e = strikerDecodificar(accionIdx);
        const int cost = strikerCoste(STRIKER_ACTIONS(accionIdx));
        if (cost <= _presupuestoStriker)           // Striker's own budget
        {
            _presupuestoStriker -= cost;
            if (e.release)                 _botnetMW.fill(0.0f);
            else if (e.bus >= 0)           _botnetMW[e.bus] = e.deltaMW * _modelo.cargaBaseMW[e.bus];   // fake load proportional to the bus
            else if (e.lineaDisparo >= 0)  _lineaCerrada[e.lineaDisparo] = false;  // hacked breaker trips

            // attack intensity for the graph (heat per attacker)
            if (e.bus >= 0)                _pendStriker += e.deltaMW * _modelo.cargaBaseMW[e.bus];   // botnet: injected MW
            else if (e.lineaDisparo >= 0)  _pendStriker += 30.0f;         // line trip (fixed intensity)
            else if (e.release)            _pendStriker += 10.0f;         // botnet release

            if (e.release)                 _stepBotnet.fill(-1);              // botnet released
            else if (e.bus >= 0)           _stepBotnet[e.bus] = _step;        // time of the hit
            else if (e.lineaDisparo >= 0)  _stepTrip[e.lineaDisparo] = _step;

            // SYNC: hit on a bus the Spoofer ALREADY masked recently (pre-masked)
            if (e.bus >= 0 && _contadorCongelado[e.bus] && _stepFreezeMeter[e.bus] >= 0 &&
                _step - _stepFreezeMeter[e.bus] <= kSyncVentana)
                _pendSync |= (1 << e.bus);
        }
    }
    else // PID_SPOOFER
    {
        const SpooferEfecto e = spooferDecodificar(accionIdx);
        const int cost = spooferCoste(SPOOFER_ACTIONS(accionIdx));
        if (cost <= _presupuestoSpoofer)           // Spoofer's own budget
        {
            _presupuestoSpoofer -= cost;
            switch (e.tipo)
            {
            case EnganoTipo::CongelarContador:
                if (e.bus >= 0) {
                    _contadorCongelado[e.bus] = true; 
                    _lecturaCongeladaMW[e.bus] = _demandaConsumidoresMW[e.bus];   // believable "normal" reading (no botnet)
                    _stepFreezeMeter[e.bus] = _step;
                }                     // time of the masking
                break;
            case EnganoTipo::CongelarFlujo:
                if (e.linea >= 0 && e.linea < (int)_flujoCongelado.size())
                {
                    _flujoCongelado[e.linea] = true;
                    _lecturaFlujoCongeladaMW[e.linea] = _flujoMW[e.linea];
                    _stepFreezeFlow[e.linea] = _step;                                    // time of the masking
                }
                break;
            case EnganoTipo::FalsaAlarma:
                if (e.bus >= 0) _falsaAlarma[e.bus] = true;
                break;
            default:
                break;
            }

            // SYNC: deception (meter freeze or false alarm) on the bus the Striker just hit
            if ((e.tipo == EnganoTipo::CongelarContador || e.tipo == EnganoTipo::FalsaAlarma) && e.bus >= 0 &&
                _botnetMW[e.bus] > 1.0f && _stepBotnet[e.bus] >= 0 && _step - _stepBotnet[e.bus] <= kSyncVentana)
                _pendSync |= (1 << e.bus);

            if (e.tipo != EnganoTipo::Ninguno) _pendSpoofer += 1.0f;   // one deception applied
        }
    }

    // Pass the turn. When it wraps back to the first player, the round is complete -> run physics.
    advanceToNextActivePlayer(false);
    
}

void GridEnvironment::fillRenderState(GridRenderState& s, bool mostrarVerdad) const
{
    // Start from the static layout (positions, labels, hacker placement, capacities).
    s = buildRenderOpcionA(Fase::Normal);

    const float gmax = _modelo.genMaxMW;
    float demandaTotal = 0.0f;

    // --- buses ---
    for (int i = 0; i < kNumBuses; ++i)
    {
        float demReal = _demandaConsumidoresMW[i] + _botnetMW[i] - _deslastradoMW[i];
        if (demReal < 0.0f) demReal = 0.0f;

        float demShow = demReal;
        if (!mostrarVerdad && _contadorCongelado[i]) demShow = _lecturaCongeladaMW[i];

        s.buses[i].demandaMW = demShow;
        s.buses[i].deslastradoMW = _deslastradoMW[i];
        s.buses[i].cuarentena = _enCuarentena[i];
        s.buses[i].contadorCongelado = (!mostrarVerdad) && _contadorCongelado[i];
        s.buses[i].falsaAlarma = (!mostrarVerdad) && _falsaAlarma[i];

        demandaTotal += demReal;   // totals always use the truth
    }

    // --- lines ---
    for (int l = 0; l < _modelo.nLineas() && l < (int)s.lineas.size(); ++l)
    {
        s.lineas[l].flujoMW = _flujoMW[l];
        s.lineas[l].capacidadMW = _modelo.lineas[l].capacidadMW;
        s.lineas[l].cerrada = _lineaCerrada[l];
    }

    // --- global ---
    float gen = demandaTotal;
    if (gen > gmax) gen = gmax;
    s.generacionMW = gen;
    s.genMaxMW = gmax;
    s.presupuestoRed = _presupuestoStriker + _presupuestoSpoofer;   // Red total for the HUD
    s.step = _step;
    s.maxSteps = MAX_STEPS;
    s.winner = _winner;          // -1 draw/ongoing, 0 Blue, 1 Red (matches TEAM_*_ID)
    s.fase = Fase::Normal;     // no fixed phase during training

    // turn + per-agent last action + real demand total (for the daily graph)
    s.currentPlayer = _currentPlayer;
    s.demandaRealTotalMW = demandaTotal;

    for (int i = 0; i < kNumBuses; ++i)
        s.buses[i].ultimaAccion = nombreBlue(Bus(i), _ultimaAccion[i]);

    for (HackerView& h : s.hackers)
        h.ultimaAccion = h.esStriker ? nombreStriker(_ultimaAccion[PID_STRIKER])
        : nombreSpoofer(_ultimaAccion[PID_SPOOFER]);

    s.demandaRealDia = _demandaRealDia;
    s.generacionDia = _generacionDia;
    s.eventoStrikerDia = _eventoStrikerDia;
    s.eventoSpooferDia = _eventoSpooferDia;
    s.eventoSyncDia = _eventoSyncDia;
    s.realDemandMW = _realDemandMW;
    s.realServedMW = _realServedMW;
}
void GridEnvironment::forecastDemandaDia(std::vector<float>& out) const
{
    out.assign(MAX_STEPS, 0.0f);
    for (int step = 0; step < MAX_STEPS; ++step)
    {
        const float prof = dailyLoadProfile(step);   // static function in this .cpp
        float tot = 0.0f;
        for (int i = 0; i < kNumBuses; ++i) tot += _modelo.cargaBaseMW[i] * prof;
        out[step] = tot;
    }
}
void GridEnvironment::getPossibleActions(int playerID, std::vector<TGridAction>& out) 
{
    out.clear();

    int idx[kRedMaxActions];                        // widest head (Red = 12)
    const int n = legalActions(playerID, idx, kRedMaxActions);

    out.reserve(n);
    for (int j = 0; j < n; ++j)
    {
        TGridAction a;
        a.id = idx[j];
        out.push_back(a);
    }
}

bool GridEnvironment::isTerminal()
{
    if (_fraccionServida < kUmbralBlackout)   // Red wins: the grid collapsed
    {
        _winner = TEAM_RED_ID;
        return true;
    }
    if (_step >= MAX_STEPS)                   // Blue wins: it kept the grid up all day
    {
        _winner = TEAM_BLUE_ID;
        return true;
    }
    _winner = TEAM_DRAW_ID;
    return false;
}
float GridEnvironment::getMaterialHeuristicValue(int playerToEvaluate) {
    const float gmax = (_modelo.genMaxMW > 0.0f) ? _modelo.genMaxMW : 1.0f;

    // Base: margin of customers served vs the blackout threshold (same threshold as isTerminal).
    float vBlue = margenServicio(_fraccionServida);

    // (a) VISIBLE congestion: brings the grid closer to a cascade.
    float maxRatio = 0.0f;
    for (int l = 0; l < _modelo.nLineas(); ++l) {
        const float cap = _modelo.lineas[l].capacidadMW;
        if (cap <= 0.0f || !_lineaCerrada[l]) continue;
        const float r = std::fabs(_flujoMW[l]) / cap;
        if (r > maxRatio) maxRatio = r;
    }
    if (maxRatio > 1.0f)
        vBlue -= 0.3f * ((maxRatio - 1.0f > 1.0f) ? 1.0f : (maxRatio - 1.0f));

    const float TAU = 4.0f;
    float amenazaOculta = 0.0f;
    for (int i = 0; i < kNumBuses; ++i)
    {
        if (_botnetMW[i] <= 1.0f || !_contadorCongelado[i]) continue;      // botnet and freeze both active on THAT bus
        float sync = 0.0f;
        if (_stepBotnet[i] >= 0 && _stepFreezeMeter[i] >= 0)
        {
            int dt = _stepFreezeMeter[i] - _stepBotnet[i];
            const bool tarde = (dt > 0);                                     // masked AFTER the hit
            if (dt < 0) dt = -dt;
            sync = std::exp(-float(dt) / TAU);
            if (tarde) sync *= 0.5f;                                         // Blue may already have seen the attack
        }
        amenazaOculta += (_botnetMW[i] / gmax) * (0.5f + 0.5f * sync);
    }
    for (int l = 0; l < _modelo.nLineas(); ++l)
    {
        if (!_lineaCerrada[l] || !_flujoCongelado[l]) continue;            // IN-SERVICE line with frozen flow
        const float cap = _modelo.lineas[l].capacidadMW;
        if (cap <= 0.0f) continue;
        const float ratioReal = std::fabs(_flujoMW[l]) / cap;
        const float ratioVisto = std::fabs(_lecturaFlujoCongeladaMW[l]) / cap;
        float oculto = ratioReal - ratioVisto;                              // load Blue does NOT see
        if (ratioReal < 0.85f || oculto <= 0.0f) continue;                  // only if it hides dangerous load
        if (oculto > 1.0f) oculto = 1.0f;
        amenazaOculta += 0.3f * oculto;
    }

    if (amenazaOculta > 1.0f) amenazaOculta = 1.0f;   // tope

    vBlue -= 0.6f * amenazaOculta;

    if (vBlue > 1.0f) vBlue = 1.0f;
    if (vBlue < -1.0f) vBlue = -1.0f;

    // Clean zero-sum: Red sees the opposite value. Coordinated masking raises Red's value,
    // and Blue has a symmetric incentive to use Verify (clearing the spoof lowers the hidden threat).
    return (teamDe(playerToEvaluate) == TEAM_BLUE_ID) ? vBlue : -vBlue;
}
// --- turn / state helpers ---
bool GridEnvironment::isPlayerActive(int /*pid*/) {
    // TODO: all active for now (a quarantined substation still chooses NoOp/Restore)
    return true;
}

void GridEnvironment::advanceToNextActivePlayer(bool incluirActual) {
    const int start = incluirActual ? 0 : 1;
    for (int i = start; i <= MAX_PLAYERS; ++i) 
    {
        const int c = (_currentPlayer + i) % MAX_PLAYERS;
        if (isPlayerActive(c)) 
        { 
            _currentPlayer = c; 
             return; 
        }
    }
}

void GridEnvironment::construirContextoAcciones(ActionContext& ctx) const {
    for (int i = 0; i < kNumBuses; ++i) 
    {
        ctx.deslastradoMW[i] = _deslastradoMW[i];
        ctx.enCuarentena[i]  = _enCuarentena[i];
        ctx.botnetMW[i]      = _botnetMW[i];
    }
    ctx.presupuestoRed = _presupuestoStriker;   // ctx.presupuestoRed is only used by the Striker logic
}

void GridEnvironment::resolverFisicaFinDeRonda()
{
    const int nb = kNumBuses;
    const int nl = _modelo.nLineas();
    const int slack = _modelo.busSlack;

    // Real demand per bus (quarantined buses stay isolated -> their demand is unserved).
    std::vector<double> demand(nb, 0.0);
    double totalDemand = 0.0;
    for (int i = 0; i < nb; ++i)
    {
        double d = _demandaConsumidoresMW[i] + _botnetMW[i] - _deslastradoMW[i];
        if (d < 0.0) d = 0.0;
        demand[i] = d;
        totalDemand += d;
    }

    // Active lines: closed and with no quarantined endpoint.
    std::vector<char> active(nl, 0);
    for (int l = 0; l < nl; ++l)
    {
        const int f = _modelo.lineas[l].from;
        const int t = _modelo.lineas[l].to;
        active[l] = (_lineaCerrada[l] && !_enCuarentena[f] && !_enCuarentena[t]) ? 1 : 0;
    }

    // REAL customer demand (no botnet, shedding not subtracted): what the grid should serve.
    double realDemand = 0.0;
    for (int i = 0; i < nb; ++i) realDemand += _demandaConsumidoresMW[i];

    std::vector<double> flow(nl, 0.0);
    double totalServed = 0.0;
    double realServed  = 0.0;   // real customers actually supplied

    // Cascade: solve, trip the most overloaded line, repeat until stable.
    for (int iter = 0; iter < nl + 2; ++iter)
    {
        // Island reachable from the slack (BFS over active lines, skipping quarantined buses).
        std::vector<char> inIsland(nb, 0);
        if (!_enCuarentena[slack])
        {
            std::vector<int> stack;
            stack.push_back(slack);
            inIsland[slack] = 1;
            while (!stack.empty())
            {
                const int u = stack.back();
                stack.pop_back();
                for (int l = 0; l < nl; ++l)
                {
                    if (!active[l]) continue;
                    const int a = _modelo.lineas[l].from;
                    const int b = _modelo.lineas[l].to;
                    int v = -1;
                    if (a == u) v = b; else if (b == u) v = a;
                    if (v >= 0 && !inIsland[v] && !_enCuarentena[v]) { inIsland[v] = 1; stack.push_back(v); }
                }
            }
        }

        // Island demand and UFLS scale (the central cannot exceed genMax).
        double islandDemand = 0.0;
        for (int i = 0; i < nb; ++i)
            if (inIsland[i] && i != slack) islandDemand += demand[i];

        double scale = 1.0;
        if (islandDemand > _modelo.genMaxMW && islandDemand > 1e-9)
            scale = _modelo.genMaxMW / islandDemand;

        // Reduced Laplacian B*theta = P over island non-slack buses.
        std::vector<int> idx(nb, -1);
        int n = 0;
        for (int i = 0; i < nb; ++i)
            if (inIsland[i] && i != slack) idx[i] = n++;

        std::vector<double> theta(nb, 0.0);
        if (n > 0)
        {
            std::vector<std::vector<double>> B(n, std::vector<double>(n, 0.0));
            std::vector<double> P(n, 0.0);
            for (int l = 0; l < nl; ++l)
            {
                if (!active[l]) continue;
                const int a = _modelo.lineas[l].from;
                const int b = _modelo.lineas[l].to;
                if (!inIsland[a] || !inIsland[b]) continue;
                const double bb = _modelo.lineas[l].susceptancia;
                const int ia = idx[a];
                const int ib = idx[b];
                if (ia >= 0) B[ia][ia] += bb;
                if (ib >= 0) B[ib][ib] += bb;
                if (ia >= 0 && ib >= 0) { B[ia][ib] -= bb; B[ib][ia] -= bb; }
            }
            for (int i = 0; i < nb; ++i)
                if (idx[i] >= 0) P[idx[i]] = -demand[i] * scale;   // consumption actually served

            std::vector<double> x;
            if (solveLinear(B, P, x))
                for (int i = 0; i < nb; ++i)
                    if (idx[i] >= 0) theta[i] = x[idx[i]];
        }

        // Flows on active, in-island lines.
        for (int l = 0; l < nl; ++l)
        {
            if (!active[l]) { flow[l] = 0.0; continue; }
            const int a = _modelo.lineas[l].from;
            const int b = _modelo.lineas[l].to;
            if (!inIsland[a] || !inIsland[b]) { flow[l] = 0.0; continue; }
            flow[l] = _modelo.lineas[l].susceptancia * (theta[a] - theta[b]);
        }

        // Most overloaded line -> trip it and cascade; none -> stable.
        int worst = -1;
        double worstOver = 0.0;
        for (int l = 0; l < nl; ++l)
        {
            if (!active[l]) continue;
            const double over = std::fabs(flow[l]) - _modelo.lineas[l].capacidadMW;
            if (over > 1e-6 && over > worstOver) { worstOver = over; worst = l; }
        }
        if (worst < 0)
        {
            totalServed = islandDemand * scale;
            // REAL customers served: energized island, minus shed load (shedding counts as NOT served),
            // scaled by UFLS (the botnet steals capacity and cuts power to real customers).
            realServed = 0.0;
            for (int i = 0; i < nb; ++i)
            {
                if (!inIsland[i] || i == slack) continue;
                double cust = _demandaConsumidoresMW[i] - _deslastradoMW[i];
                if (cust < 0.0) cust = 0.0;
                realServed += cust * scale;
            }
            break;
        }
        active[worst] = 0;
        _lineaCerrada[worst] = false;   // the trip persists (Blue can reclose with ToggleLinea)
    }

    // Store results.
    for (int l = 0; l < nl; ++l) _flujoMW[l] = (float)flow[l];
    // REAL served fraction: real customers supplied / real customers that wanted power.
    // So shedding real customers DOES lower Blue's score (real cost of shedding).
    _fraccionServida = (realDemand > 1e-9) ? (float)(realServed / realDemand) : 1.0f;
    _realDemandMW = (float)realDemand;   // for the BALANCE panel (same truth as the graph)
    _realServedMW = (float)realServed;
    _sumaFraccionServida += _fraccionServida;   // for the graded service reward
    ++_rondasContadas;

    // daily history (consistent with the forecast, which sums cargaBase*profile = real demand)
    if (_step >= 0 && _step < (int)_demandaRealDia.size())
    {
        _demandaRealDia[_step] = (float)realDemand;    // what real customers wanted
        _generacionDia[_step]  = (float)realServed;    // what they actually received
        _eventoStrikerDia[_step] = _pendStriker;       // Striker attack intensity this round
        _eventoSpooferDia[_step] = _pendSpoofer;       // Spoofer deception intensity this round
        _eventoSyncDia[_step]    = (float)_pendSync;   // buses hit in a synchronized way this round
    }
    _pendStriker = 0.0f;   // reset accumulators for the next round
    _pendSpoofer = 0.0f;
    _pendSync = 0;

    //// advance the day here (one round == one 5-min tick), independent of update()
    //++_step;
    //const float profile = dailyLoadProfile(_step);
    //for (int i = 0; i < kNumBuses; ++i)
    //    _demandaConsumidoresMW[i] = _modelo.cargaBaseMW[i] * profile;
}
bool GridEnvironment::is_valid_action(int action)
{
    const int pid = _currentPlayer;                 // the action refers to the acting player

    // Out of the head range -> never valid.
    if (action < 0 || action >= numActionsDe(pid))
        return false;

    int idx[kRedMaxActions];
    const int n = legalActions(pid, idx, kRedMaxActions);
    for (int j = 0; j < n; ++j)
        if (idx[j] == action)
            return true;

    return false;
}

// Fills out[] with the legal action indices for playerID; returns how many.
// Single source of legality, shared by getPossibleActions() and is_valid_action().
int GridEnvironment::legalActions(int playerID, int* out, int maxOut) const
{
    ActionContext ctx;
    construirContextoAcciones(ctx);

    if (teamDe(playerID) == TEAM_BLUE_ID)
        return blueAccionesLegales(_modelo, busDeSubestacion(playerID), ctx, out, maxOut);
    
    if (playerID == PID_STRIKER)
    {
        // remove useless hits: Trip on an already open line, or a botnet that doesn't raise the current one
        int tmp[kRedMaxActions];
        const int nAll = strikerAccionesLegales(ctx, tmp, kRedMaxActions);
        int n = 0;
        for (int j = 0; j < nAll && n < maxOut; ++j)
        {
            const StrikerEfecto e = strikerDecodificar(tmp[j]);
            if (e.release) continue;
            if (e.lineaDisparo >= 0 && !_lineaCerrada[e.lineaDisparo]) continue;
            if (e.bus >= 0 && e.deltaMW * _modelo.cargaBaseMW[e.bus] <= _botnetMW[e.bus]) continue;
            if (e.bus >= 0 && _botnetMW[e.bus] <= 1.0f)                       // botnet on a NEW bus:
            {
                int activos = 0;                                              // only if fewer than kMaxBusesBotnet are active
                for (int b = 0; b < kNumBuses; ++b) if (_botnetMW[b] > 1.0f) ++activos;
                if (activos >= kMaxBusesBotnet) continue;
            }
            out[n++] = tmp[j];                                              // NoOp/Release pass
        }
        return n;
    }
    // PID_SPOOFER: budget already reflects what the striker spent this round
    
    // PID_SPOOFER: remove useless deceptions (target already frozen, or line out of service)
    int tmp[kRedMaxActions];
    const int nAll = spooferAccionesLegales(ctx, _presupuestoSpoofer, tmp, kRedMaxActions);
    int n = 0;
    for (int j = 0; j < nAll && n < maxOut; ++j)
    {
        const SpooferEfecto e = spooferDecodificar(tmp[j]);
        if (e.tipo == EnganoTipo::CongelarContador && e.bus >= 0 && _contadorCongelado[e.bus]) continue;
        if (e.tipo == EnganoTipo::CongelarFlujo && e.linea >= 0 &&
            (_flujoCongelado[e.linea] || !_lineaCerrada[e.linea])) continue;
        if (e.tipo == EnganoTipo::FalsaAlarma && e.bus >= 0 && _falsaAlarma[e.bus]) continue;
        out[n++] = tmp[j];                                                  // NoOp always allowed
    }
    return n;
}

} // namespace smartgrid
