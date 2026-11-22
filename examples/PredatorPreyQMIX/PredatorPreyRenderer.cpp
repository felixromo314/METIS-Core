// ============================================================================
// PredatorPreyRenderer.cpp  -  only file that includes raylib
// Draws directly from predatorPrayEnv (read-only).
// ============================================================================
#include "pch.h"
#include "PredatorPreyRenderer.h"
#include "Kernel/predatorPrayEnv.h"

#include <cmath>

namespace
{
	// fixed colors per agent (up to 6; then they repeat)
	const Color kAgentColors[] = {
		{ 0, 121, 241, 255 },   // blue
		{ 0, 158, 47, 255 },    // green
		{ 255, 161, 0, 255 },   // orange
		{ 200, 122, 255, 255 }, // purple
		{ 0, 190, 190, 255 },   // turquoise
		{ 211, 176, 131, 255 }, // beige
	};
	Color agentColor(int i) { return kAgentColors[i % 6]; }

	const Color kBg       = { 24, 26, 32, 255 };
	const Color kGridLine = { 55, 60, 72, 255 };
	const Color kCell     = { 34, 37, 46, 255 };
	const Color kPrey     = { 230, 41, 55, 255 };
}

PredatorPreyRenderer::PredatorPreyRenderer(int cellPx)
	: _gridW(SIZE_WORD), _gridH(SIZE_WORD), _cell(cellPx)
{
	_actionNames = { "up", "down", "left", "right", "stay", "capture" };
}

PredatorPreyRenderer::~PredatorPreyRenderer()
{
	close();
}

void PredatorPreyRenderer::open(const char* title)
{
	if (_open) return;
	SetConfigFlags(FLAG_MSAA_4X_HINT);
	InitWindow(_gridW * _cell, _gridH * _cell + _hudH, title);
	SetTargetFPS(60);
	SetExitKey(KEY_ESCAPE);
	_open = true;
}

void PredatorPreyRenderer::close()
{
	if (!_open) return;
	CloseWindow();
	_open = false;
}

bool PredatorPreyRenderer::isOpen() const
{
	return _open && !WindowShouldClose();
}

void PredatorPreyRenderer::handleInput(bool& stepOnce)
{
	if (IsKeyPressed(KEY_SPACE)) _paused = !_paused;
	if (IsKeyPressed(KEY_V))     _showSight = !_showSight;
	if (IsKeyPressed(KEY_N))     stepOnce = true;
	if (IsKeyPressed(KEY_KP_ADD) || IsKeyPressed(KEY_EQUAL))      _sps = _sps * 2 > 240 ? 240 : _sps * 2;
	if (IsKeyPressed(KEY_KP_SUBTRACT) || IsKeyPressed(KEY_MINUS)) _sps = _sps / 2 < 1 ? 1 : _sps / 2;
}

void PredatorPreyRenderer::draw(predatorPrayEnv& env, const PPHud* hud)
{
	if (!isOpen()) return;

	// keep the frame for one step's time; when paused, redraw until SPACE or N
	const double holdSec = 1.0 / (double)_sps;
	const double t0 = GetTime();
	while (isOpen())
	{
		bool stepOnce = false;
		handleInput(stepOnce);

		BeginDrawing();
		drawFrame(env, hud);
		EndDrawing();

		if (_paused)
		{
			if (stepOnce) break;
			continue;
		}
		if (GetTime() - t0 >= holdSec) break;
	}
}

// quadcopter drone: central body + 4 propellers in X
void PredatorPreyRenderer::drawQuad(float cx, float cy, float size, Color col, int id)
{
	const float arm = size * 0.42f;            // distance center -> propeller
	const float rot = size * 0.20f;            // radius of each propeller
	const float body = size * 0.16f;            // body radius
	const float d = arm * 0.7071f;           // diagonal projection (cos 45º)

	// positions of the 4 propellers (diagonals)
	Vector2 h[4] = {
		{ cx - d, cy - d }, { cx + d, cy - d },
		{ cx - d, cy + d }, { cx + d, cy + d }
	};

	// arms
	for (int k = 0; k < 4; ++k)
		DrawLineEx({ cx, cy }, h[k], 3.0f, Fade(col, 0.9f));

	// propellers: translucent disc + outline
	for (int k = 0; k < 4; ++k)
	{
		DrawCircleV(h[k], rot, Fade(col, 0.35f));
		DrawCircleLinesV(h[k], rot, col);
		DrawCircleV(h[k], 2.0f, RAYWHITE);      // motor shaft
	}

	// body
	DrawCircleV({ cx, cy }, body, col);
	DrawCircleLinesV({ cx, cy }, body, RAYWHITE);

	// agent number
	const char* txt = TextFormat("%d", id);
	int fs = (int)(size * 0.22f);
	DrawText(txt, (int)(cx - MeasureText(txt, fs) / 2), (int)(cy - fs / 2), fs, RAYWHITE);
}
void PredatorPreyRenderer::drawFrame(predatorPrayEnv& env, const PPHud* hud)
{
	ClearBackground(kBg);
	const int c = _cell;
	const int nPred = env.getNumberOfAgents();
	const int sight = env.getSightRange();

	// ---- cells
	for (int y = 0; y < _gridH; ++y)
		for (int x = 0; x < _gridW; ++x)
			DrawRectangle(x * c + 1, y * c + 1, c - 2, c - 2, kCell);

	// ---- vision radius of each predator (partial observation)
	if (_showSight)
	{
		// ---- predators as drones (if they share a cell, they are offset slightly)
		const int visionRadius = env.getSightRange();   // vision radius in cells (replace with your variable)

		for (int i = 0; i < nPred; ++i)
		{
			Metis::Vector2D p = env.getPredatorPos(i);
			int sameCell = 0;
			for (int j = 0; j < i; ++j)
			{
				Metis::Vector2D q = env.getPredatorPos(j);
				if (q.x == p.x && q.y == p.y) sameCell++;
			}
			float off = sameCell * c * 0.12f;
			float cx = p.x * c + c * 0.5f + off, cy = p.y * c + c * 0.5f + off;

			// ---- drone vision zone: (2R+1) x (2R+1) cells centered on its cell
			Color col = agentColor(i);
			Rectangle zone = { (float)(p.x - visionRadius) * c,
							   (float)(p.y - visionRadius) * c,
							   (float)(2 * visionRadius + 1) * c,
							   (float)(2 * visionRadius + 1) * c };
			DrawRectangleRec(zone, Fade(col, 0.10f));               // semi-transparent fill
			DrawRectangleLinesEx(zone, 1.0f, Fade(col, 0.60f));     // border

			drawQuad(cx, cy, (float)c, col, i);
		}
	}

	// ---- grid
	for (int x = 0; x <= _gridW; ++x) DrawLine(x * c, 0, x * c, _gridH * c, kGridLine);
	for (int y = 0; y <= _gridH; ++y) DrawLine(0, y * c, _gridW * c, y * c, kGridLine);

	// ---- prey
	{
		Metis::Vector2D p = env.getPreyPos();
		float cx = p.x * c + c * 0.5f, cy = p.y * c + c * 0.5f;
		float r = c * 0.30f;
		if (env.isPreyCaptured())
		{
			// prey captured: cross and golden halo
			DrawCircleV({ cx, cy }, r * 1.4f, Fade(GOLD, 0.35f));
			DrawLineEx({ cx - r, cy - r }, { cx + r, cy + r }, 4.0f, GOLD);
			DrawLineEx({ cx - r, cy + r }, { cx + r, cy - r }, 4.0f, GOLD);
		}
		else
		{
			// red diamond
			Vector2 top = { cx, cy - r }, right = { cx + r, cy }, bot = { cx, cy + r }, left = { cx - r, cy };
			DrawTriangle(top, left, right, kPrey);
			DrawTriangle(left, bot, right, kPrey);
		}
	}

	// ---- predators (if they share a cell, they are offset slightly)
	for (int i = 0; i < nPred; ++i)
	{
		Metis::Vector2D p = env.getPredatorPos(i);
		int sameCell = 0;
		for (int j = 0; j < i; ++j)
		{
			Metis::Vector2D q = env.getPredatorPos(j);
			if (q.x == p.x && q.y == p.y) sameCell++;
		}
		float off = sameCell * c * 0.12f;
		float cx = p.x * c + c * 0.5f + off, cy = p.y * c + c * 0.5f + off;
		Color col = agentColor(i);
		DrawCircleV({ cx, cy }, c * 0.32f, col);
		DrawCircleLinesV({ cx, cy }, c * 0.32f, RAYWHITE);
		const char* id = TextFormat("%d", i);
		int fs = c / 3;
		DrawText(id, (int)(cx - MeasureText(id, fs) / 2), (int)(cy - fs / 2), fs, RAYWHITE);
	}

	// ---- red border if there was a penalty (solo attack)
	if (env.lastWasPenalty())
		DrawRectangleLinesEx({ 0, 0, (float)(_gridW * c), (float)(_gridH * c) }, 4.0f, kPrey);

	// ---- HUD: environment data
	const int hy = _gridH * c + 8;
	const float eps  = hud ? hud->epsilon : 0.0f;
	const float qtot = hud ? hud->qtot : 0.0f;
	DrawText(TextFormat("Episode %d   Step %d   eps %.2f", env.getEpisode(), env.getStep(), eps), 10, hy, 18, RAYWHITE);
	DrawText(TextFormat("r = %+.2f   Return = %+.2f   Q_tot = %.2f", env.getLastReward(), env.getEpisodeReturn(), qtot),
	         10, hy + 24, 18, env.getLastReward() < 0 ? Color{ 255, 120, 120, 255 } : RAYWHITE);

	// action of each agent (+ its Q if provided in the hud)
	int ax = 10;
	for (int i = 0; i < nPred; ++i)
	{
		int a = env.getLastAction(i);
		std::string txt = TextFormat("A%d: ", i);
		txt += (a >= 0 && a < (int)_actionNames.size()) ? _actionNames[a] : std::string(TextFormat("%d", a));
		if (hud && i < (int)hud->qValues.size()) txt += TextFormat(" (%.2f)", hud->qValues[i]);
		DrawText(txt.c_str(), ax, hy + 50, 16, agentColor(i));
		ax += MeasureText(txt.c_str(), 16) + 18;
	}

	if (hud && !hud->info.empty()) DrawText(hud->info.c_str(), 10, hy + 74, 16, LIGHTGRAY);

	// playback state
	const char* play = _paused ? "PAUSED (N = step)" : TextFormat("%d steps/s", _sps);
	DrawText(play, _gridW * c - MeasureText(play, 16) - 10, hy + 74, 16, _paused ? YELLOW : GRAY);
}
