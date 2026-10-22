#pragma once

// ---------------------------------------------------------------------------
//  PongRender.h - drawing layer with raylib.
//
//  This header does NOT include raylib.h or Metis-Core.h on purpose: if Metis
//  pulls in <windows.h>, it clashes with raylib (CloseWindow, DrawText, Rectangle,
//  ShowCursor...). That is why raylib is only included in PongRender.cpp and here
//  plain data is exchanged.
// ---------------------------------------------------------------------------

struct RenderPaddle
{
    float x, y;            // x = edge, y = vertical center
    float len;
    unsigned char r, g, b;
};

struct RenderFrame
{
    RenderPaddle left;
    RenderPaddle right;
    float ballX, ballY, ballRadius;

    bool  hasPrediction;   // predicted impact point on the left paddle
    float predictionX, predictionY;

    // HUD
    const char* mode;      // "TRAINING" / "PLAY"
    int   episode;
    int   step;
    float reward;
    float meanLoss;
    int   successX50;
};

void Render_Init(int width, int height, const char* title);
bool Render_ShouldClose();
void Render_Draw(const RenderFrame& frame);
void Render_Close();