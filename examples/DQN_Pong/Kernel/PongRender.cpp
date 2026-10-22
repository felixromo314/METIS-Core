// Only file that includes raylib (see comment in PongRender.h)
#include "PongRender.h"
#include "raylib.h"

static int gWidth = 0;
static int gHeight = 0;

void Render_Init(int width, int height, const char* title)
{
    gWidth = width;
    gHeight = height;
    SetTraceLogLevel(LOG_WARNING);   // less noise in the training console
    InitWindow(width, height, title);
    SetTargetFPS(0);                 // no limit: the training sets the pace
}

bool Render_ShouldClose()
{
    return WindowShouldClose();
}

static void drawPaddle(const RenderPaddle& p)
{
    const Color c = { p.r, p.g, p.b, 255 };
    DrawRectangle((int)p.x - 2, (int)(p.y - p.len / 2), 5, (int)p.len, c);
}

void Render_Draw(const RenderFrame& f)
{
    BeginDrawing();
    ClearBackground({ 18, 20, 28, 255 });

    // field
    const Color lines = { 90, 95, 110, 255 };
    for (int y = 0; y < gHeight; y += 24)
        DrawRectangle(gWidth / 2 - 1, y, 3, 14, lines);

    // impact prediction
    if (f.hasPrediction)
    {
        DrawLine((int)f.ballX, (int)f.ballY, (int)f.predictionX, (int)f.predictionY, Fade(YELLOW, 0.25f));
        DrawCircle((int)f.predictionX, (int)f.predictionY, 3, YELLOW);
    }

    drawPaddle(f.left);
    drawPaddle(f.right);
    DrawCircle((int)f.ballX, (int)f.ballY, f.ballRadius, RAYWHITE);

    // HUD
    DrawText(TextFormat("%s  ep %d  step %d", f.mode, f.episode, f.step), 10, 10, 16, LIGHTGRAY);
    DrawText(TextFormat("reward %.2f  loss %.5f  successes/50 %d", f.reward, f.meanLoss, f.successX50), 10, 30, 16, LIGHTGRAY);
    DrawFPS(gWidth - 90, 10);

    EndDrawing();
}

void Render_Close()
{
    CloseWindow();
}