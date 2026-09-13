#include <cstdint>
#include <array>
#include <random>

// Adjust the sizes to your game (e.g., Chess: 8x8, 12 piece types/colors)
constexpr int MAX_ROWS = 6;
constexpr int MAX_COLS = 6;
constexpr int NUM_PIECE_TYPES = 6; // Example: Pawn, Knight, Bishop, Rook, Queen, King  
constexpr int NUM_COLORS = 2;      // 1: White, 2: Black

class ZobristKeys 
{
public:
            // [Row][Column][PieceType][Color]
    uint64_t pieces[MAX_ROWS][MAX_COLS][NUM_PIECE_TYPES][NUM_COLORS];
    uint64_t turn[NUM_COLORS];

    ZobristKeys();

    static ZobristKeys& instance();
};