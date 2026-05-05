#include "stdafx.h"
#include "MyFrame.h"
#include "MyView.h"

#define WINDOW_WIDTH  700 //1700  // 600
#define WINDOW_HEIGHT 500 //700 //250



MyFrame::MyFrame()
    : wxFrame(nullptr, wxID_ANY, "Dynamic Pursuit (Police-Thief Scenario)",
        wxDefaultPosition,
        wxSize(1700, 1050), // Tamaño inicial
        wxDEFAULT_FRAME_STYLE & ~(wxRESIZE_BORDER | wxMAXIMIZE_BOX))
{
    wxMenu* menuView = new wxMenu;
    menuView->Append(ID_ViewTranning, "View Tranning", "", wxItemKind::wxITEM_CHECK);
    menuView->Append(ID_ViewRealTime, "View Realtime", "", wxItemKind::wxITEM_CHECK);

    _menuPlayWith = new wxMenu;
    _menuPlayWith->Append(ID_Play_HITL, "Human in the Loop", "", wxItemKind::wxITEM_CHECK);
    _menuPlayWith->Append(ID_Play_AgentProcedural, "Rule-Based Bot", "", wxItemKind::wxITEM_CHECK);

    wxMenu* menuIA = new wxMenu;
    menuIA->Append(ID_StartTranning, "Start Traning");
    menuIA->Append(ID_StopTranning, "Stop Traning");
    menuIA->Append(ID_SaveTranning, "Save Traning");
    menuIA->Append(ID_LoadTranning, "Load Traning");
    //menuIA->Append(wxID_SEPARATOR);


    wxMenuBar* menuBar = new wxMenuBar;
    //menuBar->Append(menuFile, "&File");
    menuBar->Append(menuView, "&View");
    menuBar->Append(_menuPlayWith, "&Play");
    menuBar->Append(menuIA, "&IA");

    SetMenuBar(menuBar);

    CreateStatusBar();
    SetStatusText("Ready");
        
    Bind(wxEVT_MENU, &MyFrame::OnViewTranning, this, ID_ViewTranning);
    Bind(wxEVT_MENU, &MyFrame::OnViewInRealTime, this, ID_ViewRealTime);
    //Bind(wxEVT_MENU, &MyFrame::OnExit, this, wxID_EXIT);
    Bind(wxEVT_MENU, &MyFrame::OnStartTraning, this, ID_StartTranning);
    //Bind(wxEVT_MENU, &MyFrame::OnStopTraning, this, ID_StopTranning);
    //Bind(wxEVT_MENU, &MyFrame::OnSaveTraning, this, ID_SaveTranning);
    Bind(wxEVT_MENU, &MyFrame::OnLoadTraning, this, ID_LoadTranning);
    Bind(wxEVT_MENU, &MyFrame::OnPlay_HITL, this, ID_Play_HITL);
    Bind(wxEVT_MENU, &MyFrame::OnPlay_RuleBasedBot, this, ID_Play_AgentProcedural);
    //Bind(wxEVT_MENU, &MyFrame::OnTest, this, ID_Test);
    // 
    // Set up frame properties
    SetSize(WINDOW_WIDTH, WINDOW_HEIGHT);

    m_MapView = new MyView(this);

    //Maximize(true);
    Show(true);
   

}

void MyFrame::OnStartTraning(wxCommandEvent& event)
{
    /*if (m_MapView->_IsPresent_GPU)
    {
        SetStatusText("Tranning...GPU=yes");
    }
    else
    {
        SetStatusText("Tranning...GPU=no");
    }*/

    m_MapView->StartTraningInterceptionShip();
}

void MyFrame::OnViewTranning(wxCommandEvent& event)
{
    bool bCheck = event.IsChecked();
    m_MapView->DisplayTranning(bCheck);
}
void MyFrame::OnViewInRealTime(wxCommandEvent& event)
{
    bool bCheck = event.IsChecked();
    m_MapView->DisplayInRealTime(bCheck);
}

void MyFrame::OnLoadTraning(wxCommandEvent& event)
{
    m_MapView->loadIAModel();
}

void MyFrame::OnPlay_HITL(wxCommandEvent& event)
{
    if (!m_MapView->isPlaying())
    {
        m_MapView->Play(1);
    }
    m_MapView->_modeBotPlay = 1;
    _menuPlayWith->Check(ID_Play_AgentProcedural, false);
}
void MyFrame::OnPlay_RuleBasedBot(wxCommandEvent& event)
{
    if (!m_MapView->isPlaying())
    {
        m_MapView->Play(2);
    }

    m_MapView->_modeBotPlay = 2;
    _menuPlayWith->Check(ID_Play_HITL, false);
}
