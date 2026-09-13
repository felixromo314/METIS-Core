#pragma once

#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/wx.h>
#include <wx/timer.h>
#include <wx/wx.h>

#include <map>
#include <vector>
#include <deque>


#define WORLD_MIN -500.0f
#define WORLD_MAX +500.0f

class Board;
class Player;
class Figure;

class MyView : public wxPanel
{
public:
    MyView(wxWindow* parent, wxWindowID id, const int* args);
    ~MyView();
    void OnPaint(wxPaintEvent& event);
    virtual void OnSize(wxSizeEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnMouseUp(wxMouseEvent& event);


    void DrawPlanet(wxDC* pDC);
    void DrawDeathStar(wxDC* pDC, int cx, int cy, float scale);
    //void Draw(wxDC& dc, SpaceEnviroment* pEnviroment);
    //void DrawSpacecraft(wxDC& dc, Spacecraft* pCraft);
    void drawStep(void* pSender, Metis::TSEARCHACTION* pSearchAction);
    void drawHistoricalReward(wxDC& dc);
    void DrawRewardMetricsForAgent(wxDC& dc, int xPos, int startY);
    void Draw(wxDC &dc);
    void Draw(wxDC& dc, Board* pBoard, Metis::TSEARCHACTION* pSearchAction);
    void drawPiece(wxDC& dc,int x,int y,Figure *pFigure);
    wxBrush GetBrushForSquare(bool isLight);
    //void DrawRewardMetricsForShip(wxDC& dc, Spacecraft* pShip, int xPos, int startY, std::vector<double>* pHistoryReward);

    void StopTraning();
    void LoadTraning();
    void SaveIAModel();
    void StartTraningAZG();
    void Play(int modeBot);
    bool isPlaying();

    void DisplayTranning(double bViewTraining);
    void DisplayInRealTime(double bViewTraining);

    void OnEndEpisode(Metis::TMULTIHEADAGENTMETRICS* pMetrics);
    void writeMetricsTranningToDisk(Metis::TMULTIHEADAGENTMETRICS* pMetrics);

    bool _IsPresent_GPU;

    

    //int _episodeNumber;
    int _iCycle;
    double _minTotalLoss;
    int    _minNumStepXEpisode;
    int _countPlayerBlueWin;
    int _maxPlayerBlueWin;
    double _winRateModel;
    double _winRateModelMaximum;
    int countWhoWin[4];

    wxTimer* m_pPlayTimer;       // Timer update the physics
    static MyView* _pSelf;

    bool _viewTraning;
    bool _realTime;
    bool _stopTraining;

    //metrics
    wxImage _logoMetisCore;
    wxImage _whites;
    wxImage _blacks;

    //Domain Problem
    Board* _pBoard;
    Player* _pWhites;
    Player* _pBlacks;
    Metis::AlphaGoZeroTrainer *_AGZTrainer;

    Metis::IAgent* _AZGPlayer;
    bool _isPlaying;
    int _turnPlay;
    void processSquareClick(int r, int c);
    bool _isDragging = true;
    int _dragSourceI; // guardamos 'i' (Eje X)
    int _dragSourceJ; // guardamos 'j' (Eje Y)
    wxPoint _dragPos;
    bool pixelToBoard(int xPixel, int yPixel, int& outI, int& outJ);
    void processDragAndDrop(int fromI, int fromJ, int toI, int toJ);

    
private:
    
    

private:
    wxDECLARE_EVENT_TABLE();
};

