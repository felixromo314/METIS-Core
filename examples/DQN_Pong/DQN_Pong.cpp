// ---------------------------------------------------------------------------
//  DQN_Pong.cpp - Pong with the left paddle (AI) trained by Metis' DQN
//
//  Startup menu:
//    [T] Training : the left paddle learns against the procedural opponent.
//                   Every time it improves it is saved to pongModel.ia
//                   SPACE -> show/hide the training (hidden = faster)
//                   ESC or close window -> stop
//    [P] Play     : the AI (left, blue) loads pongModel.ia and plays
//                   against the human (right, red) with UP / DOWN ARROW
// ---------------------------------------------------------------------------

#include "raylib.h"
#include <math.h>

#include "Kernel/Ball.h"
#include "Kernel/Racket.h"
#include "Kernel/PongEnviroment.h"

#include <cstdio>
#include <cstdlib>
#include <ctime>

#define MODEL_FILE "pongModel.ia"



const int screenWidth = 800;
const int screenHeight = 450;

// game objects (global: Metis callbacks are C functions without context)
Ball   ball;
Racket leftPaddle;                  // AI (DQN)
Racket rightPaddle;                 // opponent: procedural (Training) or human (Play)
PongEnviroment environment(screenWidth, screenHeight, &leftPaddle, &rightPaddle, &ball);



static Texture2D g_metisLogo;   // logo texture (loaded only once)

// loads the PNG and turns the green background transparent (with tolerance for the anti-aliased edges)
static Texture2D LoadLogoWithoutBackground(const char* fileName)
{
    if (!FileExists(fileName))
    {
        TraceLog(LOG_WARNING, "Logo file not found. Current folder: %s", GetWorkingDirectory());
    }

    Image img = LoadImage(fileName);
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);   // ensures an alpha channel
    Color background = GetImageColor(img, 0, 0);             // background color (top-left corner)

    Color* pix = (Color*)img.data;
    for (int i = 0; i < img.width * img.height; i++)
    {
        float dr = (float)pix[i].r - background.r;
        float dg = (float)pix[i].g - background.g;
        float db = (float)pix[i].b - background.b;
        float distance = sqrtf(dr * dr + dg * dg + db * db);

        if (distance < 45.0f)
        {
            pix[i] = BLANK;                                                     // background -> transparent
        }
        else if (distance < 90.0f)
        {
            pix[i].a = (unsigned char)(255.0f * (distance - 45.0f) / 45.0f);    // semi-transparent edge
            if (pix[i].g > pix[i].r && pix[i].g > pix[i].b)
                pix[i].g = (pix[i].r > pix[i].b) ? pix[i].r : pix[i].b;         // removes the green tint
        }
    }

    Texture2D tex = LoadTextureFromImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);   // smooth edges when scaled
    UnloadImage(img);
    return tex;
}

// draws the METIS-Core logo centered at the top
// scale: 1.0 = original size (200x213), 0.5 = half...   topMargin: pixels from the top edge
void DrawMetisLogo(float scale = 1.0f, float topMargin = 30.0f)
{
    if (g_metisLogo.id == 0)
        g_metisLogo = LoadLogoWithoutBackground("logo_metis_watermark.png");   // first call (after InitWindow)

    if (g_metisLogo.id == 0)
        return;   // the file could not be loaded

    Vector2 position = { (GetScreenWidth() - g_metisLogo.width * scale) / 2.0f, topMargin };
    DrawTextureEx(g_metisLogo, position, 0.0f, scale, WHITE);
}

// frees the texture on exit (before CloseWindow)
void UnloadMetisLogo()
{
    if (g_metisLogo.id != 0)
    {
        UnloadTexture(g_metisLogo);
        g_metisLogo = { 0 };
    }
}

#pragma region DRAWING
// draws the court, paddles, ball and a line of text
void drawScene(const char* text)
{
    BeginDrawing();
    ClearBackground(BLACK);

    // center line
    for (int y = 0; y < screenHeight; y += 24)
        DrawRectangle(screenWidth / 2 - 1, y, 3, 14, DARKGRAY);

    Metis::Vector2D leftPos = leftPaddle.GetPosition();
    Metis::Vector2D rightPos = rightPaddle.GetPosition();
    Metis::Vector2D ballPos = ball.GetPosition();

    DrawRectangle((int)leftPos.x - 2, (int)leftPos.y - leftPaddle.GetLen() / 2, 5, leftPaddle.GetLen(),
        Color{ leftPaddle._r, leftPaddle._g, leftPaddle._b, 255 });
    DrawRectangle((int)rightPos.x - 2, (int)rightPos.y - rightPaddle.GetLen() / 2, 5, rightPaddle.GetLen(),
        Color{ rightPaddle._r, rightPaddle._g, rightPaddle._b, 255 });
    DrawCircle((int)ballPos.x, (int)ballPos.y, (float)ball.GetRadius(), RAYWHITE);

    DrawText(text, 10, 10, 20, LIGHTGRAY);

    DrawMetisLogo(0.5f);
    EndDrawing();
}
#pragma endregion

#pragma region TRAINING
bool  gShowTraining = true;         // SPACE toggles it
int   gMaxHitsX50 = -1;           // best number of hits in 50 episodes
int   gTotalSteps = 0;
Metis::TAGENTMETRICS gLastMetrics = {};

// Metis calls this on every training step
void onStepTraining(void* /*pSender*/, Metis::TAGENTMETRICS* pMetrics)
{
    gLastMetrics = *pMetrics;
    gTotalSteps++;


    drawScene(TextFormat("TRAINING  ep:%d  loss:%.2f  hits/50 %d  best %d", pMetrics->episode, pMetrics->_meanLoss, pMetrics->countSuccesX50episodes, gMaxHitsX50));




    // ESC or close the window -> stop the training
    if (WindowShouldClose())
        pMetrics->bForceStopTraining = true;
}
// Metis calls this at the end of each episode
void onEndEpisode(void* /*pSender*/, Metis::TAGENTMETRICS* pMetrics)
{

    printf("Episode:%d  hits/50:%d  loss:%.5f\n", pMetrics->episode, pMetrics->countSuccesX50episodes, pMetrics->_meanLoss);

    // save the model when the number of hits in 50 episodes improves
    if (pMetrics->countSuccesX50episodes > gMaxHitsX50)
    {
        gMaxHitsX50 = pMetrics->countSuccesX50episodes;
        leftPaddle.saveIAModel((char*)MODEL_FILE);
        printf("    model saved (hits/50: %d)\n", gMaxHitsX50);
    }
}

void doTraining()
{
    SetTargetFPS(0);                          // no limit: Metis sets the pace
    environment.rightProcedural = true;       // automatic sparring

    Metis::AgentTrainerDQN trainer;
    trainer.setCallbackPerStep(onStepTraining);
    trainer.setCallbackEndEpisode(onEndEpisode);
    trainer.training(&environment, &leftPaddle, nullptr);   // blocks until stopped
}
#pragma endregion

#pragma region PLAY
void doPlay()
{
    SetTargetFPS(60);
    leftPaddle.loadIAModel((char*)MODEL_FILE);
    environment.rightProcedural = false;      // the right paddle is moved by the human
    environment.reset();

    int aiPoints = 0, humanPoints = 0;

    while (!WindowShouldClose())
    {
        // 1. human action (right paddle)
        int humanAction = Racket::ACTION_NOMOVE;
        if (IsKeyDown(KEY_UP))   humanAction = Racket::ACTION_UP;
        if (IsKeyDown(KEY_DOWN)) humanAction = Racket::ACTION_DOWN;
        environment.rightManualAction = humanAction;

        // 2. AI action (left paddle): the network decides, without exploration
        Metis::State state;
        environment.getState(&state);
        environment.serizalizeState(&state, &state._stateVector);
        int aiAction = leftPaddle.predictAction(state);

        // 3. one world tick (moves both paddles and the ball)
        environment.applyAction(&leftPaddle, aiAction);
        leftPaddle.update(0.5);

        // 4. point? (in Play a hit does NOT end anything: only who misses counts)
        TSTATE s;
        environment.getStateFromEnviroment(&s);
        if (s.leftMiss) { humanPoints++; environment.reset(); }
        if (s.rightMiss) { aiPoints++;    environment.reset(); }

        // 5. draw
        drawScene(TextFormat("PLAY   AI %d  -  %d Human   [arrows]", aiPoints, humanPoints));
    }
}
#pragma endregion

// startup menu inside the window: returns 'T', 'P' or 0 if it is closed
int chooseMode()
{
    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        if (IsKeyPressed(KEY_T)) return 'T';
        if (IsKeyPressed(KEY_P)) return 'P';

        BeginDrawing();
        ClearBackground(BLACK);
        DrawText("DQN Pong - Metis-Core", 230, 120, 32, RAYWHITE);
        DrawText("[T]  Training  (the AI learns)", 250, 200, 22, LIGHTGRAY);
        DrawText("[P]  Play      (human vs AI)", 250, 240, 22, LIGHTGRAY);
        EndDrawing();
    }
    return 0;
}

int main()
{
    srand((unsigned)time(nullptr));     // seed only once

    leftPaddle.SetColor(60, 130, 255);  // blue: AI
    rightPaddle.SetColor(235, 70, 70);  // red: opponent / human

    // the network: NUM_INPUTS inputs (serizalizeState) and 3 actions (UP, NOMOVE, DOWN)
    leftPaddle.createBrain(NUM_INPUTS, Racket::NUM_ACTIONS);
    environment.addAgent((Metis::IAgent*)&leftPaddle);

    InitWindow(screenWidth, screenHeight, "DQN Pong");

    int mode = chooseMode();
    if (mode == 'T') doTraining();
    if (mode == 'P') doPlay();

    CloseWindow();
    UnloadMetisLogo();
    return 0;
}