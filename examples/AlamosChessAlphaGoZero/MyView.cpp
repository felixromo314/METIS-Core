#include "stdafx.h"
#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <wx/docview.h>
#include <wx/dcclient.h>

#include "MyView.h"
#include <assert.h>
#include <numeric>

#include "Kernel/Board.h"
#include "Kernel/Player.h"
#include "Kernel/Figure.h"


#include "MemoryLeaks.h"


#define HUMAN_TURN 0
#define IA_TURN 1
#define GAME_OVER 10 


#define TIME_EACH_TICK 33.0
#define DELTA_TIME 0.1  // you can increase to 0.1 to accelarte the tranning

wxBEGIN_EVENT_TABLE(MyView, wxPanel)
EVT_PAINT(MyView::OnPaint)
EVT_KEY_DOWN(OnKeyDown)
EVT_LEFT_DOWN(MyView::OnMouseDown)
EVT_MOTION(MyView::OnMouseMove)
EVT_LEFT_UP(MyView::OnMouseUp)
wxEND_EVENT_TABLE()


std::deque<int> recent_results;
const int WINDOW_SIZE_WINS = 100; // Evaluamos los últimos 100 episodios
std::deque<float> last_valueLoss;
std::deque<float> last_policyLoss;

MyView *MyView::_pSelf = NULL;
static FILE* metrics_fp = NULL;


void DebugPrint(const char* format, ...)
{
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    OutputDebugStringA(buffer);
}

#pragma region  CALLBACKS_OF_TRAINING

void callbackSearchMovement(void* pSender, Metis::TSEARCHACTION* pSearchAction)
{
    static FILE* metrics_fp = NULL;
    if (metrics_fp == NULL)
    {
        metrics_fp = fopen("traning_AZG_alamosChess.txt", "w+t");
    }

    if (pSearchAction->bFinishEpisode)
    {
        int success = 0;
        if (pSearchAction->playerID == 0)
        {
            success = 1; // ganaron blancas
        }
        recent_results.push_back(success);
        if (recent_results.size() > WINDOW_SIZE_WINS) 
        {
            recent_results.pop_front();
        }

        last_valueLoss.push_back(pSearchAction->valueLoss);
        if (last_valueLoss.size() > WINDOW_SIZE_WINS)
        {
            last_valueLoss.pop_front();
        }

        last_policyLoss.push_back(pSearchAction->polictyLoss);
        if (last_policyLoss.size() > WINDOW_SIZE_WINS)
        {
            last_policyLoss.pop_front();
        }
        

        
        if (recent_results.size() == WINDOW_SIZE_WINS)
        {
            
            int wins = std::accumulate(recent_results.begin(), recent_results.end(), 0);
            float absolute_win_rate = static_cast<float>(wins) / WINDOW_SIZE_WINS;

            float total_Valueloss = std::accumulate(last_valueLoss.begin(), last_valueLoss.end(), 0.0f);
            float meanValueLoss = static_cast<float>(total_Valueloss) / (float) WINDOW_SIZE_WINS;

            float total_PolicyLoss = std::accumulate(last_policyLoss.begin(), last_policyLoss.end(), 0.0f);
            float meanPolictyLoss = static_cast<float>(total_PolicyLoss) / (float)WINDOW_SIZE_WINS;

            

            double ratioPercent = (absolute_win_rate * 100.0);
            fprintf(metrics_fp, "Tasa de éxito: %.2f % ratio meanPolictyLoss:%f  meanLossValue:%f  valueLoss:%f\n", ratioPercent, meanPolictyLoss, meanValueLoss, pSearchAction->valueLoss);

            // If White achieves the goal 90% of the times at this level, we level up
            if (absolute_win_rate >= 0.90f && pSearchAction->valueLoss < 0.1)
            {
                fprintf(metrics_fp, "   [METIS-CORE] Nivel superado! Tasa de éxito: %.2f % ratio   \n", ratioPercent);
                // reset
                recent_results.clear();
                last_valueLoss.clear();
                last_policyLoss.clear();
                
            }
        }
        fflush(metrics_fp);
    }

    if (pSearchAction->bFinishEpisode)
    {


        if (pSearchAction->playerID == 0)
        {
            MyView::_pSelf->_countPlayerBlueWin++;
        }

        fprintf(metrics_fp, "\n========================================================\n");
        fprintf(metrics_fp, "Episode:%d   NumStep:%d\n", pSearchAction->episode, pSearchAction->step);

        double totalLoss = pSearchAction->polictyLoss + pSearchAction->valueLoss;
        MyView::_pSelf->_minTotalLoss = totalLoss;
        if (pSearchAction->step < MyView::_pSelf->_minNumStepXEpisode)
        {
            if ((pSearchAction->playerID == 0) || (pSearchAction->playerID == 1)) // si gano blancoas o negras, no se cuenta si en tablas
            {
                MyView::_pSelf->_minNumStepXEpisode = pSearchAction->step;
            }
        }


        if ((pSearchAction->episode % 50) == 0)
        {
            if (MyView::_pSelf->_countPlayerBlueWin > MyView::_pSelf->_maxPlayerBlueWin)
            {
                MyView::_pSelf->_maxPlayerBlueWin = MyView::_pSelf->_countPlayerBlueWin;
            }
            MyView::_pSelf->_countPlayerBlueWin = 0;
        }

        if (pSearchAction->winRateModel >= MyView::_pSelf->_winRateModelMaximum)
        {
            MyView::_pSelf->_winRateModelMaximum = pSearchAction->winRateModel;
            MyView::_pSelf->SaveIAModel();
        }


        int whoWinTmp = pSearchAction->playerID;
        if (whoWinTmp == -1)
            whoWinTmp = 4;
        MyView::_pSelf->countWhoWin[whoWinTmp]++;

        MyView::_pSelf->_winRateModel = pSearchAction->winRateModel;
        fprintf(metrics_fp, "_minTotalLoss:%.3f   _minNumStepXEpisode:%d\n", MyView::_pSelf->_minTotalLoss, MyView::_pSelf->_minNumStepXEpisode);
        fprintf(metrics_fp, "       _countPlayerBlueWin:%d\n", MyView::_pSelf->_countPlayerBlueWin);
        fprintf(metrics_fp, "       _maxPlayerBlueWin:%d\n", MyView::_pSelf->_maxPlayerBlueWin);
        fprintf(metrics_fp, "       _winRateModelMaximum:%.2f   winRateModel:%.2f\n", MyView::_pSelf->_winRateModelMaximum, pSearchAction->winRateModel);
        fprintf(metrics_fp, "       maxDepthActionNode:%d\n", pSearchAction->maxDepthActionNode);
        fprintf(metrics_fp, "       whoWin:%d\n", pSearchAction->playerID);
        fprintf(metrics_fp, "       CountWins:  Blancas:%d  Negras:%d  Tablas:%d   maxsteps:%d\n",  MyView::_pSelf->countWhoWin[0], 
                                                                                                    MyView::_pSelf->countWhoWin[1],
                                                                                                    MyView::_pSelf->countWhoWin[2], 
                                                                                                    MyView::_pSelf->countWhoWin[3]);
        fflush(metrics_fp);
    }
    if (MyView::_pSelf->_stopTraining)
    {
        pSearchAction->bForceStopTraning = true;
    }

    if (MyView::_pSelf->_viewTraning)
    {
        MyView::_pSelf->drawStep(pSender, pSearchAction);
        if (MyView::_pSelf->_realTime)
        {
            wxMilliSleep(1000);
        }
    }


    //if ((pSearchAction->step % 10) == 0)
    {
        wxTheApp->Yield();
        MyView::_pSelf->Refresh();  // Mark the window as needing a repaint
        MyView::_pSelf->Update();
    }
}

// callback when Metis-Core execute a step in the traning
void onStepTraining(void* pSender, Metis::TMULTIHEADAGENTMETRICS* pMetrics)
{
    
    
    
#ifdef _DEBUG
    //MyView::_pSelf->drawStep(pSender, pMetrics->episode, pMetrics->step);
#else
    //if (MyView::_pSelf->_bDisplayInRealTime)
    //{
    //    wxMilliSleep(100);
    //}
    //if (MyView::_pSelf->_bDisplayTranning)
    //{
    //    wxTheApp->Yield();
    //    MyView::_pSelf->Refresh();  // Mark the window as needing a repaint
    //    MyView::_pSelf->Update();
    //}
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
    
}

MyView::MyView(wxWindow* parent, wxWindowID id, const int* args)
    : wxPanel(parent)
{
    

    MyView::_pSelf = this;
    // Bind events
    //Bind(wxEVT_PAINT, &MyView::OnPaint, this);
    Bind(wxEVT_SIZE, &MyView::OnSize, this);
   
    wxImage::AddHandler(new wxPNGHandler);
    bool bRes = _logoMetisCore.LoadFile("MetiscoreAlphaZeroGoResize.png", wxBITMAP_TYPE_PNG);
    
    _logoMetisCore.SetMaskColour(0, 255, 0);

    bRes = _blacks.LoadFile("FiguresBlacks.bmp", wxBITMAP_TYPE_BMP);
    _blacks.SetMaskColour(0, 255, 0);

    bRes = _whites.LoadFile("FiguresWhites.bmp", wxBITMAP_TYPE_BMP);
    _whites.SetMaskColour(0, 0, 0);

    this->SetBackgroundStyle(wxBG_STYLE_PAINT); // to avoid window repaint the background and avoid the flicking

    _pWhites = MY_NEW Player(COLOR_WHITE);
    _pBlacks = MY_NEW Player(COLOR_BLACK);
    _pBoard = MY_NEW Board(_pWhites, _pBlacks);

    _pBoard->ini();

    _AGZTrainer = MY_NEW Metis::AlphaGoZeroTrainer();
    _AGZTrainer->setMaterialHeuristicWeight(0.3); // to help a little bit to MCTS
    
    _minTotalLoss = 1000.0;
    _minNumStepXEpisode = 1000;
    _maxPlayerBlueWin = 0;
    _countPlayerBlueWin = 0;
    _winRateModelMaximum = -1.0;
    _viewTraning = false;
    _stopTraining = false;

    _isPlaying = false;
    _isDragging = false;

    countWhoWin[0]=0;
    countWhoWin[1] = 0;
    countWhoWin[2] = 0;
    countWhoWin[3] = 0; // -1

}

MyView::~MyView()
{


}

// Keyboard handler: Detects Ctrl + Right Arrow
void MyView::OnKeyDown(wxKeyEvent& event)
{
    
}

// Convierte coordenadas de ratón (píxeles) a índices del tablero (i = X, j = Y)
// Devuelve false si el clic cae fuera de los límites del tablero (0..MAXROWS-1)
bool MyView::pixelToBoard(int xPixel, int yPixel, int& outI, int& outJ)
{
    const int cellWidth = 50;
    const int cellHeight = 50;

    wxSize sizeWnd = GetClientSize();

    // 1. Eje X: Directo
    outI = xPixel / cellWidth;

    // 2. Eje Y: Invertido desde la parte inferior de la ventana
    outJ = (sizeWnd.GetHeight() - yPixel) / cellHeight;

    // 3. Validar que la posición esté dentro del tablero
    return (outI >= 0 && outI < MAXROWS && outJ >= 0 && outJ < MAXROWS);
}

void MyView::OnMouseDown(wxMouseEvent& event)
{
    if (_turnPlay != HUMAN_TURN) return;


    const int cellWidth = 50;
    const int cellHeight = 50;

    int mouseX = event.GetX();
    int mouseY = event.GetY();
    wxSize sizeWnd = GetClientSize(); // Obtener tamaño real de la ventana
        
    int boardI;
    int boardJ;
    pixelToBoard(mouseX, mouseY, boardI, boardJ);

    // Validate board boundaries (0..6)
    if (boardI < 0 || boardI >= MAXROWS || boardJ < 0 || boardJ >= MAXROWS)
    {
        return;
    }
    
    // Get the piece at(i, j) based on your native coordinates  
    Figure* f = _pBoard->getFigure(boardI, boardJ);

    // Validate human piece and initiate Drag & Drop
    if (f && !f->isEmpty() && f->getColor() == COLOR_WHITE)
    {
        _isDragging = true;
        _dragSourceI = boardI;
        _dragSourceJ = boardJ;
        _dragPos = event.GetPosition();

        CaptureMouse();
        Refresh();
    }
}

void MyView::OnMouseMove(wxMouseEvent& event)
{
    if (_isDragging && event.Dragging())
    {
        _dragPos = event.GetPosition();
        Refresh();
    }
}

void MyView::processDragAndDrop(int fromI, int fromJ, int toI, int toJ)
{
    // Avoid processing if it is the same origin square
    if (fromI == toI && fromJ == toJ)
    {
        Refresh(); // Simplemente repinta la pieza en su sitio
        return;
    }

    // Get the piece intended to be moved
    Figure* pFigure = _pBoard->getFigure(fromI, fromJ);
    if (!pFigure || pFigure->isEmpty())
    {
        Refresh();
        return;
    }

    Piece p = pFigure->getPiece();
    bool bClearPath = true;
    if ((p == Piece::WhiteKnight) || (p == Piece::BlackKnight) )
    {
        bClearPath = false;
    }

    bool bValidMov = _pBoard->isValidMovement(fromI, fromJ, toI, toJ, pFigure, bClearPath);
    if (!bValidMov)
    {
        return;
    }

    // Try to apply the move to the board logic
    _pBoard->moveFigure(fromI, fromJ, toI, toJ, pFigure);

    //if (!validMove)
    //{
    //    // Movimiento ilegal: la pieza regresa a su posición original
    //    Refresh();
    //    return;
    //}

    // -------------------------------------------------------------
    // VALID HUMAN MOVE APPLIED
    // -------------------------------------------------------------

    // Paint the screen (WM_PAINT)
    Refresh();
    Update();

    // Check if the human has finished the game
    if (_pBoard->isTerminal())
    {
        _turnPlay = GAME_OVER;
        wxMessageBox("You win!! :-(", "End");
        return;
    }


    // -------------------------------------------------------------
    // ALPHAGO ZERO TURN (MCTS)
    // -------------------------------------------------------------
    _turnPlay = IA_TURN;

    wxClientDC dc(this);
    this->Draw(dc); // para pintar que esta pensando la ioa

    wxYield();
    int bestMove = _AZGPlayer->selectAction(_pBoard, _turnPlay);
    _pBoard->applyAction(bestMove);
       

    Refresh();


    if (_pBoard->isTerminal())
    {
        _turnPlay = GAME_OVER;
        wxMessageBox("AlphaZero Go win!!! :-)", "End");
    }
    else
    {
        _turnPlay = HUMAN_TURN; // Devolver el control al humano
    }

    Update();
}

//  DROP AND EXECUTE MOV
void MyView::OnMouseUp(wxMouseEvent& event)
{
    if (!_isDragging) return;

    _isDragging = false;

    if (HasCapture()) 
    {
        ReleaseMouse();
    }

    
    int outI;
    int outJ;
    pixelToBoard(event.GetX(), event.GetY(), outI, outJ);

    // Execute the logic of the movement.
    processDragAndDrop(_dragSourceI, _dragSourceJ, outI, outJ);
}

void MyView::OnSize(wxSizeEvent& event)
{

}

bool MyView::isPlaying()
{

    return false;
}



// modeBot: 1 man in the loop
//          2: bot procedural
void MyView::Play(int modeBot)
{
    _isPlaying = true;

    _turnPlay = 0; // AlphaGo-Zero
    //srand(123);

    _AZGPlayer = Metis::createAlphaZeroGoAgentFromModelFile(_pBoard,(char*)"AlamosChess.ia");
    _AZGPlayer->setMaterialHeuristicWeight(0.3);
    _pBoard->ini();

    this->Refresh();
    this->Update();
}

void MyView::processSquareClick(int r, int c)
{
}

void MyView::drawStep(void* pSender, Metis::TSEARCHACTION* pSearchAction)
{
    wxClientDC dc(this); // Get the DC for the client area of the window (for debugin, instand paiting)

    // Background
    dc.SetBackground(wxBrush(wxColour(10, 20, 30)));
    dc.Clear();

    Board *pBoard = (Board *) pSearchAction->pState;

    this->Draw(dc,pBoard, pSearchAction);
}

wxBrush MyView::GetBrushForSquare(bool isLight)
{
    // Ejemplo usando la Paleta "AI Modern Dark"
    if (isLight)
        return wxBrush(wxColour(140, 155, 170));
    else
        return wxBrush(wxColour(85, 100, 115));
}

int getIndexFromPiece(Piece &aPiece)
{
    int index = -1;
    switch (aPiece)
    {
        case Piece::Empty:
        {
            index = -1;
            break;
        }
        case Piece::BlackPawn:
        case Piece::WhitePawn:
        {
            index = 5;
            break;
        }
        case Piece::BlackRook:
        case Piece::WhiteRook:
        {
            index = 0;
            break;
        }
        case Piece::BlackKnight:
        case Piece::WhiteKnight:
        {
            index = 1;
            break;
        }
        case Piece::BlackBishop:
        case Piece::WhiteBishop:
        {
            index = 2;
            break;
        }
        case Piece::BlackQueen:
        case Piece::WhiteQueen:
        {
            index = 3;
            break;
        }
        case Piece::BlackKing:
        case Piece::WhiteKing:
        {
            index = 4;
            break;
        }
    }

    return index;
}
void MyView::drawPiece(wxDC& dc,int x, int y, Figure* pFigure)
{
    int color = pFigure->getColor();
    wxBitmap subBitmap;
    int srcX = 0;
    int srcY = 0;
    int spriteWidth = 55;
    int spriteHeight = 40;

    Piece aPiece = pFigure->getPiece();
    int indexInSprite = getIndexFromPiece(aPiece);
    if (indexInSprite == -1)
        return;

    srcX = indexInSprite * spriteWidth;

    if (color == COLOR_WHITE)
    {
        subBitmap = _whites.GetSubImage(wxRect(srcX, srcY, spriteWidth, spriteHeight));
    }
    else
    {
        subBitmap = _blacks.GetSubImage(wxRect(srcX, srcY, spriteWidth, spriteHeight));
    }

    dc.DrawBitmap(subBitmap, x, y, true); // El 'true' activa el uso de máscara/transparencia


}
/// <summary>
/// Drawing starts from the bottom-left corner (0,0)
/// </summary>
/// <param name="dc"></param>
/// <param name="pBoard"></param>
void MyView::Draw(wxDC& dc, Board* pBoard, Metis::TSEARCHACTION *pSearchAction)
{
    

    // draw logo Metis-core
    if (_logoMetisCore.IsOk())
    {
        dc.DrawBitmap(_logoMetisCore, 0.0, 0.0, true);
    }

    wxSize sizeWnd = GetSize();

    if (pSearchAction != NULL)
    {
        
        dc.SetTextForeground(wxColour(167, 255, 235)); // Verde Menta suave
        wxSize logoSize = _logoMetisCore.GetSize();
        int xo = logoSize.GetWidth() + 10;
        int y = 10;
        wxString str2;
        str2= wxString::Format("Episode:%d  step:%d", pSearchAction->episode, pSearchAction->step);
        dc.DrawText(str2, xo, y);
        y += 15;

        double totalLoss = pSearchAction->polictyLoss + pSearchAction->valueLoss;
        wxString str;
        str= wxString::Format("TotalLoss:%.3f", totalLoss);
        dc.DrawText(str, xo, y);
        y += 15;


        str = wxString::Format("policyLoss:%.3f", pSearchAction->polictyLoss);
        dc.DrawText(str, xo, y);
        y += 15;

        str = wxString::Format("valueLoss:%.3f", pSearchAction->valueLoss);
        dc.DrawText(str, xo, y);
        y += 15;

        str = wxString::Format("winRateModel:%.2f", pSearchAction->winRateModel);
        dc.DrawText(str, xo, y);
        y += 15;

    }
    if (!_viewTraning && !_isPlaying)
    {
        return;
    }

    int cellWidth = 50;
    int cellHeight = 50;

    if (_isPlaying)
    {
        dc.SetTextForeground(wxColour(167, 255, 235)); // Verde Menta suave
        int y = _logoMetisCore.GetHeight() + 15;
        wxString str2;
        if (_turnPlay == IA_TURN)
        {
            str2 = "AlphaGo Zero thinking...";
        }
        else
        {
            str2 = "Human playing..";
        }

        dc.DrawText(str2, 5, y);
        y += 15;
    }
    
    
    
    for (int i = 0; i < MAXROWS; i++)
    {
        for (int j = 0; j < MAXROWS; j++)
        {
            // X-AXIS: Normal (left to right)
            int x = i * cellWidth;

            // Y-AXIS: Inverted
            // Calculate: Total height - (current row + 1) * cell height
            int y = sizeWnd.GetHeight() - ((j + 1) * cellHeight);

            // DRAW BACKGROUND
            bool isBlackSquare = (i + j) % 2 == 0;
            wxBrush brushSquare = GetBrushForSquare(isBlackSquare);
            dc.SetBrush(brushSquare);
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.DrawRectangle(x, y, cellWidth, cellHeight);

            if (!pBoard->isEmpty(i,j))
            {
                Figure *pFigure = pBoard->getFigure(i, j);
                drawPiece(dc,x, y, pFigure);
            }
        }
    }

    // DRAW FLOATING PIECE (On top of the entire board)   
    if (_isDragging)
    {
        Figure* pDraggingFigure = _pBoard->getFigure(_dragSourceI, _dragSourceJ);
        if (pDraggingFigure && !pDraggingFigure->isEmpty())
        {
            // Center the piece under the cursor tip
            int floatX = _dragPos.x - (cellWidth / 2);
            int floatY = _dragPos.y - (cellHeight / 2);

            drawPiece(dc, floatX, floatY, pDraggingFigure);
        }
    }
}
void MyView::Draw(wxDC& dc)
{
    dc.SetBackground(wxBrush(wxColour(10, 20, 30)));
    dc.Clear();

    Draw(dc, _pBoard,NULL);
}
void MyView::OnPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this); // to debug better, drawing inmediate after draw funtions called
    
    Draw(dc);
    
}

void MyView::StopTraning()
{
    _stopTraining = true;
}
void MyView::LoadTraning()
{
    
}
void MyView::SaveIAModel()
{

    _AGZTrainer->saveIAModel((char *)"AlamosChess.ia");
    
}
void MyView::DisplayTranning(double bViewTraining)
{
    _viewTraning = bViewTraining;
}
void MyView::DisplayInRealTime(double bViewTraining)
{
    _realTime = bViewTraining;
}


void MyView::StartTraningAZG()
{
    bool bIsPresent_GPU = Metis::isCUDAavailable();
    //bIsPresent_GPU = true;

    _pWhites->setID(0);
    _pBlacks->setID(1);

    // set up
    _AGZTrainer->setLoadModel((char*)"AlamosChess.ia");
    _AGZTrainer->setCallbackPerStep( (void *) this,callbackSearchMovement);

    _AGZTrainer->setMaterialHeuristicWeight(0.3); // to help a little bit to MCTS

    _AGZTrainer->training<TBOARD,TPIECEMOVEMENT>(_pBoard, _pWhites, _pBlacks, bIsPresent_GPU); //traning with alphazero go

    delete _AGZTrainer;


}
