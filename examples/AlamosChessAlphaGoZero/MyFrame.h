#pragma once
#include <wx/wx.h>
#include <wx/splitter.h>

enum
{
    ID_Hello = 1,
    ID_ViewTranning,
    ID_PlayMenu,
    ID_ViewRealTime,
    ID_StartTranning,
    ID_StopTranning,
    ID_SaveTranning,
    ID_LoadTranning,
    ID_Play_GAME,
    ID_Play_AgentProcedural,
    ID_Test
};

class MyView;
class TelemetryView;

class MyFrame : public wxFrame
{
public:
    MyFrame();

    
    wxMenu* _menuPlayWith;

private:
    void OnHello(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);
    void OnViewTranning(wxCommandEvent& event);
    void OnViewInRealTime(wxCommandEvent& event);
    void OnStartTraning(wxCommandEvent& event);
    void OnStopTraning(wxCommandEvent& event);
    void OnSaveTraning(wxCommandEvent& event);
    void OnLoadTraning(wxCommandEvent& event);
    void OnPlay_Game(wxCommandEvent& event);
    void OnPlay_RuleBasedBot(wxCommandEvent& event);
    void OnTest(wxCommandEvent& event);
    virtual void OnFrameReady();


    MyView* m_MapView;
    
};

