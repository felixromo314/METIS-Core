#include "PongEnviroment.h"
#include "Racket.h"
#include "Ball.h"

#include <cmath>
#include <cstdlib>

PongEnviroment::PongEnviroment(int w, int h, Racket* pLeft, Racket* pRight, Ball* pBall)
{
    _w = w;
    _h = h;
    _pPlayerLeft = pLeft;
    _pPlayerRight = pRight;
    _pBall = pBall;
    _margin = 10;

    rewardHit = 1.0f;
    rewardMiss = -1.0f;
    pseudoReward = 0.1f;          // same value as in the MiniDNN version

    rightProcedural = true;
    rightManualAction = 1;        // Racket::ACTION_NOMOVE

    // field limits for the physics
    _pBall->SetFieldLimits(0, _h);
    _pPlayerLeft->SetFieldLimits(0, _h);
    _pPlayerRight->SetFieldLimits(0, _h);

    // the right paddle needs to see the ball for its procedural policy
    _pPlayerRight->SetBall(_pBall);
    _pPlayerLeft->SetBall(_pBall);
}

// ---------------------------------------------------------------------------
//  reset(): Metis calls it at the start of each episode
// ---------------------------------------------------------------------------
void PongEnviroment::reset()
{
    // NOTE: there used to be a srand(time(NULL)) here. time() has a 1 s resolution,
    // so all the episodes starting in the same second had the SAME serve.
    // The seed is set only once in main().

    const int middleY = _h / 2;

    _pPlayerLeft->Reset();
    _pPlayerLeft->SetPosition(_margin, middleY);
    _pPlayerLeft->SetState(playing);

    _pPlayerRight->Reset();
    _pPlayerRight->SetPosition(_w - _margin, middleY);
    _pPlayerRight->SetState(playing);

    // serve from the right towards the agent (typeAxis == 1 in the previous version)
    const int minY = 10, maxY = _h - 10;
    const int y = minY + rand() % (maxY - minY + 1);

    const int minHeading = 180 - 30, maxHeading = 180 + 30;
    const int heading = minHeading + rand() % (maxHeading - minHeading + 1);

    _pBall->SetPosition(_w - 15, y);
    _pBall->SetVelocity(6);
    _pBall->SetHeading(heading);
}

// ---------------------------------------------------------------------------
//  getState(): copies the raw state to Metis (it is stored in the replay buffer)
// ---------------------------------------------------------------------------
void PongEnviroment::getStateFromEnviroment(TSTATE* state)
{
    Metis::Vector2D ballPos = _pBall->GetPosition();
    Metis::Vector2D vel = _pBall->GetVectorSpeed();
    const double speed = sqrt(vel.x * vel.x + vel.y * vel.y);

    state->leftPlayerPositionY = (float)_pPlayerLeft->GetPosition().y;
    state->ballPosX = (float)ballPos.x;
    state->ballPosY = (float)ballPos.y;
    state->ballVelX = speed > 0 ? (float)(vel.x / speed) : 0.0f;
    state->ballVelY = speed > 0 ? (float)(vel.y / speed) : 0.0f;
    state->leftDirection = _pPlayerLeft->GetDirection();
    state->leftHit = _pPlayerLeft->ThereIsHitBall() ? 1 : 0;
    state->leftMiss = (_pPlayerLeft->GetState() == gameover) ? 1 : 0;
    state->rightMiss = (_pPlayerRight->GetState() == gameover) ? 1 : 0;
}

void PongEnviroment::getState(Metis::State* pState)
{
    TSTATE pongState;
    getStateFromEnviroment(&pongState);
    pState->copyState(&pongState, sizeof(TSTATE));
}

// ---------------------------------------------------------------------------
//  serizalizeState(): network inputs, normalized to ~[-1, 1]
// ---------------------------------------------------------------------------
void PongEnviroment::serizalizeState(void* /*state*/, std::vector<float>* stateVector)
{
    TSTATE s;
    getStateFromEnviroment(&s);

    const float halfW = _w * 0.5f;
    const float halfH = _h * 0.5f;

    stateVector->push_back((s.leftPlayerPositionY - halfH) / halfH);  // [0] paddle
    stateVector->push_back((s.ballPosX - halfW) / halfW);             // [1] ball X
    stateVector->push_back((s.ballPosY - halfH) / halfH);             // [2] ball Y
    stateVector->push_back(s.ballVelX);                               // [3] direction X
    stateVector->push_back(s.ballVelY);                               // [4] direction Y
    // (NUM_INPUTS = 5; if you add inputs, update NUM_INPUTS)
}

// ---------------------------------------------------------------------------
//  applyAction(): Metis applies the action chosen by the network (index 0..2)
// ---------------------------------------------------------------------------
void PongEnviroment::applyAction(Metis::IAgent* pAgent, int actionId)
{
    Racket* pRacket = static_cast<Racket*>(pAgent);

    pRacket->HitTheBall(false);
    pRacket->ApplyActionId(actionId);

    // Only the trained agent advances the world (1 action = 1 tick)
    if (pRacket == _pPlayerLeft)
    {
        // the opponent: procedural policy (Training) or human (Play)
        int rightAction = rightManualAction;
        if (rightProcedural)
        {
            Metis::State dummy;
            rightAction = _pPlayerRight->getActionProcedural(dummy);
        }
        _pPlayerRight->HitTheBall(false);
        _pPlayerRight->ApplyActionId(rightAction);

        stepSimulation();
    }
}

void PongEnviroment::checkHit(Racket* pRacket, bool isLeft)
{
    Metis::Vector2D ballPos = _pBall->GetPosition();
    Metis::Vector2D ballVel = _pBall->GetVectorSpeed();
    Metis::Vector2D racketPos = pRacket->GetPosition();
    const int radius = _pBall->GetRadius();

    // only if the ball is moving TOWARDS the paddle (before, this was not checked on the left)
    if (isLeft && ballVel.x >= 0) return;
    if (!isLeft && ballVel.x <= 0) return;

    const bool reachedLine = isLeft ? (ballPos.x - radius <= racketPos.x)
        : (ballPos.x + radius >= racketPos.x);
    if (!reachedLine) return;

    const double diffY = fabs(ballPos.y - racketPos.y);
    if (diffY <= pRacket->GetLen() / 2.0 + radius)
    {
        pRacket->HitTheBall(true);
        _pBall->HitWith(pRacket);
    }
}

void PongEnviroment::stepSimulation()
{
    _pBall->Update();

    checkHit(_pPlayerLeft, true);
    checkHit(_pPlayerRight, false);

    // misses: the ball has gone past the paddle line
    Metis::Vector2D ballPos = _pBall->GetPosition();
    const int radius = _pBall->GetRadius();

    if (!_pPlayerLeft->ThereIsHitBall() && (ballPos.x + radius) < _pPlayerLeft->GetPosition().x)
        _pPlayerLeft->SetState(gameover);

    if (!_pPlayerRight->ThereIsHitBall() && (ballPos.x - radius) > _pPlayerRight->GetPosition().x)
        _pPlayerRight->SetState(gameover);
}

// Y where the straight trajectory of the ball crosses the vertical line x = xLine
// (without bounces, same as calculateIntersection in the previous version)
bool PongEnviroment::predictImpactY(float xLine, float* pY)
{
    Metis::Vector2D p = _pBall->GetPosition();
    Metis::Vector2D v = _pBall->GetVectorSpeed();
    if (fabs(v.x) < 1e-6) return false;

    const double t = (xLine - p.x) / v.x;
    if (t < 0) return false;                  // the ball is moving away

    *pY = (float)(p.y + v.y * t);
    return true;
}


bool PongEnviroment::getPredictedImpactLeft(float* pY)
{
    return predictImpactY((float)_pPlayerLeft->GetPosition().x, pY);
}

float PongEnviroment::getDeltaTime()
{
    return 2.0;
}

// ---------------------------------------------------------------------------
//  calculateReward(): same logic as the old Reward() + Step()
//    *pDone =  1 -> end of episode with WIN  (hits the ball)
//    *pDone = -1 -> end of episode with LOSS (lets it through)
// ---------------------------------------------------------------------------
float PongEnviroment::calculateReward(Metis::State& /*state*/, int* pDone)
{
    *pDone = 0;

    // The CURRENT state of the environment is read (after applyAction), as the
    // TreasureHunter example does, so as not to depend on whether Metis passes s or s'.
    TSTATE s;
    getStateFromEnviroment(&s);

    // --- terminal states ---
    if (s.leftHit)
    {
        *pDone = 1;
        return rewardHit;
    }
    if (s.leftMiss)
    {
        *pDone = -1;
        return rewardMiss;
    }
    if (s.rightMiss)
    {
        // the sparring partner missed: episode ended with no reward or penalty
        *pDone = 1;      // TODO: decide whether it counts as a success in countSuccesX50episodes
        return 0.0f;
    }

    // --- shaping: move towards where the ball will arrive ---
    float impactY = 0.0f;
    const bool comingToMe = getPredictedImpactLeft(&impactY);
    const float halfLen = _pPlayerLeft->GetLen() / 2.0f;

    int wantedDir = Racket::DIR_NOMOVE;
    if (comingToMe && impactY >= 0 && impactY <= _h)   // outside the field -> stay still (as before)
    {
        const float diffY = impactY - s.leftPlayerPositionY;
        if (diffY > halfLen) wantedDir = Racket::DIR_DOWN;
        if (diffY < -halfLen) wantedDir = Racket::DIR_UP;
    }

    return (s.leftDirection == wantedDir) ? pseudoReward : -pseudoReward;
}

// only for multi-head networks: not used here
std::vector<Metis::TMULTIHEAD> PongEnviroment::calculateRewards(Metis::State& /*state*/, int* /*pDone*/)
{
    return std::vector<Metis::TMULTIHEAD>();
}