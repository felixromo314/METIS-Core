#pragma once
#include <vector>
#include <string>

class predatorPrayEnv;   // forward declaration: no need to include the environment here

// Data that does NOT belong to the environment but to the learner (optional in the HUD)
struct PPHud
{
	float epsilon = 0.0f;
	float qtot = 0.0f;
	std::vector<float> qValues;   // Q_a of the action chosen by each agent
	std::string info;             // free text
};

class PredatorPreyRenderer
{
public:
	explicit PredatorPreyRenderer(int cellPx = 72);
	~PredatorPreyRenderer();

	void open(const char* title = "Predator-Prey");
	void close();
	bool isOpen() const;

	// draws the current state of the environment; hud is optional
	void draw(predatorPrayEnv& env, const PPHud* hud = nullptr);

	void setStepsPerSecond(int sps) { _sps = sps < 1 ? 1 : sps; }
	void setActionNames(const std::vector<std::string>& names) { _actionNames = names; }

private:
	int  _gridW, _gridH, _cell;
	int  _hudH = 110;
	int  _sps = 6;
	bool _open = false;
	bool _paused = false;
	bool _showSight = true;
	std::vector<std::string> _actionNames;

	void handleInput(bool& stepOnce);
	void drawFrame(predatorPrayEnv& env, const PPHud* hud);
	void drawQuad(float cx, float cy, float size, Color col, int id);
};
