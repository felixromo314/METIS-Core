#include "pch.h"
#include "prey.h"
#include "predatorPrayEnv.h"



void prey::setPosition(Metis::Vector2D& pos)
{
	_pos = pos;
}
Metis::Vector2D prey::getPosition()
{
	return _pos;
}

int prey::getActionProcedural()
{
    // offsets per action: UP, DOWN, LEFT, RIGHT, STAY
    static const int dx[(int)PreyAction::COUNT] = { 0, 0, -1, 1, 0 };
    static const int dy[(int)PreyAction::COUNT] = { -1, 1,  0, 0, 0 };

    const int size = SIZE_WORD;

    // 0. immobilized? count predators at distance 1 (Manhattan: up/down/left/right)
    int adjacent = 0;
    for (int i = 0; i < _pEnv->getNumberOfAgents(); ++i)
    {
        Metis::Vector2D p = _pEnv->getPredatorPos(i);
        int dist = std::abs(p.x - _pos.x) + std::abs(p.y - _pos.y);
        if (dist == 1) adjacent++;
    }
    if (adjacent >= 2)
        return (int)PreyAction::STAY;        // surrounded by two: does not move

    while (true)
    {
        // 1. pick a random action
        int a = (int)(_rng() % (int)PreyAction::COUNT);

        // 2. validate
        if (a == (int)PreyAction::STAY)
            return a;                        // stay is always valid

        int nx = _pos.x + dx[a];
        int ny = _pos.y + dy[a];

        if (nx < 0 || ny < 0 || nx >= size || ny >= size)
            continue;                        // off the board: retry
        if (!_pEnv->isCellFree(nx, ny))
            continue;                        // occupied: retry

        return a;                            // valid
    }
}
void prey::applyAction(int action)
{
    int nx = _pos.x;
    int ny = _pos.y;

    switch ((PreyAction)action)
    {
    case PreyAction::UP:    ny -= 1; break;   // y - 1
    case PreyAction::DOWN:  ny += 1; break;   // y + 1
    case PreyAction::LEFT:  nx -= 1; break;   // x - 1
    case PreyAction::RIGHT: nx += 1; break;   // x + 1
    case PreyAction::STAY:  return;           // stay still
    default:                return;           // invalid action: stay still
    }

    const int size = SIZE_WORD;

    // re-validate: a predator may have moved into that cell this step
    if (nx < 0 || ny < 0 || nx >= size || ny >= size) return;   // off the board
    if (!_pEnv->isCellFree(nx, ny)) return;                      // occupied

    // move
    _pos.x = nx;
    _pos.y = ny;
}