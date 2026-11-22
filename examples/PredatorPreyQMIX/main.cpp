// ============================================================================
// demo_render.cpp - renderer test without environment or network:
// 2 predators moving randomly and 1 random prey on a 7x7 grid.
// Replace this loop with your PredatorPreyEnv + QMIXLearner.
// ============================================================================

#include "pch.h"
#include <random>
#include <cstdlib>
#include <algorithm>
#include <map>
#include <numeric>


#include "PredatorPreyRenderer.h"
#include "Kernel/predator.h"
#include "Kernel/predatorPrayEnv.h"

PredatorPreyRenderer gPredatorRender(72);
predatorPrayEnv gEnv;
int gMaxSuccessValue = -1;
float gMaxMeanReward = -1;


float computeMean(const std::deque<float>& meanGlobalReward)
{
    if (meanGlobalReward.empty()) {
        return 0.0f; // Avoid division by zero (or throw an exception / return NaN depending on your case)
    }

    float sum = std::accumulate(meanGlobalReward.begin(), meanGlobalReward.end(), 0.0f);
    return sum / static_cast<float>(meanGlobalReward.size());
}

//typedef void (*OnStepTraining)(void* pSender, TQMIX_METRICS* pMetrics);
void OnStepTraining_cb(void* pSender, Metis::TQMIX_METRICS* pMetrics)
{

    float r = pMetrics->globalReward;
    gEnv.setLastReward(r);

    gPredatorRender.draw(gEnv);
}
int countSuccess(const std::deque<int>& success50)
{
    return (int)std::count(success50.begin(), success50.end(), 1);
}


void OnEndTraining_cb(void* pSender, Metis::TQMIX_METRICS* pMetrics)
{
    static FILE* fp = NULL;
    if (fp == NULL) fp = fopen("qmixtraning.txt", "w+t");

    if (gEnv._success50.size() >= 50)
        gEnv._success50.pop_front();
    if (pMetrics->bWin)
        gEnv._success50.push_back(1);
    else gEnv._success50.push_back(0);

    if (gEnv.meanGlobalReward.size() >= 50)
        gEnv.meanGlobalReward.pop_front();
    gEnv.meanGlobalReward.push_back(pMetrics->globalReward);

    double meanR = computeMean(gEnv.meanGlobalReward);



    int countSucessValue = countSuccess(gEnv._success50);
    bool bUpdateBestModel = false;
    if (countSucessValue > gMaxSuccessValue)
    {
        bUpdateBestModel = true;
        gMaxSuccessValue = countSucessValue;
        for (int i = 0; i < MAX_PREDATORS; i++)
        {
            char szName[500];
            sprintf(szName, "predator_%d.ia", i);
            gEnv._predators[i].saveIAModel(szName);
        }
    }
    if (meanR > gMaxMeanReward)
    {
        bUpdateBestModel = true;
        gMaxMeanReward = meanR;
    }

    fprintf(fp, "Episode:%d  captures:%d  epsilon:%.3f\n", pMetrics->episode, gEnv._countCaptures, pMetrics->epsilon);
    fprintf(fp, "    globalReward:%.3f  loss:%.3f    gMaxSuccessValue:%d\n", pMetrics->globalReward, pMetrics->loss, gMaxSuccessValue);
    fprintf(fp, "   gMaxMeanReward:%.3f  meanR:%.3f\n", gMaxMeanReward, meanR);


    if (bUpdateBestModel)
    {
        fprintf(fp, "   Writing new models to file\n");
    }

    fflush(fp);

    gPredatorRender.draw(gEnv);




}

void playQMIX(PredatorPreyRenderer* pRender, int numEpisodes = 0)
{
    

    // --- load each predator's network and disable exploration
    for (int i = 0; i < MAX_PREDATORS; i++)
    {
        char szName[500];
        sprintf(szName, "predator_%d.ia", i);
        gEnv._predators[i].loadIAModel(szName);
        gEnv._predators[i].setPlayMode();          // set on play mode on agents
    }


    int totalCaptures = 0;
    int totalSteps = 0;      // steps of the episodes with a capture
    int totalSolo = 0;       // solo capture attempts

    while (pRender->isOpen() && (numEpisodes == 0 || gEnv._episode < numEpisodes))
    {
        gEnv._episode++;
        gEnv.reset();
        gEnv._step = 0;

        pRender->draw(gEnv);

        bool isDone = false;
        bool win = false;


        while (!isDone && pRender->isOpen())
        {
            gEnv._step++;
            for (int i = 0; i < MAX_PREDATORS; i++)
                gEnv._predators[i]._tryCaptured = 0;

            std::vector<float> joinState;
            gEnv.getJoinState(&joinState);

            // --- each predator chooses with ITS OWN observation (no global state)
            std::map<int, int> jointActions;
            for (int i = 0; i < MAX_PREDATORS; i++)
            {
                std::vector<float> predatorObs;
                gEnv.getAgentObservation(i, &predatorObs);
                int predatorID = gEnv._predators[i].getID();
                std::vector<int> validActions = gEnv.getValidActionsFromAgent(i);
                jointActions[predatorID] = gEnv._predators[i].selectActionQmix(predatorObs, i, validActions);
            }
            jointActions[gEnv._prey.getID()] = gEnv._prey.getActionProcedural();

            gEnv.applyActions(jointActions);

            // --- resolve capture and reward (same as in training)
            bool bWin;
            bWin = false;
            double jointReward = gEnv.getJoinReward(&isDone, &bWin);
            if (gEnv.lastWasPenalty())
                totalSolo++;
            if (isDone && bWin)
                win = true;

            if (gEnv._step >= MAX_STEPS)
                isDone = true;   // time limit cut-off

            pRender->draw(gEnv);                          // here yes: draw every step
        }


        if (win)
        {
            totalCaptures++;
            totalSteps += gEnv._step;
        }

        // --- console summary
        printf("Play episode %d: %s in %d steps   | captures %d/%d (%.0f%%)  mean steps %.1f  solo %d\n",
            gEnv._episode, win ? "CAPTURE" : "no capture", gEnv._step,
            totalCaptures, gEnv._episode, 100.0 * totalCaptures / gEnv._episode,
            totalCaptures ? (double)totalSteps / totalCaptures : 0.0, totalSolo);
    }
}

int main()
{


    printf("-------------------------------------------------\n");
    printf("Options:\n");
    printf(" Training: 1\n");
    printf(" Play: 2\n");
    printf("        Choose a option\n");
    int option;
    scanf("%d", &option);

    if (option == 1)
    {
        const int W = 7, H = 7;
        gPredatorRender.open("QMIX Predator-Prey ");

        Metis::QMIXTrainer aQmixTrainer;

        bool bUseCUDA = Metis::isCUDAavailable();

        //// --- load each predator's network and disable exploration
        //for (int i = 0; i < MAX_PREDATORS; i++)
        //{
        //    char szName[500];
        //    sprintf(szName, "predator_%d.ia", i);
        //    env._predators[i].loadIAModel(szName);
        //    env._predators[i].setPlayMode();          // set on play mode on agents
        //}


        aQmixTrainer.setCallbackPerStep(OnStepTraining_cb);
        aQmixTrainer.setCallbackEndEpisode(OnEndTraining_cb);
        aQmixTrainer.training(&gEnv, bUseCUDA);
    }
    else
    {
        const int W = 7, H = 7;
        gPredatorRender.open("QMIX Predator-Prey ");

        playQMIX(&gPredatorRender);
    }


    return 0;
}
