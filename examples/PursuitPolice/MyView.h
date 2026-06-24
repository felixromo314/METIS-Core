#pragma once

#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/wx.h>
#include <wx/timer.h>

#include <map>
#include <vector>

#include "Kernel/UrbanEnviroment.h"
#include "Kernel/Car.h"


class MyView : public wxPanel
{
public:
    MyView(wxFrame* parent);
    ~MyView();
    void OnPaint(wxPaintEvent& event);
    void OnTimer(wxTimerEvent& event);
    void DisplayTranning(bool bDisplayTranning);
    void DisplayInRealTime(bool bDispalyInRealTime);

    void drawHistoricalReward(wxDC& dc, UrbanEnviroment *pEnviroment);
    void DrawRewardMetricsForShip(wxDC& dc, Car* pShip, int xPos, int startY, std::vector<double>* pHistoryReward);
    int ReadFromKeyboard();

    void StartTraningInterceptionShip();

    void saveIAModel();
    void loadIAModel();

    bool isPlaying();
    void Play(int modeBot);


    UrbanEnviroment* _pUrbanEnv;

    Car* _policeCar;
    Car* _thiefCar;

    void drawShip(wxDC& dc, Car* pShip);
    void drawStep(void *pSender,int episode,int step);
    void draw(wxDC& dc, UrbanEnviroment* pEnviroment);

    static MyView* _pSelf;

    bool _bDisplayInRealTime;
    bool _bDisplayTranning;
    bool _doTranning;
    wxTimer* m_pTimer;       // Timer update the physics

    int _episode;
    int _iCycle;
    int _countMaxSuccessPer50; // Numero maximo de existo de intercepcion por cada 50 episodios
    double _maxMeanReward;
    bool _IsPresent_GPU;

    int _modeBotPlay; //controla el modo del agente sparring, si humano o bot

    std::vector<double> _rewardHistory;

private:
    wxDECLARE_EVENT_TABLE();
};

