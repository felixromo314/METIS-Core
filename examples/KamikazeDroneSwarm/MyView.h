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

    
    void DrawDeathStar(wxDC* pDC, int cx, int cy, float scale);
    void Draw(wxDC& dc, SpaceEnviroment* pEnviroment);
    void DrawSpacecraft(wxDC& dc, Spacecraft* pCraft);
    void drawStep(void* pSender, int episode, int step, SpaceEnviroment* pEnv);
    void drawHistoricalReward(wxDC& dc);
    void DrawRewardMetricsForTeams(wxDC& dc, int xPos, int startY);
    void DrawSparkline(wxDC& dc, const std::vector<double>& history, int x, int y, int width, int height, wxColour color);

    void StopTraning();
    void LoadTraning();
    void SaveIAModel();
    void StartTraningMultiheads();
    void Play(int modeBot);
    int  ReadFromKeyboard();
    bool isPlaying();

    void DisplayTranning(double bViewTraining);

    void OnEndEpisode(Metis::TSEARCHACTIONASYM* pMetrics);

    int _modeBotPlay;
    bool _IsPresent_GPU;


    Spacecraft* _drone_XWing_1;
    Spacecraft* _drone_XWing_2;
    Metis::MultiHeadAgent* _pTeamBlue;

    Spacecraft* _ImperialShuttle;
    Metis::MultiHeadAgent* _pTeamRed;
    
    IAgent* _teamBluePlayAgent;
        
    //SpaceScortAgent* _TIE_figther;

    SpaceEnviroment* _pSpaceEnviroment;

    

    //int _episodeNumber;
    int _iCycle;

    wxTimer* m_pPlayTimer;       // Timer update the physics
    static MyView* _pSelf;

    //metrics
    int _countMaxSuccessPer50;
    int _episode;
    float _maxMeanRewardTotal;
    std::vector<double> _policyLossHistoryShuttle;
    std::vector<double> _policyLossHistoryDrone_1;
    std::vector<double> _policyLossHistoryDrone_2;

    std::vector<double> _valueLossTeamBlue;
    std::vector<double> _valueLossTeamRed;

    std::vector<double> _MeanTeamBlue_expectedValues;
    std::vector<double> _MeanTeamRed_expectedValues;

    std::vector<double> _teamBlue_expectedValues;
    std::vector<double> _teamRed_expectedValues;
    

    std::map<int, int> _countWinner;
    
    std::vector<double> _historyWinTeamBlue; //drone
    std::vector<double> _historyWinTeamRed; //shuttle
    std::vector<double> _historyWinTablas;
    float _redWinPercent;
    float _blueWinPercent;
    float _drawPercent;


    bool _bDisplayInRealTime;
    bool _bDisplayTranning;
    bool _doTranning;
    
    wxImage _logoMetisCore;

    
private:
        

private:
    wxDECLARE_EVENT_TABLE();
};

