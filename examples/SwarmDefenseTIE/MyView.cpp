//#include "stdafx.h"
#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <wx/docview.h>
#include <wx/dcclient.h>

#include "METIS-Core.h"
#include "Kernel/SpaceEnviroment.h"

#include "MyView.h"
#include "Enviroment.h"
#include "MemoryLeaks.h"
#include <assert.h>
#include <numeric>

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
void onStepTraining(void* pSender, Metis::TMULTIHEADAGENTMETRICS* pMetrics)
{
    MyView::_pSelf->_episode = pMetrics->episode;
    MyView::_pSelf->_iCycle = pMetrics->step;

    pMetrics->bForceStopTraining = !MyView::_pSelf->_doTranning; // in case we want to stop de training
    

    if (MyView::_pSelf->_rewardHistoryShuttle.size() >= 50)
    {
        // Eliminamos el elemento más antiguo (índice 0)
        MyView::_pSelf->_rewardHistoryShuttle.erase(MyView::_pSelf->_rewardHistoryShuttle.begin());
        MyView::_pSelf->_rewardHistoryTIE_1.erase(MyView::_pSelf->_rewardHistoryTIE_1.begin());
        MyView::_pSelf->_rewardHistoryTIE_2.erase(MyView::_pSelf->_rewardHistoryTIE_2.begin());
        
    }

    double r_Shuttle = pMetrics->_headMetric[NAME_SHUTTLE].reward;
    double r_XWing_1 = pMetrics->_headMetric[NAME_TIE1].reward;
    double r_XWing_2 = pMetrics->_headMetric[NAME_TIE2].reward;

    // Añadimos el nuevo dato al final
    MyView::_pSelf->_rewardHistoryShuttle.push_back(r_Shuttle);
    MyView::_pSelf->_rewardHistoryTIE_1.push_back(r_XWing_1);
    MyView::_pSelf->_rewardHistoryTIE_2.push_back(r_XWing_2);


#ifdef _DEBUG
    MyView::_pSelf->drawStep(pSender, pMetrics->episode, pMetrics->step);
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
        MyView::_pSelf->Refresh();  // Mark the window as needing a repaint
        MyView::_pSelf->Update();
    }
}

//callback when the Metis-Core ends a episode
void onEndEpisode(void* pSender, Metis::TMULTIHEADAGENTMETRICS* pMetrics)
{
    MyView::_pSelf->OnEndEpisode(pMetrics);
}
#pragma endregion

void MyView::OnEndEpisode(Metis::TMULTIHEADAGENTMETRICS* pMetrics)
{
    _episode = pMetrics->episode;
    _iCycle = pMetrics->step;

    if ((pMetrics->episode % METRICS_X_EPISODES) == 0)
    {

        double meanShutle_R = calculateAverageReward(_pSpaceEnviroment->_minmaxReward.ShuttleRewardsList);
        double meanXWing1_R = calculateAverageReward(_pSpaceEnviroment->_minmaxReward.TIE_1_RewardsList);
        double meanXWing2_R = calculateAverageReward(_pSpaceEnviroment->_minmaxReward.TIE_2_RewardsList);
        double meanTOTAL_R = calculateAverageReward(_pSpaceEnviroment->_minmaxReward.Total_RewardsList);

        double percentSucess_Shuttle = calculatePercentSuccess(_pSpaceEnviroment->_minmaxReward.minShuttle, _pSpaceEnviroment->_minmaxReward.maxShuttle, meanShutle_R);
        double percentSucess_XWing1 = calculatePercentSuccess(_pSpaceEnviroment->_minmaxReward.minTIE_1, _pSpaceEnviroment->_minmaxReward.maxTIE_1, meanXWing1_R);
        double percentSucess_XWing2 = calculatePercentSuccess(_pSpaceEnviroment->_minmaxReward.minTIE_2, _pSpaceEnviroment->_minmaxReward.maxTIE_2, meanXWing2_R);
        double percentSucess_Total = calculatePercentSuccess(_pSpaceEnviroment->_minmaxReward.minXTotal, _pSpaceEnviroment->_minmaxReward.maxXTotal, meanTOTAL_R);

        _pSpaceEnviroment->_minmaxReward.ShuttleRewardsList.clear();
        _pSpaceEnviroment->_minmaxReward.TIE_1_RewardsList.clear();
        _pSpaceEnviroment->_minmaxReward.TIE_2_RewardsList.clear();
        _pSpaceEnviroment->_minmaxReward.Total_RewardsList.clear();

        if (metrics_fp == NULL)
        {
            metrics_fp = fopen("traningLogConvoyMARL.txt", "w+t");
        }

        fprintf(metrics_fp, "\n========================================================\n");
        fprintf(metrics_fp, "Episode:%d   countSuccesX50episodes:%d  meanRewardTotal:%.3f\n", pMetrics->episode, pMetrics->countSuccesX50episodes, meanTOTAL_R);
        fprintf(metrics_fp, "  meanShutle_R:%.3f    meanXWing1_R:%.3f   meanXWing2_R:%.3f\n", meanShutle_R, meanXWing1_R, meanXWing2_R);
        fprintf(metrics_fp, "\npercentSucess_Total:%.3f\n", percentSucess_Total);
        fprintf(metrics_fp, "   percentSucess_Shuttle:%.3f    percentSucess_XWing1:%.3f   percentSucess_XWing2:%.3f\n", percentSucess_Shuttle, percentSucess_XWing1, percentSucess_XWing2);
                
        

        fprintf(metrics_fp, "Maximos y Minimos\n");
        fprintf(metrics_fp, "       Maximos TARGET:_countMaxSuccessPer50:%d  _maxMeanRewardTotal:%.3f \n", _countMaxSuccessPer50, _maxMeanRewardTotal);
        fprintf(metrics_fp, "  minShuttle:%.3f    maxShuttle:%.3f\n", _pSpaceEnviroment->_minmaxReward.minShuttle, _pSpaceEnviroment->_minmaxReward.maxShuttle);
        fprintf(metrics_fp, "  minXWing_1:%.3f    maxXWing_1:%.3f\n", _pSpaceEnviroment->_minmaxReward.minTIE_1, _pSpaceEnviroment->_minmaxReward.maxTIE_1);
        fprintf(metrics_fp, "  minXWing_2:%.3f    maxXWing_2:%.3f\n", _pSpaceEnviroment->_minmaxReward.minTIE_2, _pSpaceEnviroment->_minmaxReward.maxTIE_1);
        

        std::map<std::string, Metis::THEADMETRIC>::iterator it;

        fprintf(metrics_fp, "   Total Loss Net: %.3f\n", pMetrics->_meanTotalLoss);

        for (it = pMetrics->_headMetric.begin(); it != pMetrics->_headMetric.end(); it++)
        {
            std::string headName = it->first;
            Metis::THEADMETRIC& headMetric = it->second;
            fprintf(metrics_fp, "   headName:%s  _meanLossHead:%.3f   _meanGradient:%.3f\n", headName.c_str(), headMetric._meanLossHead, headMetric._meanGradient);
        }


        if (_countMaxSuccessPer50 < pMetrics->countSuccesX50episodes)
        {
            _countMaxSuccessPer50 = pMetrics->countSuccesX50episodes; // grabar el numero maximo alcanzado en el modelo, y grabmos a fichero

            SaveIAModel();

            fprintf(metrics_fp, "Save IA model: countMaxSuccessPer50:%d   meanRewardTotal:%f\n", MyView::_pSelf->_countMaxSuccessPer50, meanTOTAL_R);
        }
        else
        {
            if (_maxMeanRewardTotal < meanTOTAL_R)
            {
                _maxMeanRewardTotal = meanTOTAL_R;

                SaveIAModel();

                fprintf(metrics_fp, "Save IA model: maxMeanReward:%.2f meanRewardTotal:%f\n", MyView::_pSelf->_maxMeanRewardTotal, meanTOTAL_R);
            }
        }

        fflush(metrics_fp);
    }
}

MyView::MyView(wxFrame* parent)
    : wxPanel(parent) {
    // Initialization code (if needed)

        
    _Shuttle = new Spacecraft();
    _Shuttle->setID(0);
    _Shuttle->setColor(255, 255, 255);
    _Shuttle->setHeading(90.0);
    _Shuttle->_speedMaxSpeed = MAX_SPEED_SHUTTLE; // too lazy to create a method :-(

    _fighter_TIE_1 = new Spacecraft();
    _fighter_TIE_1->setID(1);
    _fighter_TIE_1->setColor(255,255, 0);
    _fighter_TIE_1->setHeading(90.0);
    _fighter_TIE_1->_speedMaxSpeed = MAX_SPEED_FIGHTER_TIE;

    _fighter_TIE_2 = new Spacecraft();
    _fighter_TIE_2->setID(2);
    _fighter_TIE_2->setColor(0, 255, 0);
    _fighter_TIE_2->setHeading(90.0);
    _fighter_TIE_2->_speedMaxSpeed = MAX_SPEED_FIGHTER_TIE;

    _X_Wing = new Spacecraft();
    _X_Wing->setID(3);
    _X_Wing->setColor(255, 255, 255);
    _X_Wing->setHeading(90.0);
    _X_Wing->GoHead();
    _X_Wing->_speedMaxSpeed = MAX_SPEED_X_WINGS;
    //_X_Wing->_speedMaxSpeed = 0; // for testing. set the X-Wing in a fixed position

    

    //_TIE_figther = new SpaceScortAgent();

    Metis::MultiHeadBrainBuilder multiBuilderHeads;

    _IsPresent_GPU = Metis::isCUDAavailable();

    _pMultiAgentHead = multiBuilderHeads.addHead(std::string(NAME_SHUTTLE), _Shuttle)
                                    .addHead(std::string(NAME_TIE1), _fighter_TIE_1)
                                    .addHead(std::string(NAME_TIE2), _fighter_TIE_2)
                                    .setNumberInputsOutputs(CONVOY_INPUTS, ACTIONS::MAX_ACTIONS)
                                    .useGPU(_IsPresent_GPU).build();

    _pSpaceEnviroment = new SpaceEnviroment();

    //spacecraft can access to the enviroment
    _Shuttle->_pEnv = _pSpaceEnviroment;
    _fighter_TIE_1->_pEnv = _pSpaceEnviroment;
    _fighter_TIE_2->_pEnv = _pSpaceEnviroment;
    _X_Wing->_pEnv = _pSpaceEnviroment;
    
    _pSpaceEnviroment->addAgent(_Shuttle);
    _pSpaceEnviroment->addAgent(_fighter_TIE_1);
    _pSpaceEnviroment->addAgent(_fighter_TIE_2);
    _pSpaceEnviroment->addAgent(_X_Wing);

    _pSpaceEnviroment->reset();

    //_pSpaceEnviroment->addAgent(_TIE_figther);

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
    
}

MyView::~MyView()
{

    /*delete _pQTorchNetOnline;
    delete _pQTorchNetTarget;

    delete _pEnviroment;*/

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

    _pSpaceEnviroment->reset();


    //srand(123);

    m_pPlayTimer = new wxTimer(this, wxID_ANY);
    Bind(wxEVT_TIMER, &MyView::OnTimer, this, m_pPlayTimer->GetId());
    // Start the timer with a 1000 ms (1 second) interval
    m_pPlayTimer->Start(TIME_EACH_TICK); // Interval in milliseconds

}
int MyView::ReadFromKeyboard()
{
    int action;

    action = AHEAD;
    if (wxGetKeyState(WXK_LEFT))
    {
        action = LEFT;
    }
    if (wxGetKeyState(WXK_RIGHT))
    {
        action = RIGHT;
    }
    if (wxGetKeyState(WXK_DOWN))
    {
        action = STOP;
    }
    if (wxGetKeyState((wxKeyCode)'f'))
    {
        action = FIRE_MISSILE;
    }

    return action;
}

void MyView::OnTimer(wxTimerEvent& event)
{
    DWORD updateTimerThreadID = GetCurrentThreadId();

    Metis::State spaceState;
    _pSpaceEnviroment->getState((Metis::State*)&spaceState);
    _pSpaceEnviroment->serizalizeState((Metis::State*)&spaceState, &spaceState._stateVector);

    
    Spacecraft* pShuttle = (Spacecraft*)_pSpaceEnviroment->getAgentFromID(0);
    Spacecraft* pTIE_1 = (Spacecraft*)_pSpaceEnviroment->getAgentFromID(1);
    Spacecraft* pTIE_2 = (Spacecraft*)_pSpaceEnviroment->getAgentFromID(2);
    Spacecraft* pX_Wing = (Spacecraft*)_pSpaceEnviroment->getAgentFromID(3);

    int shuttle_Action = _pMultiAgentHead->getBestAction(pShuttle, spaceState);
    int TIE_1_Action = _pMultiAgentHead->getBestAction(pTIE_1, spaceState);
    int TIE_2_Action = _pMultiAgentHead->getBestAction(pTIE_2, spaceState);

    //-----------------------------------------------------------------------------
    // calculate de reward for the action choosen
    TSPACEENVIROMENT *pEnv = (TSPACEENVIROMENT*)spaceState.getUserState();
    double shuttle_r = _pSpaceEnviroment->calculate_rewardShuttle(pEnv);
    if (_rewardHistoryShuttle.size() > 50)
    {
        _rewardHistoryShuttle.erase(_rewardHistoryShuttle.begin());
    }
    _rewardHistoryShuttle.push_back(shuttle_r);
    double TIE1_r = _pSpaceEnviroment->calculate_reward_X_TIE1(pEnv);
    if (_rewardHistoryTIE_1.size() > 50)
    {
        _rewardHistoryTIE_1.erase(_rewardHistoryTIE_1.begin());
    }
    _rewardHistoryTIE_1.push_back(TIE1_r);

    double TIE2_r = _pSpaceEnviroment->calculate_reward_X_TIE2(pEnv);
    if (_rewardHistoryTIE_2.size() > 50)
    {
        _rewardHistoryTIE_2.erase(_rewardHistoryTIE_2.begin());
    }
    _rewardHistoryTIE_2.push_back(TIE2_r);
    //-----------------------------------------------------------------------------

    
    ////-------- seleccion de la accion de la policia y del agente
    int actionXWing;
    if (_modeBotPlay == 1)
        actionXWing = ReadFromKeyboard(); // controla el usuario
    else actionXWing = pX_Wing->getActionProcedural((Metis::State&)spaceState); //// accion procedural


    // aplicamos la accion al entorno
    _pSpaceEnviroment->applyAction(pShuttle, shuttle_Action);
    _pSpaceEnviroment->applyAction(pTIE_1, TIE_1_Action);
    _pSpaceEnviroment->applyAction(pTIE_2, TIE_2_Action);
    
    _pSpaceEnviroment->applyAction(pX_Wing, actionXWing);

    double deltatime = _pSpaceEnviroment->getDeltaTime();
    _pSpaceEnviroment->updatePhysics(deltatime);  // actualizacion de las físicas


    //// --- chequear quien ha ganado
    Metis::Vector2D posShuttle = pShuttle->getPosition();
    Metis::Vector2D posTarget = _pSpaceEnviroment->_posTarget;
    Metis::Vector2D r = posShuttle.DistanceTo(posTarget);
    if (r.magnitude() < 10)
    {
        m_pPlayTimer->Stop();
        delete m_pPlayTimer;
        m_pPlayTimer = NULL;
        _modeBotPlay = -1; // reset the play mode
        wxMessageBox("The Imperial shuttle has arrived at the Death Star!!!. You lose");
    }
    if ( pShuttle->getState() == destroyed )
    {
        m_pPlayTimer->Stop();
        delete m_pPlayTimer;
        m_pPlayTimer = NULL;
        _modeBotPlay = -1; // reset the play mode
        wxMessageBox("The Imperial shuttle destroyed. You win!!");
    }
    if (pX_Wing->getState() == destroyed)
    {
        m_pPlayTimer->Stop();
        delete m_pPlayTimer;
        m_pPlayTimer = NULL;
        _modeBotPlay = -1; // reset the play mode
        wxMessageBox("Rebel X-Wing destroyed. You lose!!");
    }
    


    Refresh();  // Mark the window as needing a repaint
    Update();   // Process the repaint immediately
}
void MyView::drawStep(void* pSender, int episode, int step)
{
    wxClientDC dc(this); // Get the DC for the client area of the window (for debugin, instand paiting)
    
    // Background
    dc.SetBackground(wxBrush(wxColour(10, 20, 30)));
    dc.Clear();

    Draw(dc, _pSpaceEnviroment);
}

void MyView::DrawSpacecraft(wxDC& dc, Spacecraft* pCraft)
{
    //wxSize sizeWindow = dc.GetSize();
    //wxPoint center;
    //center.x = sizeWindow.x / 2;
    //center.y = sizeWindow.y / 2;

    //wxAffineMatrix2D matrix = dc.GetTransformMatrix();
    //wxAffineMatrix2D backup = matrix;


    //matrix.Translate(center.x, center.y);
    ////matrix.Rotate(wxDegToRad(180)); // Rotate 45 degrees
    //dc.SetTransformMatrix(matrix);

    // dibujar ejes
    //dc.DrawLine(wxPoint(-sizeWindow.x, 0), wxPoint(sizeWindow.x, 0));
    //dc.DrawLine(wxPoint(0, -sizeWindow.y), wxPoint(0, sizeWindow.y));

    pCraft->Draw(&dc);

    //dc.SetTransformMatrix(backup);
}

#include <algorithm> // Para std::max

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

    // dibujar target
    /*dc.SetPen(wxPen(wxColour(255, 0, 0), 1, wxPENSTYLE_DOT));
    dc.DrawCircle(pEnviroment->_posTarget.x, pEnviroment->_posTarget.y, MIN_DISTANCE_TO_TARGET_POSITION);*/
   DrawDeathStar(&dc, pEnviroment->_posTarget.x, pEnviroment->_posTarget.y, 1.0);
   
    

    int numberSpacecrafts = pEnviroment->getNumberAgents();

    for (int i = 0; i < numberSpacecrafts; i++)
    {
        Spacecraft* pCraft = (Spacecraft*)pEnviroment->getAgentFromID(i);
        DrawSpacecraft(dc, pCraft);
    }

    //Helicopter* pHeli = (Helicopter*)pEnviroment->GetHelicopter();
    //DrawHelicopter(dc, pHeli);

    dc.SetTransformMatrix(backup);


    drawHistoricalReward(dc);
}

void MyView::drawHistoricalReward(wxDC& dc)
{


    DrawRewardMetricsForAgent(dc, 10, 350);

}
void MyView::DrawRewardMetricsForAgent(wxDC& dc,int xPos, int startY)
{
    // --- AJUSTE DE FUENTES (Más pequeñas para 125px) ---
    wxFont fontCabecera(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    wxFont fontDatos(8, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);

    int w, h;
    this->GetSize(&w, &h);

    int colWidth = 400;
    int xStart = 40;

    int yHeader = h - 60;
    int ySeparator = 15;
    int yStartData = 35;
    int lineHeight = 16;

    // --- CABECERAS COMPACTAS ---
    dc.SetFont(fontCabecera);
    dc.SetTextForeground(wxColour(100, 200, 255));

    dc.DrawText("Shuttle", xStart, yHeader);
    dc.DrawText("TIE 1", xStart + colWidth, yHeader);
    dc.DrawText("TIE 2", xStart + (colWidth * 2), yHeader);

    dc.SetPen(wxPen(wxColour(100, 200, 255, 80), 1));
    dc.DrawLine(20, ySeparator, 1680, ySeparator);

    // --- DIBUJADO DE LAS 3 COLUMNAS ---
    dc.SetFont(fontDatos);

    

    int yStartReward = h - 42; // Inicio fijo del bloque de recompensa
    // --- DIBUJADO DE COLUMNAS (Delegación) ---
    for (int shipIdx = 0; shipIdx < 3; ++shipIdx)
    {
        int xPos = xStart + (shipIdx * colWidth);
        Spacecraft* pShip = (Spacecraft *)_pSpaceEnviroment->getAgentFromID(shipIdx);

        if (pShip)
        {
            std::vector<double>* pHistoryReward=NULL;
            if (shipIdx == 0)
                pHistoryReward = &_rewardHistoryShuttle;
            if (shipIdx == 1)
                pHistoryReward = &_rewardHistoryTIE_1;
            if (shipIdx == 2)
                pHistoryReward = &_rewardHistoryTIE_2;

            // 2. Pintar la zona de recompensas en la base del panel
            DrawRewardMetricsForShip(dc, pShip, xPos, yStartReward, pHistoryReward);
        }
    }
}
void MyView::DrawRewardMetricsForShip(wxDC& dc, Spacecraft* pShip, int xPos, int startY, std::vector<double>* pHistoryReward)
{
    // 1. Configuración de la "Caja" de la gráfica
    int graphWidth = 300;
    int graphHeight = 40; // Altura total de la gráfica
    int centerY = startY + (graphHeight / 2); // Línea del 0.0

    // Dibujar el marco de la gráfica (estilo radar)
    dc.SetPen(wxPen(wxColour(60, 80, 100, 150), 1));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(xPos, startY, graphWidth, graphHeight);

    // Línea de referencia del cero (Eje X)
    dc.SetPen(wxPen(wxColour(100, 100, 100, 100), 1, wxPENSTYLE_DOT));
    dc.DrawLine(xPos, centerY, xPos + graphWidth, centerY);

    // 2. Obtener los datos (el histórico de recompensas del barco)
    // Supongamos que pShip->_rewardHistory tiene los últimos valores [-1, 1]

    std::vector<double>& history = *pHistoryReward;
    if (history.size() < 2) return;

    // 3. Dibujar la línea de la gráfica f(t) = r
    dc.SetPen(wxPen(wxColour(0, 255, 255), 1)); // Cyan para la línea de datos

    int numPoints = history.size();
    double stepX = (double)graphWidth / (double)(numPoints - 1);

    for (size_t i = 0; i < history.size() - 1; ++i)
    {
        // Mapeo: 
        // +1.0 debe ser la parte superior (startY)
        // -1.0 debe ser la parte inferior (startY + graphHeight)
        // El centro (0.0) es centerY.

        // Invertimos el signo porque en pantalla, restar sube la coordenada
        int y1 = centerY - (int)(history[i] * (graphHeight / 2.0));
        int y2 = centerY - (int)(history[i + 1] * (graphHeight / 2.0));

        int x1 = xPos + (int)(i * stepX);
        int x2 = xPos + (int)((i + 1) * stepX);

        dc.DrawLine(x1, y1, x2, y2);
    }

    // 4. Etiqueta de valor actual
    dc.SetFont(wxFont(7, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    dc.SetTextForeground(wxColour(200, 200, 200));
    dc.DrawText("REWARD HISTORY (t)", xPos, startY + graphHeight + 2);

    double lastR = history.back();
    dc.SetTextForeground(lastR >= 0 ? wxColour(170, 255, 0) : wxColour(255, 0, 0));
    dc.DrawText(wxString::Format("%+.2f", lastR), xPos + graphWidth - 35, startY - 12);
}

void MyView::OnPaint(wxPaintEvent& event)
{
#ifdef _DEBUG
    //wxPaintDC dc(this); // to debug better, drawing inmediate after draw funtions called
    wxBufferedPaintDC dc(this); // no flicking
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
    _pMultiAgentHead->loadIAModel((char*)"imperialConvoy.ai"); // current path o full path name
}
void MyView::SaveIAModel()
{
    
    _pMultiAgentHead->saveIAModel((char *) "imperialConvoy.ai");

}
void MyView::DisplayTranning(double bViewTraining)
{
    _bDisplayTranning = bViewTraining;
}
void MyView::StartTraningMultiheads()
{
    Metis::MultiHeadsAgentTrainerDQN multiHeadsTrainer;

    _doTranning = true;

    _X_Wing->setState(attacked); // set state attack so X-wing attack the imperial convoy

    multiHeadsTrainer.setCallbackPerStep(onStepTraining);
    multiHeadsTrainer.setCallbackEndEpisode(onEndEpisode);
    multiHeadsTrainer.training(_pSpaceEnviroment, _pMultiAgentHead, _X_Wing);
}