#pragma once
#include <wx/wx.h>
#include <wx/splitter.h>
#include <wx/frame.h>


class MyView;
class TelemetryView;

enum
{
    ID_ViewTranning=1,
    ID_ViewRealTime,
    ID_StartTranning,
    ID_StopTranning,
    ID_SaveTranning,
    ID_LoadTranning,
    ID_Play_HITL,
    ID_Play_AgentProcedural,
    ID_Test
};


class MyFrame : public wxFrame
{
public:
    MyFrame();
       
    void OnStartTraning(wxCommandEvent& event);
    void OnViewTranning(wxCommandEvent& event);
    void OnViewInRealTime(wxCommandEvent& event);

    void OnLoadTraning(wxCommandEvent& event);

    void OnPlay_HITL(wxCommandEvent& event);
    void OnPlay_RuleBasedBot(wxCommandEvent& event);

private:

    wxMenu* _menuPlayWith;
    MyView* m_MapView;
    
};

