#pragma once

#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/wx.h>
#include <wx/timer.h>

#include <map>
#include <vector>

#include "Kernel/Spacecraft.h"
#include "Kernel/SpaceEnviroment.h"

class MyView : public wxPanel
{
public:
    MyView(wxFrame* parent);
    ~MyView();
    void OnPaint(wxPaintEvent& event);
    virtual void OnSize(wxSizeEvent& event);
    void OnTimer(wxTimerEvent& event);

    void DrawPlanet(wxDC* pDC);
    void DrawDeathStar(wxDC* pDC, int cx, int cy, float scale);
    void Draw(wxDC& dc, SpaceEnviroment* pEnviroment);
    void DrawSpacecraft(wxDC& dc, Spacecraft* pCraft);
    void drawStep(void* pSender, int episode, int step);
    void drawHistoricalReward(wxDC& dc);
    void DrawRewardMetricsForAgent(wxDC& dc, int xPos, int startY);
    void DrawRewardMetricsForShip(wxDC& dc, Spacecraft* pShip, int xPos, int startY, std::vector<double>* pHistoryReward);

    void StopTraning();
    void LoadTraning();
    void SaveIAModel();
    void StartTraningMultiheads();
    void Play(int modeBot);
    int  ReadFromKeyboard();
    bool isPlaying();

    void DisplayTranning(double bViewTraining);

    void OnEndEpisode(Metis::TMULTIHEADAGENTMETRICS* pMetrics);
    void writeMetricsTranningToDisk(Metis::TMULTIHEADAGENTMETRICS* pMetrics);

    int _modeBotPlay;
    bool _IsPresent_GPU;

    Spacecraft* _Shuttle;
    Spacecraft* _fighter_TIE_1;
    Spacecraft* _fighter_TIE_2;
    Spacecraft* _X_Wing;
        
    //SpaceScortAgent* _TIE_figther;

    SpaceEnviroment* _pSpaceEnviroment;

    Metis::MultiHeadAgent* _pMultiAgentHead;

    //int _episodeNumber;
    int _iCycle;

    wxTimer* m_pPlayTimer;       // Timer update the physics
    static MyView* _pSelf;

    //metrics
    int _countMaxSuccessPer50;
    int _episode;
    float _maxMeanRewardTotal;
    std::vector<double> _rewardHistoryShuttle;
    std::vector<double> _rewardHistoryTIE_1;
    std::vector<double> _rewardHistoryTIE_2;

    bool _bDisplayInRealTime;
    bool _bDisplayTranning;
    bool _doTranning;
    
    wxImage _logoMetisCore;

    
private:
        

private:
    wxDECLARE_EVENT_TABLE();
};

