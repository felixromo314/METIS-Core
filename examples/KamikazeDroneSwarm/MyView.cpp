//#include "stdafx.h"
#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <wx/docview.h>
#include <wx/dcclient.h>

#include "METIS-Core.h"

#include "MyView.h"
#include "Enviroment.h"
#include "MemoryLeaks.h"
#include <assert.h>
#include <numeric>
#include <algorithm> // Para std::max



#include "Kernel/Spacecraft.h"

#define TIME_EACH_TICK 33.0

wxBEGIN_EVENT_TABLE(MyView, wxPanel)
EVT_PAINT(MyView::OnPaint)
EVT_TIMER(wxID_HIGHEST, MyView::OnTimer)   //Receive Timer event declaration
wxEND_EVENT_TABLE()

MyView *MyView::_pSelf = NULL;
static FILE* metrics_fp = NULL;

double calculatePercentSuccess(double minVal, double maxVal, double value)
{
    // Evitar división por cero
    if (maxVal <= minVal)
        return 0.0;

    // Clamping del valor
    if (value <= minVal)
        return 0.0;

    if (value >= maxVal)
        return 100.0;


    // Cálculo del porcentaje
    double percent = ((value - minVal) / (maxVal - minVal)) * 100.0;
    return percent;
}


double calculateAverageReward(std::vector<double> vectorReward)
{
    if (vectorReward.empty()) {
        return 0.0;
    }

    // Calculamos la suma de todos los elementos
    double sum = std::accumulate(vectorReward.begin(), vectorReward.end(), 0.0);

    // Retornamos la media
    return sum / static_cast<double>(vectorReward.size());
}


#pragma region  CALLBACKS_OF_TRAINING
// callback when Metis-Core execute a step in the traning
void onStepTraining(void* pSender, Metis::TSEARCHACTIONASYM* pMetrics)
{
    MyView::_pSelf->_episode = pMetrics->episode;
    MyView::_pSelf->_iCycle = pMetrics->step;

    SpaceEnviroment* pEnv = (SpaceEnviroment*)pMetrics->pState;

    int teamID = MyView::_pSelf->_pSpaceEnviroment->getTeamID(pMetrics->playerID);
    if (teamID == 0)
    {
        MyView::_pSelf->_teamBlue_expectedValues.push_back(pMetrics->actionValue);
    }
    if (teamID == 1)
    {
        MyView::_pSelf->_teamRed_expectedValues.push_back(pMetrics->actionValue);
    }



#ifdef _DEBUG
    MyView::_pSelf->drawStep(pSender, pMetrics->episode, pMetrics->step, pEnv);
#else
    if (MyView::_pSelf->_bDisplayInRealTime)
    {
        wxMilliSleep(100);
    }
    if (MyView::_pSelf->_bDisplayTranning)
    {
        wxTheApp->Yield();
        MyView::_pSelf->Refresh();  // Mark the window as needing a repaint
        MyView::_pSelf->Update();
    }
#endif

    if ((pMetrics->step % 10) == 0)
    {
        wxTheApp->Yield();
    }


}


//callback when the Metis-Core ends a episode
void onEndEpisode(void* pSender, Metis::TSEARCHACTIONASYM* pMetrics)
{
    MyView::_pSelf->OnEndEpisode(pMetrics);
}
#pragma endregion


void MyView::OnEndEpisode(Metis::TSEARCHACTIONASYM* pMetrics)
{
    _episode = pMetrics->episode;
    _iCycle = pMetrics->step;

    double policyLossMeanShutle = 0;
    double policyLossMeanDrone1 = 0;
    double policyLossMeanDrone2 = 0;

    double valueLossMeanTeamBlue = 0.0;
    double valueLossMeanTeamRed = 0.0;

    double meanExpected_Value_TeamBlue = 0.0;
    double meanExpected_Value_RedBlue = 0.0;

    double meanExpected_Value_TeamBlueXepi = 0.0;
    double meanExpected_Value_RedBlueXepi = 0.0;


    meanExpected_Value_TeamBlueXepi = calculateAverageReward(_teamBlue_expectedValues);
    meanExpected_Value_RedBlueXepi = calculateAverageReward(_teamRed_expectedValues);

    _teamBlue_expectedValues.clear();
    _teamRed_expectedValues.clear();

    if (MyView::_pSelf->_policyLossHistoryShuttle.size() >= METRICS_X_EPISODES)
    {
        // Eliminamos el elemento más antiguo (índice 0)
        _policyLossHistoryShuttle.erase(MyView::_pSelf->_policyLossHistoryShuttle.begin());
        _policyLossHistoryDrone_1.erase(MyView::_pSelf->_policyLossHistoryDrone_1.begin());
        _policyLossHistoryDrone_2.erase(MyView::_pSelf->_policyLossHistoryDrone_2.begin());

        _MeanTeamBlue_expectedValues.erase(MyView::_pSelf->_MeanTeamBlue_expectedValues.begin());
        _MeanTeamRed_expectedValues.erase(MyView::_pSelf->_MeanTeamRed_expectedValues.begin());

        policyLossMeanShutle = calculateAverageReward(_policyLossHistoryShuttle);
        policyLossMeanDrone1 = calculateAverageReward(_policyLossHistoryDrone_1);
        policyLossMeanDrone2 = calculateAverageReward(_policyLossHistoryDrone_2);

        meanExpected_Value_TeamBlue = calculateAverageReward(_MeanTeamBlue_expectedValues);
        meanExpected_Value_RedBlue = calculateAverageReward(_MeanTeamRed_expectedValues);
    }

    valueLossMeanTeamBlue = calculateAverageReward(_valueLossTeamBlue);
    valueLossMeanTeamRed = calculateAverageReward(_valueLossTeamRed);

    



    int teamWinner = _pSpaceEnviroment->getWinner();
        

    int countWinner = _countWinner[teamWinner];
    countWinner++;
    _countWinner[teamWinner] = countWinner;

    if (metrics_fp == NULL)
    {
        metrics_fp = fopen("traningKamikazeDrones.txt", "w+t");
    }

    fprintf(metrics_fp, "\n========================================================\n");
    fprintf(metrics_fp, "Episode:%d   maxDepthActionNode:%d\n", pMetrics->episode, pMetrics->maxDepthActionNode);
    fprintf(metrics_fp, "   winner:%d\n", teamWinner);
    fprintf(metrics_fp, "   policyLossMeanDrone1:%.3f     policyLossMeanDrone2:%.3f    policyLossMeanShutle:%.3f\n\n", policyLossMeanDrone1, policyLossMeanDrone2, policyLossMeanShutle);
    fprintf(metrics_fp, "   meanlossValueTeamBlue:%.3f     meanlossValueTeamRed:%.3f\n", valueLossMeanTeamBlue, valueLossMeanTeamRed);
    fprintf(metrics_fp, "   meanExpected_Value_TeamBlue:%.3f     meanExpected_Value_RedBlue:%.3f\n", meanExpected_Value_TeamBlue, meanExpected_Value_RedBlue);
    
    //calculo porcnetaje ganadores
    int winCounterBlue = _countWinner[TEAM_BLUE_ID];
    int winCounterRed = _countWinner[TEAM_RED_ID];
    int winCounterTables = _countWinner[-1];


    float teamBlueWinPercent = (float) (winCounterBlue / (float)pMetrics->episode) * 100.0;
    float teamRedWinPercent = (float) (winCounterRed / (float) pMetrics->episode) * 100.0;
    float tablasWinPercent = (float)(winCounterTables / (float)pMetrics->episode) * 100.0;

    _blueWinPercent = teamBlueWinPercent;
    _redWinPercent = teamRedWinPercent;
    _drawPercent = tablasWinPercent;

    
    if (_historyWinTeamBlue.size() >= 400)
    {
        // Eliminamos el elemento más antiguo (índice 0)
        _historyWinTeamBlue.erase(_historyWinTeamBlue.begin());
        _historyWinTeamRed.erase(_historyWinTeamRed.begin());
        _historyWinTablas.erase(_historyWinTablas.begin());

        _valueLossTeamBlue.erase(_valueLossTeamBlue.begin());
        _valueLossTeamRed.erase(_valueLossTeamRed.begin());
    }

    _historyWinTeamBlue.push_back(teamBlueWinPercent);
    _historyWinTeamRed.push_back(teamRedWinPercent);
    _historyWinTablas.push_back(tablasWinPercent);

    _MeanTeamBlue_expectedValues.push_back(meanExpected_Value_TeamBlueXepi);
    _MeanTeamRed_expectedValues.push_back(meanExpected_Value_RedBlueXepi);


    

    fprintf(metrics_fp, "   teamBlueWinPercent:%.3f     teamRedWinPercent:%.3f    tablasWinPercent:%.3f\n", teamBlueWinPercent, teamRedWinPercent, tablasWinPercent);
    
    fprintf(metrics_fp, "  BLUE TEAM\n");
    TASYMMULTIHEADAGENTMETRICS& metric = pMetrics->metricsTeamBlue;
    std::map<std::string, ASYMTHEADMETRIC>::iterator it;// _headMetric; // reconpensa obtenida al final de step
    
    

    float valueLossHead_Blue = metric._meanValueHeadLoss;

    for (it = metric._headMetric.begin(); it != metric._headMetric.end(); it++)
    {
        std::string sHead = it->first;
        const ASYMTHEADMETRIC& metric = it->second;
        float polityLossHead = metric._meanPolicyHeadLoss;
        
        fprintf(metrics_fp, " Head:%s  polityLossHead=%.3f\n",sHead.c_str(), polityLossHead);
        if (sHead == NAME_XWINDRONE_1)
        {
            _policyLossHistoryDrone_1.push_back(polityLossHead);
            
        }
            
        if (sHead == NAME_XWINDRONE_2)
            _policyLossHistoryDrone_2.push_back(polityLossHead);

    }
    _valueLossTeamBlue.push_back(valueLossHead_Blue);
    fprintf(metrics_fp, "   valueLossHead_Blue:%.3f\n", valueLossHead_Blue);
    
    fprintf(metrics_fp, "  RED TEAM\n");
    TASYMMULTIHEADAGENTMETRICS& metric2 = pMetrics->metricsTeamRed;

    float valueLossHead_Red = metric2._meanValueHeadLoss;


    for (it = metric2._headMetric.begin(); it != metric2._headMetric.end(); it++)
    {
        std::string sHead = it->first;
        const ASYMTHEADMETRIC& metric = it->second;
        float policyLossHead = metric._meanPolicyHeadLoss;
        fprintf(metrics_fp, " Head:%s  policyLossHead=%.3f\n", sHead.c_str(), policyLossHead);
        _policyLossHistoryShuttle.push_back(policyLossHead);
    }


    _valueLossTeamRed.push_back(valueLossHead_Red);
    fprintf(metrics_fp, "   valueLossHead_Red:%.3f\n", valueLossHead_Red);


    fflush(metrics_fp);

    
}

MyView::MyView(wxFrame* parent)
    : wxPanel(parent) {
    
       
    _pSpaceEnviroment = new SpaceEnviroment();

    _drone_XWing_1 = &_pSpaceEnviroment->_drone_XWing_1;
    _drone_XWing_1->setID(0); // ID of the player must be sequencial (from 0 o N)
    _drone_XWing_1->setColor(255,255, 0);
    _drone_XWing_1->setHeading(90.0);
    _drone_XWing_1->_speedMaxSpeed = MAX_SPEED_DRON_XWING;
    _drone_XWing_1->setSpeed(MAX_SPEED_DRON_XWING);
    _drone_XWing_1->setShipType(ShipType::XWingDrone);


    _drone_XWing_2 = &_pSpaceEnviroment->_drone_XWing_2;
    _drone_XWing_2->setID(1);
    _drone_XWing_2->setColor(0, 255, 0);
    _drone_XWing_2->setHeading(90.0);
    _drone_XWing_2->_speedMaxSpeed = MAX_SPEED_DRON_XWING;
    _drone_XWing_2->setSpeed(MAX_SPEED_DRON_XWING);
    _drone_XWing_2->setShipType(ShipType::XWingDrone);

    _drone_XWing_1->_dronWingMan = _drone_XWing_2->getID();
    _drone_XWing_2->_dronWingMan = _drone_XWing_1->getID();

    
    _ImperialShuttle = &_pSpaceEnviroment->_ImperialShuttle;
    _ImperialShuttle->setID(2);
    _ImperialShuttle->setColor(255, 255, 255);
    _ImperialShuttle->setHeading(90.0);
    _ImperialShuttle->_speedMaxSpeed = MAX_SPEED_SHUTTLE; // too lazy to create a method :-(
    _ImperialShuttle->setSpeed(MAX_SPEED_SHUTTLE);
    _ImperialShuttle->setShipType(ShipType::Shuttle);


    _IsPresent_GPU = Metis::isCUDAavailable();

    
    

    //spacecraft can access to the enviroment
    _ImperialShuttle->_pEnv = _pSpaceEnviroment;
    _drone_XWing_1->_pEnv = _pSpaceEnviroment;
    _drone_XWing_1->_pEnv = _pSpaceEnviroment;
    
    _countWinner[-1] = 0;
    _countWinner[TEAM_BLUE_ID] = 0;
    _countWinner[TEAM_RED_ID] = 0;

    
    _pSpaceEnviroment->reset(0);
        

    m_pPlayTimer = new wxTimer(this, wxID_ANY);
    Bind(wxEVT_TIMER, &MyView::OnTimer, this, m_pPlayTimer->GetId());

    MyView::_pSelf = this;

    _bDisplayInRealTime = false;
    _bDisplayTranning = false;
    _doTranning = false;

    _maxMeanRewardTotal = 0.0;
    _countMaxSuccessPer50 = -1;
    wxImage::AddHandler(new wxPNGHandler);

    
    bool bRes = _logoMetisCore.LoadFile("logo_metis_watermark.png", wxBITMAP_TYPE_PNG);
    _logoMetisCore.SetMaskColour(0, 255, 0);


    this->SetBackgroundStyle(wxBG_STYLE_PAINT); // to avoid window repaint the background and avoid the flicking


    //Ini the blue team and red team
    // build the teams
    Metis::MultiHeadBrainBuilder multiBuilderTeamBlue;
    Metis::MultiHeadBrainBuilder multiBuilderTeamRed;
    _IsPresent_GPU = Metis::isCUDAavailable();



    //build the multi-agent blue team that will have 2 x-wing drones
    _pTeamBlue = multiBuilderTeamBlue.addHead(std::string(NAME_XWINDRONE_1), _drone_XWing_1)
        .addHead(std::string(NAME_XWINDRONE_2), _drone_XWing_2)
        .addValueHead(std::string("valueHead"))
        .setNumberInputsOutputs(TEAM_BLUE_INPUTS, (int)TEAM_BLUE_ACTIONS::MAX_ACTIONS)
        .useGPU(_IsPresent_GPU).build();


    _pTeamBlue->setID(TEAM_BLUE_ID);

    // Build the multi-agent red team, that only have 1 entity
    _pTeamRed = multiBuilderTeamRed.addHead(std::string(NAME_SHUTTLE), _ImperialShuttle)
        .addValueHead(std::string("valueHead"))
        .setNumberInputsOutputs(TEAM_RED_INPUTS, (int)TEAM_RED_ACTIONS::MAX_ACTIONS)
        .useGPU(_IsPresent_GPU)
        .build();

    _pTeamRed->setID(TEAM_RED_ID);

    

}

MyView::~MyView()
{

    

}

void MyView::OnSize(wxSizeEvent& event)
{
    wxSize size = GetSize();  // Aquí puedes obtener las dimensiones reales
    wxRect clientRC = this->GetClientRect();
    
    Refresh();  // Mark the window as needing a repaint
    Update();
}

bool MyView::isPlaying()
{
    if (_modeBotPlay > 0)
    {
        return true;
    }
    else return false;
}


// modeBot: 1 man in the loop
//          2: bot procedural
void MyView::Play(int modeBot)
{

    _modeBotPlay = modeBot;

    //cargar los archivos ia
    _pTeamBlue->loadIAModel((char*)"blueTeam.ia", false);
    _pTeamRed->loadIAModel((char*)"redTeam.ia",false);


    _pSpaceEnviroment->reset(0);

    _teamBluePlayAgent = Metis::createAsymAlphaZeroGoAgentFromModelFile(_pSpaceEnviroment,(char *)"",0, 0,(char *) "", 0, 0, false);
    _teamBluePlayAgent->setTeamBlueRed(_pTeamBlue, _pTeamRed);
    _teamBluePlayAgent->setSearchIterations(150); // numero of simulations in MCTS
    //_teamBluePlayAgent->setMaterialHeuristicWeight(0.3);

    //srand(123);

    m_pPlayTimer = new wxTimer(this, wxID_ANY);
    Bind(wxEVT_TIMER, &MyView::OnTimer, this, m_pPlayTimer->GetId());
    // Start the timer with a 1000 ms (1 second) interval
    m_pPlayTimer->Start(TIME_EACH_TICK); // Interval in milliseconds

    this->SetFocus();

}
int MyView::ReadFromKeyboard()
{
    int action=0;

    action = (int) TEAM_RED_ACTIONS::NONE;
    if (wxGetKeyState(WXK_LEFT))
    {
        action = (int)TEAM_RED_ACTIONS::ROTATE_LEFT_TURRET;
    }
    if (wxGetKeyState(WXK_RIGHT))
    {
        action = (int)TEAM_RED_ACTIONS::ROTATE_RIGHT_TURRET;
    }
    
    if (wxGetKeyState((wxKeyCode)'f'))
    {
        action = (int)TEAM_RED_ACTIONS::FIRE_LASER;
    }

    return action;
}

void MyView::OnTimer(wxTimerEvent& event)
{
    DWORD updateTimerThreadID = GetCurrentThreadId();

    
    //_pSpaceEnviroment->getState((Metis::State*)&spaceState);
    //_pSpaceEnviroment->serizalizeState((Metis::State*)&spaceState, &spaceState._stateVector);

    //
    
    Spacecraft* pDron1 = (Spacecraft*)_pSpaceEnviroment->getAgentByID(DRON_1);
    Spacecraft* pDron2 = (Spacecraft*)_pSpaceEnviroment->getAgentByID(DRON_2);
    Spacecraft* pShuttle = (Spacecraft*)_pSpaceEnviroment->getAgentByID(SHUTTLE_ID);

    Spacecraft* drones[2];
    drones[0] = pDron1;
    drones[1] = pDron2;
            
    // turno del dron 1
    _pSpaceEnviroment->_currentPlayer = DRON_1;
    int dron1Action = _teamBluePlayAgent->selectActionAsym(_pSpaceEnviroment, DRON_1);
    _pSpaceEnviroment->applyAction(dron1Action);          // avanza _currentPlayer al dron 2

    // turno del dron 2: decide viendo ya la acción del dron 1, igual que en entrenamiento
    int dron2Action = _teamBluePlayAgent->selectActionAsym(_pSpaceEnviroment, DRON_2);
    _pSpaceEnviroment->applyAction(dron2Action);          // avanza al shuttle

    // turno del shuttle (humano)
    int shuttle_Action = ReadFromKeyboard();
    _pSpaceEnviroment->applyAction(shuttle_Action);       // vuelve al dron 1

    _pSpaceEnviroment->update(_pSpaceEnviroment->getDeltaTime());   // física una vez por ronda


    ////// --- chequear quien ha ganado
    Metis::Vector2D posShuttle = pShuttle->getPosition();
    Metis::Vector2D posTarget = _pSpaceEnviroment->_posTarget;
    Metis::Vector2D r = posShuttle.DistanceTo(posTarget);
    if (r.magnitude() < 10)
    {
        m_pPlayTimer->Stop();
        delete m_pPlayTimer;
        m_pPlayTimer = NULL;
        _modeBotPlay = -1; // reset the play mode
        wxMessageBox("The Imperial shuttle has arrived at the Death Star!!!. You human win!!");
    }
    if ( pShuttle->getState() == destroyed )
    {
        m_pPlayTimer->Stop();
        delete m_pPlayTimer;
        m_pPlayTimer = NULL;
        _modeBotPlay = -1; // reset the play mode
        wxMessageBox("The Imperial shuttle destroyed by a kamikaze dron. IA win!!");
    }

    for (int i = 0; i < 2; i++)
    {
        Metis::Vector2D posDronRelativ = drones[i]->getPosition() - posShuttle;
        double dronDist = posDronRelativ.magnitude(); // relative to the shuttle
        if (dronDist < 25)
        {
            m_pPlayTimer->Stop();
            delete m_pPlayTimer;
            m_pPlayTimer = NULL;
            _modeBotPlay = -1; // reset the play mode
            wxMessageBox("The Imperial shuttle destroyed by a kamikaze dron. IA win!!");
        }
    }
    
    
    MyView::_pSelf->drawStep(this, 0, 1, _pSpaceEnviroment);

    //Refresh();  // Mark the window as needing a repaint
    //Update();   // Process the repaint immediately
}
void MyView::drawStep(void* pSender, int episode, int step, SpaceEnviroment *pEnv)
{
    wxClientDC dc(this); // Get the DC for the client area of the window (for debugin, instand paiting)
    

    // Background
    dc.SetBackground(wxBrush(wxColour(10, 20, 30)));
    dc.Clear();

    Draw(dc, pEnv);
}

void MyView::DrawSpacecraft(wxDC& dc, Spacecraft* pCraft)
{
    

    pCraft->Draw(&dc);


}



void MyView::DrawDeathStar(wxDC* pDC, int cx, int cy, float scale)
{
    // Radio principal de la estación espacial
    int mainRadius = std::max(5, (int)(25 * scale));

    // Grosores dinámicos
    int hullThickness = std::max(1, (int)(2 * scale));
    int trenchThickness = std::max(1, (int)(3 * scale));
    int dishThickness = std::max(1, (int)(1 * scale));

    // ==========================================
    // 1. CASCO PRINCIPAL (La Esfera)
    // ==========================================
    // Borde gris claro, relleno gris plomo oscuro (Tono Imperial)
    wxPen penHull(wxColour(180, 180, 180), hullThickness);
    wxBrush brushHull(wxColour(70, 70, 70));
    pDC->SetPen(penHull);
    pDC->SetBrush(brushHull);

    pDC->DrawCircle(wxPoint(cx, cy), mainRadius);

    // ==========================================
    // 2. TRINCHERA ECUATORIAL
    // ==========================================
    // Una línea negra y gruesa que cruza la esfera de lado a lado por el centro
    wxPen penTrench(wxColour(20, 20, 20), trenchThickness);
    pDC->SetPen(penTrench);

    pDC->DrawLine(cx - mainRadius, cy, cx + mainRadius, cy);

    // ==========================================
    // 3. PLATO DEL SUPERLÁSER
    // ==========================================
    // Lo colocamos en el cuadrante superior derecho
    int dishRadius = std::max(3, (int)(7 * scale));
    int dishX = cx + (int)(8 * scale);
    int dishY = cy - (int)(10 * scale);

    // Borde gris oscuro, interior negro simulando profundidad
    wxPen penDish(wxColour(100, 100, 100), dishThickness);
    wxBrush brushDish(wxColour(30, 30, 30));
    pDC->SetPen(penDish);
    pDC->SetBrush(brushDish);

    pDC->DrawCircle(wxPoint(dishX, dishY), dishRadius);

    // ==========================================
    // 4. DETALLE DEL NÚCLEO DEL LÁSER
    // ==========================================
    // Un círculo minúsculo en el centro del plato
    pDC->SetBrush(*wxBLACK_BRUSH);
    pDC->DrawCircle(wxPoint(dishX, dishY), std::max(1, dishRadius / 3));
}


void MyView::Draw(wxDC& dc, SpaceEnviroment* pEnviroment)
{
    // Background
    dc.SetBackground(wxBrush(wxColour(10, 20, 30)));
    dc.Clear();

    wxSize sizeWindow = dc.GetSize();
    // 1. Crear el Pen con el estilo wxPENSTYLE_DOT. 
// OJO: En wxWidgets (y en la API de Windows por debajo), los estilos de línea 
// (punteado, discontinuo) solo funcionan bien si el grosor (width) es 1. 
// Si le pones grosor 3, el SO suele ignorar el patrón y dibujarla sólida.
    dc.SetPen(wxPen(wxColour(40, 50, 70), 1, wxPENSTYLE_DOT));

    // 2. Dibujar líneas VERTICALES
    for (int x = 0; x < sizeWindow.x; x += 100)
    {
        dc.DrawLine(x, 0, x, sizeWindow.y);
    }

    // 3. Dibujar líneas HORIZONTALES
    for (int y = 0; y < sizeWindow.y; y += 100)
    {
        dc.DrawLine(0, y, sizeWindow.x, y);
    }

   Spacecraft* pShuttle = (Spacecraft*)pEnviroment->getAgentByID(2);
   double temperature = pShuttle->_laser.getTemperatureValue();


   /*wxFont fontCabecera(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
   dc.SetFont(fontCabecera);
   dc.SetTextForeground(wxColour(100, 200, 255));*/

   /*char szTemperature[100];
   sprintf(szTemperature, "Temperature: %.2f", temperature);
   dc.DrawText(szTemperature, 10, sizeWindow.y-20);*/


   
   wxPoint center;
   center.x = sizeWindow.x / 2;
   center.y = sizeWindow.y / 2;

   // set the target point where the Shuttle has to go
   pEnviroment->_posTarget.x = center.x-10;
   pEnviroment->_posTarget.y = 0.0;

   // draw logo Metis-core
   if (_logoMetisCore.IsOk())
   {
       dc.DrawBitmap(_logoMetisCore, 0.0, 0.0,true);
   }


   wxAffineMatrix2D matrix = dc.GetTransformMatrix();
   wxAffineMatrix2D backup = matrix;


   matrix.Translate(center.x, center.y);
   //matrix.Rotate(wxDegToRad(180)); // Rotate 45 degrees
   dc.SetTransformMatrix(matrix);


   dc.SetPen(wxPen(wxColour(255, 255, 255), 2, wxPENSTYLE_DOT));
   dc.DrawLine(-center.x, 0, center.x, 0);
   dc.DrawLine(0, -center.y, 0, center.y);

    // dibujar target
    /*dc.SetPen(wxPen(wxColour(255, 0, 0), 1, wxPENSTYLE_DOT));
    dc.DrawCircle(pEnviroment->_posTarget.x, pEnviroment->_posTarget.y, MIN_DISTANCE_TO_TARGET_POSITION);*/
   DrawDeathStar(&dc, pEnviroment->_posTarget.x, pEnviroment->_posTarget.y, 1.0);
    

    int numberSpacecrafts = pEnviroment->getNumAgents();

    for (int i = 0; i < numberSpacecrafts; i++)
    {
        Spacecraft* pCraft = (Spacecraft*)pEnviroment->getAgentByID(i);
        DrawSpacecraft(dc, pCraft);
    }

    //Helicopter* pHeli = (Helicopter*)pEnviroment->GetHelicopter();
    //DrawHelicopter(dc, pHeli);

    dc.SetTransformMatrix(backup);


    drawHistoricalReward(dc);
}

void MyView::drawHistoricalReward(wxDC& dc)
{

    DrawRewardMetricsForTeams(dc, 10, 350);

}


void MyView::OnPaint(wxPaintEvent& event)
{
#ifdef _DEBUG
    wxPaintDC dc(this); // to debug better, drawing inmediate after draw funtions called
    //wxBufferedPaintDC dc(this); // no flicking
#else
    wxBufferedPaintDC dc(this); // no flicking
#endif
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();
    
    int y = 10;
    dc.DrawText(wxString::Format("Episode:%d  iCycle:%d   IsPresent_GPU:%d", _episode, _iCycle, _IsPresent_GPU), 10, y);

#ifndef _DEBUG
    if (!_bDisplayTranning && _doTranning)
    {
        return;
    }
#endif


    Draw(dc, _pSpaceEnviroment);
}
void MyView::StopTraning()
{
    _doTranning = false; // to stop the training
}
void MyView::LoadTraning()
{
    //_pMultiAgentHead->loadIAModel((char*)"imperialConvoy.ai"); // current path o full path name
}
void MyView::SaveIAModel()
{
    
    //_pMultiAgentHead->saveIAModel((char *) "imperialConvoy.ai");

}
void MyView::DisplayTranning(double bViewTraining)
{
    _bDisplayTranning = bViewTraining;
}
void MyView::StartTraningMultiheads()
{


    _IsPresent_GPU = Metis::isCUDAavailable();

    //cargar los archivos ia
    _pTeamBlue->loadIAModel((char*)"blueTeam.ia", _IsPresent_GPU);
    _pTeamRed->loadIAModel((char*)"redTeam.ia", _IsPresent_GPU);

    //build the trainer
    Metis::AlphaGoZeroAsymTrainer A0GAsymTrainer;

    A0GAsymTrainer.setMaterialHeuristicWeight(0.3);  //set heuristic helper to accelerate the training
    A0GAsymTrainer.setCallbackPerStep(this,onStepTraining);
    A0GAsymTrainer.setCallbackEndEpisode(this,onEndEpisode);
    A0GAsymTrainer.training(_pSpaceEnviroment, _pTeamBlue, _pTeamRed, _IsPresent_GPU);
    
}

void MyView::DrawRewardMetricsForTeams(wxDC& dc, int xPos, int startY)
{
    // --- AJUSTE DE FUENTES ---
    wxFont fontCabecera(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    wxFont fontDatos(8, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);

    int w, h;
    this->GetSize(&w, &h);

    int colWidth = 400;
    int xStart = 40;

    int yHeader = h - 70;
    int ySeparator = 15;
    int lineHeight = 18;

    // --- LÍNEA SEPARADORA SUPERIOR ---
    dc.SetPen(wxPen(wxColour(100, 200, 255, 80), 1));
    dc.DrawLine(20, ySeparator, w - 20, ySeparator);

    // --- DIBUJADO DE DATOS, TENDENCIAS Y CABECERAS COORDINADAS ---
    dc.SetFont(fontDatos);
    int dataY = yHeader + lineHeight;

    // 1. EQUIPO AZUL (DRONES) -> Columna 1 (Primero)
    dc.SetFont(fontCabecera);
    dc.SetTextForeground(wxColour(100, 180, 255)); // Cabecera Azul
    dc.DrawText("Team Blue (Drones)", xStart, yHeader);


    dc.SetFont(fontDatos);
    dc.SetTextForeground(wxColour(100, 180, 255)); // Texto Azul
    dc.DrawText(wxString::Format("WinRate: %.1f%%", _blueWinPercent), xStart, dataY);
    DrawSparkline(dc, _historyWinTeamBlue, xStart, dataY + 15, colWidth - 50, 25, wxColour(100, 180, 255));

    // 2. EQUIPO ROJO (SHUTTLE) -> Columna 2 (Segundo)
    dc.SetFont(fontCabecera);
    dc.SetTextForeground(wxColour(255, 100, 100)); // Cabecera Roja
    dc.DrawText("Team Red  (Shuttle)", xStart + colWidth, yHeader);

    dc.SetFont(fontDatos);
    dc.SetTextForeground(wxColour(255, 100, 100)); // Texto Rojo
    dc.DrawText(wxString::Format("WinRate: %.1f%%", _redWinPercent), xStart + colWidth, dataY);
    DrawSparkline(dc, _historyWinTeamRed, xStart + colWidth, dataY + 15, colWidth - 50, 25, wxColour(255, 100, 100));

    // 3. TABLAS (DRAWS) -> Columna 3 (Tercero / Final)
    dc.SetFont(fontCabecera);
    dc.SetTextForeground(wxColour(180, 190, 210)); // Cabecera Gris Neutro
    dc.DrawText("Draws", xStart + (colWidth * 2), yHeader);


    dc.SetFont(fontDatos);
    dc.SetTextForeground(wxColour(180, 190, 210)); // Texto Gris Neutro
    dc.DrawText(wxString::Format("DrawRate: %.1f%%", _drawPercent), xStart + (colWidth * 2), dataY);
    DrawSparkline(dc, _historyWinTablas, xStart + (colWidth * 2), dataY + 15, colWidth - 50, 25, wxColour(180, 190, 210));
}
// --- FUNCIÓN AUXILIAR PARA PINTAR LAS MINI-GRÁFICAS (SPARKELINES) ---
void MyView::DrawSparkline(wxDC& dc, const std::vector<double>& history, int x, int y, int width, int height, wxColour color)
{
    // Dibujar caja de fondo sutil para la gráfica
    dc.SetPen(wxPen(wxColour(40, 40, 60), 1));
    dc.SetBrush(wxBrush(wxColour(15, 15, 25)));
    dc.DrawRectangle(x, y, width, height);

    if (history.size() < 2) return;

    dc.SetPen(wxPen(color, 2));

    double step = (double)width / (double)(history.size() - 1);

    for (size_t i = 1; i < history.size(); ++i)
    {
        // Normalizar los valores asumiendo un rango de 0.0 a 100.0 por ciento
        double val1 = std::max(0.0, std::min(100.0, history[i - 1]));
        double val2 = std::max(0.0, std::min(100.0, history[i]));

        int x1 = x + (int)((i - 1) * step);
        int y1 = (y + height) - (int)((val1 / 100.0) * height);
        int x2 = x + (int)(i * step);
        int y2 = (y + height) - (int)((val2 / 100.0) * height);

        dc.DrawLine(x1, y1, x2, y2);
    }
}
