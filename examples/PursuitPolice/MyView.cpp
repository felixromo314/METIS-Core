#include "stdafx.h"
#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <wx/docview.h>
#include <wx/dcclient.h>

#include "MyView.h"
//#include "MemoryLeaks.h"
#include <assert.h>

#include "METIS-Core.h"

#include "Kernel\Car.h"
#include "Kernel\UrbanEnviroment.h"

#include <numeric>

using namespace std;

class M_InterceptorTrainer;

#define TIME_EACH_TICK 100.0

wxBEGIN_EVENT_TABLE(MyView, wxPanel)
EVT_PAINT(MyView::OnPaint)
EVT_TIMER(wxID_HIGHEST, MyView::OnTimer)   //Receive Timer event declaration
wxEND_EVENT_TABLE()

MyView* MyView::_pSelf=NULL;
static FILE* metrics_fp = NULL;

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

void writeMetricsTranningToDisk(Metis::TAGENTMETRICS* pMetrics)
{
    if (metrics_fp == NULL)
    {
        metrics_fp = fopen("traningLog.txt", "w+t");
    }

    double meanReward = calculateAverageReward(MyView::_pSelf->_rewardHistory);

    fprintf(metrics_fp, "\n========================================================\n");
    fprintf(metrics_fp, "Episode:%d\n", pMetrics->episode);
    fprintf(metrics_fp, "   meanLoss:%.3f _backbone_norm:%.2f   _gradient_norm:%.2f\n", pMetrics->_meanLoss, pMetrics->_backbone_norm, pMetrics->_gradient_norm);
    fprintf(metrics_fp, "   countSuccesX50episodes:%d   meanReward:%.2f\n", pMetrics->countSuccesX50episodes, meanReward);

    fflush(metrics_fp);
}

void onEndEpisode(void* pSender, Metis::TAGENTMETRICS* pMetrics)
{

    MyView::_pSelf->_episode = pMetrics->episode;
    MyView::_pSelf->_iCycle = pMetrics->step;

    if ((pMetrics->episode % 50) == 0)
    {
        writeMetricsTranningToDisk(pMetrics);

        if (MyView::_pSelf->_countMaxSuccessPer50 < pMetrics->countSuccesX50episodes)
        {
            MyView::_pSelf->_countMaxSuccessPer50 = pMetrics->countSuccesX50episodes; // grabar el numero maximo alcanzado en el modelo, y grabmos a fichero
            
            MyView::_pSelf->saveIAModel();

            fprintf(metrics_fp, "Grabado Modelo con _countMaxSuccessPer50:%d\n", MyView::_pSelf->_countMaxSuccessPer50);
        }
        else
        {
            double meanReward = calculateAverageReward(MyView::_pSelf->_rewardHistory);
            if (MyView::_pSelf->_maxMeanReward < meanReward)
            {
                MyView::_pSelf->_maxMeanReward = meanReward;

                MyView::_pSelf->saveIAModel();

                fprintf(metrics_fp, "Grabado Modelo con _maxMeanReward:%.2f\n", MyView::_pSelf->_maxMeanReward);
            }
        }

    }
}
void onStepTraining(void* pSender, Metis::TAGENTMETRICS *pMetrics)
{
    MyView::_pSelf->_episode = pMetrics->episode;
    MyView::_pSelf->_iCycle = pMetrics->step;

    if (MyView::_pSelf->_rewardHistory.size() >= 100)
    {
        // Eliminamos el elemento más antiguo (índice 0)
        MyView::_pSelf->_rewardHistory.erase(MyView::_pSelf->_rewardHistory.begin());
    }
    // Añadimos el nuevo dato al final
    MyView::_pSelf->_rewardHistory.push_back(pMetrics->reward);


#ifdef _DEBUG
    MyView::_pSelf->drawStep(pSender, pMetrics->episode, pMetrics->step);
#else
    if (MyView::_pSelf->_bDisplayInRealTime)
    {
        wxMilliSleep(100);
    }
    if (MyView::_pSelf->_bDisplayTranning)
    {
        MyView::_pSelf->drawStep(pSender, pMetrics->episode, pMetrics->step);
    }
#endif
    
    if ((pMetrics->step % 10) == 0)
    {
        wxTheApp->Yield();
        MyView::_pSelf->Refresh();  // Mark the window as needing a repaint
        MyView::_pSelf->Update();
    }
    
}


MyView::MyView(wxFrame* parent)
    : wxPanel(parent) {
    // Initialization code (if needed)

    _modeBotPlay = -1;
    _pUrbanEnv = new UrbanEnviroment();
    
    _policeCar = new Car();
    _thiefCar = new Car();

    _policeCar->setID(0);
    _policeCar->setIniSpeed(INTERCEPTOR_SHIP_VELOCITY);

    _thiefCar->setID(1);
    _thiefCar->setIniSpeed(ENEMY_SHIP_VELOCITY);

    _policeCar->setColor(255,255,255);
    _thiefCar->setColor(255,0,0);

    _policeCar->createBrain(NUM_INPUTS, NUM_ACTIONS);


    _pUrbanEnv->addAgent(_policeCar);
    _pUrbanEnv->addAgent(_thiefCar);

    _pUrbanEnv->reset();

    _pSelf = this;
    _bDisplayTranning = false;
    _bDisplayInRealTime = false;
    _doTranning = false;
    _episode = 0;
    _IsPresent_GPU = false;

    _countMaxSuccessPer50 = 0;
    _maxMeanReward = 0.0;
    
    m_pTimer = new wxTimer(this, wxID_ANY);
    Bind(wxEVT_TIMER, &MyView::OnTimer, this, m_pTimer->GetId());
    // Start the timer with a 1000 ms (1 second) interval
    //m_pTimer->Start(TIME_EACH_TICK); // Interval in milliseconds

}

MyView::~MyView()
{

    
}

void MyView::OnPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this); // Hace todo el trabajo sucio por ti

    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();


    // 4. Dibujar todo usando 'bdc' en lugar de 'dc'
    int y = 10;
    dc.DrawText(wxString::Format("Episode:%d  iCycle:%d   IsPresent_GPU:%d", MyView::_pSelf->_episode, MyView::_pSelf->_iCycle, _IsPresent_GPU), 10, y);

#ifndef _DEBUG
    if (!_bDisplayTranning && _doTranning)
    {
        return;
    }
#endif

    // Llamar a tu función Draw pasando el bdc
    draw(dc, _pUrbanEnv);
}

void MyView::drawStep(void* pSender, int episode, int step)
{
    wxClientDC dc(this); // Get the DC for the client area of the window

    draw(dc, _pUrbanEnv);
}

void MyView::drawShip(wxDC& dc, Car* pShip)
{
    wxSize sizeWindow = dc.GetSize();
    wxPoint center;
    center.x = sizeWindow.x / 2;
    center.y = sizeWindow.y / 2;

    wxAffineMatrix2D matrix = dc.GetTransformMatrix();
    wxAffineMatrix2D backup = matrix;


    matrix.Translate(center.x, center.y);
    //matrix.Rotate(wxDegToRad(180)); // Rotate 45 degrees
    dc.SetTransformMatrix(matrix);

    // dibujar ejes
    //dc.DrawLine(wxPoint(-sizeWindow.x, 0), wxPoint(sizeWindow.x, 0));
    //dc.DrawLine(wxPoint(0, -sizeWindow.y), wxPoint(0, sizeWindow.y));

    pShip->draw(&dc);

    //dc.DrawCircle(_pOceanEnv->_interceptPoint.x, _pOceanEnv->_interceptPoint.y, 5.0);

    dc.SetTransformMatrix(backup);
}


void MyView::draw(wxDC& dc, UrbanEnviroment* pEnviroment)
{
    //-- dibujar fondo
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

    dc.DrawRectangle(655, 0, 5, 500); // dibuar el target de ladron



    std::list<Metis::IAgent *> agents;
    pEnviroment->getAgents(&agents);
    
    std::list<Metis::IAgent*>::iterator it;
    for (it = agents.begin(); it != agents.end(); it++)
    {
        Car* pShip = (Car*)*it;
        drawShip(dc, pShip);
    }


    

    drawHistoricalReward(dc, pEnviroment);
    

}

void MyView::DrawRewardMetricsForShip(wxDC& dc, Car* pShip, int xPos, int startY)
{
    // 1. Configuración de la "Caja" de la gráfica
    int graphWidth = 650;
    int graphHeight = 50; // Altura total de la gráfica
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
    const std::vector<double>& history = _rewardHistory;
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

void MyView::saveIAModel()
{
    _policeCar->saveIAModel((char *)"carModel.ia");
}
void MyView::loadIAModel()
{
    // copy from .\PursuitPolice\IAmodel\
    // <yourdirectgor>\PursuitPolice\x64\Debug\carModel.ia
    // <yourdirectgor>\PursuitPolice\x64\Release\carModel.ia
    _policeCar->loadIAModel((char*)"carModel.ia"); // current path o full path name

}

void MyView::drawHistoricalReward(wxDC& dc, UrbanEnviroment* pEnviroment)
{


    DrawRewardMetricsForShip(dc, NULL, 10, 350);

}
void MyView::DisplayTranning(bool bDisplayTranning)
{
    _bDisplayTranning = bDisplayTranning;
}
void MyView::DisplayInRealTime(bool bDispalyInRealTime)
{
    _bDisplayInRealTime = bDispalyInRealTime;
}


void MyView::StartTraningInterceptionShip()
{
    
    Metis::AgentTrainerDQN agentTrainer;

    _doTranning = true;
    

    agentTrainer.setCallbackPerStep(onStepTraining);
    agentTrainer.setCallbackEndEpisode(onEndEpisode);
    
    agentTrainer.training(_pUrbanEnv, _policeCar, _thiefCar);
    

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

    _pUrbanEnv->reset();


    //srand(123);

    m_pTimer = new wxTimer(this, wxID_ANY);
    Bind(wxEVT_TIMER, &MyView::OnTimer, this, m_pTimer->GetId());
    // Start the timer with a 1000 ms (1 second) interval
    m_pTimer->Start(TIME_EACH_TICK); // Interval in milliseconds
 
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
        action  = RIGHT;
    }
    if (wxGetKeyState(WXK_DOWN))
    {
        action = STOP;
    }
    

    return action;
}

void MyView::OnTimer(wxTimerEvent& event)
{

    DWORD updateTimerThreadID = GetCurrentThreadId();

    Metis::State urbanState;
    _pUrbanEnv->getState((Metis::State*) &urbanState);
    _pUrbanEnv->serizalizeState((Metis::State*)&urbanState, &urbanState._stateVector);
    

    Car *pPolice = (Car*)_pUrbanEnv->getAgentFromID(0);
    Car* pThief = (Car*) _pUrbanEnv->getAgentFromID(1);

    //-------- seleccion de la accion de la policia y del agente
    int actionThief;
    if (_modeBotPlay == 1)
        actionThief = ReadFromKeyboard(); // controla el usuario
    else actionThief = pThief->getActionProcedural((Metis::State&) urbanState); //// accion procedural

    int actionPolice = pPolice->predictAction((Metis::State&)urbanState); // la red predice la mejor accion segun el estado del enviroment
    //------------------------------
        
    // aplicamos la accion al entorno
    _pUrbanEnv->applyAction(pPolice, actionPolice);
    _pUrbanEnv->applyAction(pThief, actionThief);

        
    _pUrbanEnv->updatePhysics(INC_TIME);  // actualizacion de las físicas

    
    // --- chequear quien ha ganado
    Metis::Vector2D posPolice = pPolice->getPosition();
    Metis::Vector2D posThief = pThief->getPosition();
    Metis::Vector2D r = posPolice.DistanceTo(posThief);
    if (r.magnitude() < 10)
    {
        m_pTimer->Stop();
        delete m_pTimer;
        _modeBotPlay = -1;
        wxMessageBox("Thief caught!!!!!");
    }

    if (posThief.x > 350)
    {
        m_pTimer->Stop();
        delete m_pTimer;
        _modeBotPlay = -1;
        wxMessageBox("The thief reaches the target!!!!");
    }
    

    Refresh();  // Mark the window as needing a repaint
    Update();   // Process the repaint immediately

}
