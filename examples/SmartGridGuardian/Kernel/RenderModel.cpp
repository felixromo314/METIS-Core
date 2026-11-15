#include "RenderModel.h"

namespace smartgrid {

// On-screen topology (square with the power plant hanging off A on the left):
//    CENTRAL --- A ------------- B
//                |               |
//                C ------------- D
GridRenderState buildRenderOpcionA(Fase fase) {
    GridRenderState s;
    s.fase = fase;

    // --- buses (indices: 0=A 1=B 2=C 3=D) ---
    BusView A; A.x = 360; A.y = 230; A.label = "A"; A.esCentral = true;  // A is the power plant gateway
    BusView B; B.x = 700; B.y = 230; B.label = "B"; B.demandaMW = 30;
    BusView C; C.x = 360; C.y = 500; C.label = "C"; C.demandaMW = 30;
    BusView D; D.x = 700; D.y = 500; D.label = "D"; D.demandaMW = 40;
    s.buses = { A, B, C, D };

    // --- lines: AB, AC, BD, CD with capacities 60/60/40/40 ---
    LineView AB; AB.from = 0; AB.to = 1; AB.capacidadMW = 60;
    LineView AC; AC.from = 0; AC.to = 2; AC.capacidadMW = 60;
    LineView BD; BD.from = 1; BD.to = 3; BD.capacidadMW = 40;
    LineView CD; CD.from = 2; CD.to = 3; CD.capacidadMW = 40;

    // --- hackers (corners) ---
    HackerView striker; striker.x = 880; striker.y = 150; striker.label = "StrikerHacker"; striker.esStriker = true;
    HackerView spoofer; spoofer.x = 880; spoofer.y = 560; spoofer.label = "SpooferHacker"; spoofer.esStriker = false;

    // Numbers per phase (computed in the test cases)
    switch (fase) {
        case Fase::Normal:
            AB.flujoMW = 50; AC.flujoMW = 50; BD.flujoMW = 20; CD.flujoMW = 20;
            s.generacionMW = 100; s.presupuestoRed = 6;
            striker.ultimaAccion = "NoOp"; spoofer.ultimaAccion = "NoOp";
            break;
        case Fase::Ataque:   // StrikerHacker: Botnet25_B -> B goes from 30 to 55
            AB.flujoMW = 68.75f; AC.flujoMW = 56.25f; BD.flujoMW = 13.75f; CD.flujoMW = 26.25f;
            s.generacionMW = 125; s.presupuestoRed = 4;
            s.buses[1].demandaMW = 55;
            striker.ultimaAccion = "Botnet25_B"; spoofer.ultimaAccion = "NoOp";
            break;
        case Fase::Caso1_Blackout:   // Spoofer masks, Blue does not react -> AB and AC trip -> blackout
            AB.flujoMW = 0; AB.cerrada = false;
            AC.flujoMW = 0; AC.cerrada = false;
            BD.flujoMW = 0; CD.flujoMW = 0;
            s.generacionMW = 0; s.presupuestoRed = 3; s.winner = 1;
            s.buses[1].demandaMW = 55; s.buses[1].contadorCongelado = true;
            striker.ultimaAccion = "Botnet25_B"; spoofer.ultimaAccion = "FreezeMeter_B";
            break;
        case Fase::Caso2_Defendido:  // B Shed25 + D Shed25 -> AB baja a 58.13
            AB.flujoMW = 58.13f; AC.flujoMW = 49.38f; BD.flujoMW = 15.63f; CD.flujoMW = 24.38f;
            s.generacionMW = 114.5f; s.presupuestoRed = 4; s.winner = 0;
            s.buses[1].demandaMW = 47.5f; s.buses[1].deslastradoMW = 7.5f; s.buses[1].ultimaAccion = "Shed25";
            s.buses[3].demandaMW = 35.0f; s.buses[3].deslastradoMW = 5.0f; s.buses[3].ultimaAccion = "Shed25";
            striker.ultimaAccion = "Botnet25_B"; spoofer.ultimaAccion = "FreezeMeter_B";
            break;
    }
    s.lineas  = { AB, AC, BD, CD };
    s.hackers = { striker, spoofer };
    return s;
}

} // namespace smartgrid
