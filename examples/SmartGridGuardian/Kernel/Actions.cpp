#include "Actions.h"

namespace smartgrid {

    // ============================================================================
    // Blue decode: 'sub' selects the head, 'k' is that head's local index.
    // ============================================================================
    BlueEfecto blueDecodificar(const GridTopology& topo, Bus sub, int k)
    {
        const int bus = int(sub);
        switch (sub)
        {
        case Bus::A:
            switch (ACT_A(k))
            {
            case ACT_A::ToggleL1: return { BlueOp::ToggleLinea, bus, 0.0f, topo.lineaPropiaDe(bus, 0) };
            case ACT_A::ToggleL2: return { BlueOp::ToggleLinea, bus, 0.0f, topo.lineaPropiaDe(bus, 1) };
            case ACT_A::Verify:   return { BlueOp::Verify, bus };
            default:              return { BlueOp::NoOp, bus };
            }
        case Bus::B:
        case Bus::C:   // B and C share ACT_B's numbering
            switch (ACT_B(k))
            {
            case ACT_B::Shed10:     return { BlueOp::Shed, bus, 0.10f };
            case ACT_B::Shed25:     return { BlueOp::Shed, bus, 0.25f };
            case ACT_B::Restore:    return { BlueOp::Restore, bus };
            case ACT_B::Verify:     return { BlueOp::Verify, bus };
            case ACT_B::Quarantine: return { BlueOp::Quarantine, bus };
            default:                return { BlueOp::NoOp, bus };
            }
        case Bus::D:
            switch (ACT_D(k))
            {
            case ACT_D::Shed10:     return { BlueOp::Shed, bus, 0.10f };
            case ACT_D::Shed25:     return { BlueOp::Shed, bus, 0.25f };
            case ACT_D::Restore:    return { BlueOp::Restore, bus };
            case ACT_D::ToggleL1:   return { BlueOp::ToggleLinea, bus, 0.0f, topo.lineaPropiaDe(bus, 0) };
            case ACT_D::ToggleL2:   return { BlueOp::ToggleLinea, bus, 0.0f, topo.lineaPropiaDe(bus, 1) };
            case ACT_D::Verify:     return { BlueOp::Verify, bus };
            case ACT_D::Quarantine: return { BlueOp::Quarantine, bus };
            default:                return { BlueOp::NoOp, bus };
            }
        }
        return { BlueOp::NoOp, bus };
    }

    // ============================================================================
    // Striker / Spoofer decode (head-local index)
    // ============================================================================
    StrikerEfecto strikerDecodificar(int k)
    {
        switch (STRIKER_ACTIONS(k))
        {
        case STRIKER_ACTIONS::Botnet10_B: return { int(Bus::B), 0.25f, -1, false };   // 25% of the bus base load
        case STRIKER_ACTIONS::Botnet10_C: return { int(Bus::C), 0.25f, -1, false };   // 25% of the bus base load
        case STRIKER_ACTIONS::Botnet10_D: return { int(Bus::D), 0.25f, -1, false };   // 25% of the bus base load
        case STRIKER_ACTIONS::Botnet25_B: return { int(Bus::B), 0.50f, -1, false };   // 50% of the bus base load
        case STRIKER_ACTIONS::Botnet25_C: return { int(Bus::C), 0.50f, -1, false };   // 50% of the bus base load
        case STRIKER_ACTIONS::Botnet25_D: return { int(Bus::D), 0.50f, -1, false };   // 50% of the bus base load
        case STRIKER_ACTIONS::Release:    return { -1, 0.0f, -1, true };
        case STRIKER_ACTIONS::Trip_AB:    return { -1, 0.0f, int(Line::AB), false };
        case STRIKER_ACTIONS::Trip_AC:    return { -1, 0.0f, int(Line::AC), false };
        case STRIKER_ACTIONS::Trip_BD:    return { -1, 0.0f, int(Line::BD), false };
        case STRIKER_ACTIONS::Trip_CD:    return { -1, 0.0f, int(Line::CD), false };
        default:                          return {};
        }
    }

    SpooferEfecto spooferDecodificar(int k)
    {
        switch (SPOOFER_ACTIONS(k))
        {
        case SPOOFER_ACTIONS::FreezeMeter_B: return { EnganoTipo::CongelarContador, int(Bus::B), -1 };
        case SPOOFER_ACTIONS::FreezeMeter_C: return { EnganoTipo::CongelarContador, int(Bus::C), -1 };
        case SPOOFER_ACTIONS::FreezeMeter_D: return { EnganoTipo::CongelarContador, int(Bus::D), -1 };
        case SPOOFER_ACTIONS::FreezeFlow_AB: return { EnganoTipo::CongelarFlujo, -1, int(Line::AB) };
        case SPOOFER_ACTIONS::FreezeFlow_AC: return { EnganoTipo::CongelarFlujo, -1, int(Line::AC) };
        case SPOOFER_ACTIONS::FreezeFlow_BD: return { EnganoTipo::CongelarFlujo, -1, int(Line::BD) };
        case SPOOFER_ACTIONS::FreezeFlow_CD: return { EnganoTipo::CongelarFlujo, -1, int(Line::CD) };
        case SPOOFER_ACTIONS::FalseAlarm_B:  return { EnganoTipo::FalsaAlarma, int(Bus::B), -1 };
        case SPOOFER_ACTIONS::FalseAlarm_C:  return { EnganoTipo::FalsaAlarma, int(Bus::C), -1 };
        case SPOOFER_ACTIONS::FalseAlarm_D:  return { EnganoTipo::FalsaAlarma, int(Bus::D), -1 };
        default:                             return {};
        }
    }

    // ============================================================================
    // Legal head-local indices (written to out[], count returned)
    // ============================================================================
    int blueAccionesLegales(const GridTopology& topo, Bus sub, const ActionContext& ctx, int* out, int maxOut)
    {
        const int   i = int(sub);
        const float cargaBase = topo.cargaBaseMW[i];
        const int   count = blueNumActions(sub);     // head-local width
        int n = 0;
        for (int k = 0; k < count && n < maxOut; ++k)
        {
            const BlueEfecto e = blueDecodificar(topo, sub, k);
            bool ok = true;
            if (ctx.enCuarentena[i])           ok = (e.op == BlueOp::NoOp || e.op == BlueOp::Restore);
            else if (e.op == BlueOp::Restore)  ok = ctx.deslastradoMW[i] > 0.0f;
            else if (e.op == BlueOp::Shed)     ok = ctx.deslastradoMW[i] + e.fraccion * cargaBase <= cargaBase + 1e-4f;
            if (ok) out[n++] = k;                        // head-local index
        }
        return n;
    }

    int strikerAccionesLegales(const ActionContext& ctx, int* out, int maxOut)
    {
        bool hayBotnet = false;
        for (float mw : ctx.botnetMW) hayBotnet |= (mw > 0.0f);
        int n = 0;
        for (int k = 0; k < int(STRIKER_ACTIONS::MAX_ACTIONS) && n < maxOut; ++k)
        {
            const auto a = STRIKER_ACTIONS(k);
            if (strikerCoste(a) > ctx.presupuestoRed) continue;
            if (a == STRIKER_ACTIONS::Release && !hayBotnet) continue;
            out[n++] = k;
        }
        return n;
    }

    int spooferAccionesLegales(const ActionContext& /*ctx*/, int presupuestoTrasStriker, int* out, int maxOut)
    {
        int n = 0;
        for (int k = 0; k < int(SPOOFER_ACTIONS::MAX_ACTIONS) && n < maxOut; ++k)
            if (spooferCoste(SPOOFER_ACTIONS(k)) <= presupuestoTrasStriker) out[n++] = k;
        return n;
    }

    // ============================================================================
    // Names (for logs / visualization)
    // ============================================================================
    const char* nombreBlue(Bus sub, int k)
    {
        static const char* nA[] = { "NoOp","ToggleL1","ToggleL2","Verify" };
        static const char* nB[] = { "NoOp","Shed10","Shed25","Restore","Verify","Quarantine" };
        static const char* nD[] = { "NoOp","Shed10","Shed25","Restore","ToggleL1","ToggleL2","Verify","Quarantine" };
        if (k < 0 || k >= blueNumActions(sub)) return "?";
    switch (sub) { case Bus::A: return nA[k]; case Bus::D: return nD[k]; default: return nB[k]; }
    }

    const char* nombreStriker(int k)
    {
        static const char* n[] = { "NoOp","Botnet10_B","Botnet10_C","Botnet10_D","Botnet25_B","Botnet25_C","Botnet25_D","Release","Trip_AB","Trip_AC","Trip_BD","Trip_CD" };
        return (k >= 0 && k < int(STRIKER_ACTIONS::MAX_ACTIONS)) ? n[k] : "?";
    }

    const char* nombreSpoofer(int k)
    {
        static const char* n[] = { "NoOp","FreezeMeter_B","FreezeMeter_C","FreezeMeter_D","FreezeFlow_AB","FreezeFlow_AC","FreezeFlow_BD","FreezeFlow_CD","FalseAlarm_B","FalseAlarm_C","FalseAlarm_D" };
        return (k >= 0 && k < int(SPOOFER_ACTIONS::MAX_ACTIONS)) ? n[k] : "?";
    }

    const char* nombreBus(int pid)
    {
        static const char* n[] = { "A","B","C","D" };
        return (pid >= 0 && pid < kNumBuses) ? n[pid] : "?";
    }

} // namespace smartgrid