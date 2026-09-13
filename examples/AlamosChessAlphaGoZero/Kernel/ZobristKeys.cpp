#include "stdafx.h"
#include "ZobristKeys.h"

ZobristKeys::ZobristKeys()
{
    // Deterministic with a fixed seed so the hash is reproducible across executions
    std::mt19937_64 rng(123456789ULL);
    std::uniform_int_distribution<uint64_t> dist;

    for (int r = 0; r < MAX_ROWS; ++r) 
    {
        for (int c = 0; c < MAX_COLS; ++c) {
            for (int p = 0; p < NUM_PIECE_TYPES; ++p)
            {
                for (int col = 0; col < NUM_COLORS; ++col)
                {
                    pieces[r][c][p][col] = dist(rng);
                }
            }
        }
    }

    for (int col = 0; col < NUM_COLORS; ++col) 
    {
        turn[col] = dist(rng);
    }
}

ZobristKeys& ZobristKeys::instance()
{
    static ZobristKeys keys;
    return keys;
}
