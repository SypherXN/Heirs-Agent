#include <bits/stdc++.h>
using namespace std;

// ============================================================
// Original Assignment Configuration
// ============================================================

struct Config {
    int maxDepth = 6;
    double safetyMarginSec = 0.05;

    // Simple material values.
    double babyValue = 102.5;
    double princeValue = 120000.0;
    double princessValue = 618.0422338263993;
    double ponyValue = 259.9567511266766;
    double guardValue = 380.5688267197038;
    double tutorValue = 294.03802511391206;
    double scoutValue = 199.29000946382152;
    double siblingValue = 275.0;

    // Small piece-square table tuning (very cheap positional guidance).
    int pstBabyAdvanceWeight = 6;
    int pstBabyCenterWeight = 2;
    int pstPrinceCenterWeight = 6;
    int pstPrinceAdvanceWeight = 2;
    int pstScoutAdvanceWeight = 5;
    int pstScoutCenterWeight = 1;
    int pstSiblingCenterWeight = 8;
    int pstSiblingAdvanceWeight = 1;

    // Piece-aware LMR tuning.
    int lmrBabyExtraReduction = 2;
    int lmrPrinceReductionBonus = 1;
    int lmrPrincessReductionBonus = 1;
    int lmrScoutReductionBonus = 1;
    int lmrSiblingReductionBonus = 1;

    // Time-based depth caps.
    double depth1TimeCapSec = 0.7;
    double depth2TimeCapSec = 2.5;
    double depth3TimeCapSec = 7.5;

    // Quiescence search tuning.
    bool enableQSearch = true;
    int qSearchMaxDepth = 3;
    int qSearchMaxNodes = 96;
    int qSearchDeltaMargin = 16;

    // Selective depth extensions.
    bool enableDepthExtensions = true;
    int maxTotalSearchDepth = 10;
    int extensionTriggerDepth = 2;
    int maxTotalExtensionPerPath = 4;
    int maxRootDepthBonus = 2;
    int phaseEndgamePieceThreshold = 12;
    int phaseEndgameRootBonus = 2;
    int advantageEvalMargin = 11835;
    int advantageRootBonus = 2;
    int tacticalThreatExtensionBonus = 2;
    int mobilityCrunchMoveThreshold = 9;
    int mobilityCrunchExtensionBonus = 2;
    int rootInstabilityScoreJump = 9000;
    int rootInstabilityMoveFlipBonus = 2;
    int rootInstabilityAspirationBonus = 1;

    // Prince-threat avoidance.
    bool enablePrinceThreatAvoidance = true;
    int princeThreatPenalty = 300000;
    int enemyPrinceThreatBonus = 12000;
    int princeThreatExtensionBonus = 1;

    // Root tactical guard tuning.
    int rootPrinceCaptureBonus = 1500000;
    int rootPrincePressureBonus = 220000;
    int rootPrinceDangerPenalty = 660479;

    // Tunable draw-evaluation weights.
    double drawTimeWinReward = 500000000.0;
    double drawTimeLossPenalty = -500000000.0;
    double drawPieceWinReward = 250000000.0;
    double drawPieceLossPenalty = -250000000.0;
    double drawTieScore = -0.07743337614752074;
    double drawTimeBufferBaseSec = 0.03;
    double drawTimeBufferRecentScale = 0.015;
    int recentMoveWindow = 6;
    bool avoidTwoFoldRepetition = true;
    double twoFoldAvoidTimeGapSec = 1.5;
    double twoFoldPreferTimeGapSec = 6.0;
    int twoFoldRepetitionPenalty = 30000;
    int twoFoldRepetitionBonus = 12000;

    // Move-ordering learning heuristics.
    int captureOrderThreshold = 1000;
    bool enableMoveLearning = true;
    int killerMoveBonus = 10000;
    int secondKillerMoveBonus = 6818;
    int historyMoveScale = 11;
    int historyMoveCap = 7000;
    int countermoveBonus = 4000;

    // Tactical capture ordering (SEE-lite) tuning.
    int seeSafeCaptureBonus = 1500;
    int seeRecapturePenaltyScale = 5;
    int seePrinceCaptureBonus = 5000000;

    // Transposition-table aging / decay tuning.
    int ttAgeLimit = 5;

    // Principal Variation Search (PVS) tuning.
    bool enablePVS = true;
    int pvsMinDepth = 2;

    // Late move reductions.
    bool enableLMR = true;
    int lmrMinDepth = 3;
    int lmrQuietMoveStart = 4;
    int lmrBaseReduction = 1;
    int lmrMoveCountScale = 10;
    int lmrMaxReduction = 2;
    int lmrReSearchMargin = 0;
} CFG;

// ============================================================
// Board Constants, Piece Kinds, and Basic Types
// ============================================================

static const int N = 12;
static const int BOARD_SQ = N * N;
static const string COLS = "abcdefghjkmn"; // i and l are skipped.
static const int INF = 1'000'000'000;
static const int MATE = 1'000'000'000;
static const size_t TT_SIZE = 1u << 19;
static const size_t TT_MASK = TT_SIZE - 1;
static const int MAX_MOVES = 1024;
static const int MAX_TEMPLATES_PER_LIST = 32;
static const int MAX_PIECES_PER_SIDE = 24;

using Board = array<char, BOARD_SQ>;

enum Side : uint8_t { WHITE = 0, BLACK = 1 };
enum PieceKind : uint8_t {
    PIECE_NONE = 0,
    BABY = 1,
    PRINCE = 2,
    PRINCESS = 3,
    PONY = 4,
    GUARD = 5,
    TUTOR = 6,
    SCOUT = 7,
    SIBLING = 8
};

struct Move {
    uint8_t from = 0;
    uint8_t to = 0;
    int order = 0;
};

struct MoveList {
    array<Move, MAX_MOVES> moves{};
    int size = 0;
};

struct Undo {
    uint8_t from = 0;
    uint8_t to = 0;
    char mover = '.';
    char captured = '.';
    int prevWhiteEval = 0;
    uint64_t prevHash = 0;
    int prevWhitePrinceSq = -1;
    int prevBlackPrinceSq = -1;
    Side prevSideToMove = WHITE;
    int prevHalfmoveClock = 0;

    // Piece-list bookkeeping so make/unmake stays O(1).
    uint8_t moverListIndex = 255;
    uint8_t capturedListIndex = 255;
    uint8_t prevWhiteCount = 0;
    uint8_t prevBlackCount = 0;
};

// ============================================================
// Global Search State
// ============================================================

static Board gBoard{};
static int gWhiteEval = 0; // White's material evaluation.
static int gWhitePrinceSq = -1;
static int gBlackPrinceSq = -1;
static Side gSideToMove = WHITE;
static Side gRootSide = WHITE;
static uint64_t gHash = 0;
static chrono::steady_clock::time_point gDeadline;
static bool gTimedOut = false;
static uint64_t gNodeCount = 0;

// Piece lists for fast move generation.
static array<array<uint8_t, MAX_PIECES_PER_SIDE>, 2> gPieceList{};
static array<array<uint8_t, BOARD_SQ>, 2> gPiecePos{};
static array<int, 2> gPieceCount{0, 0};
static const int MAX_RECENT_PERF_SAMPLES = 8;
static const int MAX_GAME_HISTORY = 2048;
static array<uint64_t, MAX_GAME_HISTORY> gSearchHashStack{};
static int gSearchHashStackLen = 0;
static int gHalfmoveClock = 0;
static double gMyTimeRemaining = 0.0;
static double gOppTimeRemaining = 0.0;
static array<double, MAX_RECENT_PERF_SAMPLES> gRecentMoveTimes{};
static uint64_t gQSearchNodeCount = 0;
static int gRecentMoveTimesLen = 0;
static unordered_map<uint64_t, int> gHistoryCounts;
static bool gHasPlaydataBoard = false;
static Board gPlaydataBoard{};

// ============================================================
// Move-Learning Tables (Killer / History / Countermove)
// ============================================================

static constexpr int MAX_SEARCH_PLY = 64;
static array<array<Move, 2>, MAX_SEARCH_PLY> gKillerMoves{};
static array<array<Move, BOARD_SQ * BOARD_SQ>, 2> gCounterMoves{};
static array<array<array<int, BOARD_SQ>, BOARD_SQ>, 2> gHistoryScores{};

// ============================================================
// Lookup Tables, Zobrist Keys, and Move Templates
// ============================================================

static int gMaterialValue[256];
static int gSignedMaterial[256];
static uint8_t gPieceKind[256];
static uint8_t gPieceSide[256];
static uint8_t gPieceIndex[256];
static int gPstValue[9][BOARD_SQ];
static uint64_t gZobrist[17][BOARD_SQ];
static uint64_t gSideKey = 0;

struct TTEntry {
    uint64_t key = 0;
    int depth = -1;
    int score = 0;
    uint8_t flag = 0; // 0 = exact, 1 = lower, 2 = upper
    uint8_t bestFrom = 255;
    uint8_t bestTo = 255;
    uint16_t generation = 0;
};

static array<TTEntry, TT_SIZE> gTT;
static uint16_t gTTGeneration = 0;

struct MoveTemplate {
    uint8_t from = 0;
    uint8_t to = 0;
    uint8_t pathLen = 0;
    uint8_t path0 = 255;
    uint8_t path1 = 255;
    int baseOrder = 0;
};

struct TemplateList {
    array<MoveTemplate, MAX_TEMPLATES_PER_LIST> items{};
    uint8_t size = 0;
};

static TemplateList gMoveTemplates[2][9][BOARD_SQ];

// ============================================================
// Small Utility Helpers
// ============================================================

static inline int sq(int r, int c) { return r * N + c; }
static inline int rowOf(int s) { return s / N; }
static inline int colOf(int s) { return s % N; }
static inline int mirrorSq(int s) { return sq(N - 1 - rowOf(s), colOf(s)); }
static inline bool inBounds(int r, int c) { return r >= 0 && r < N && c >= 0 && c < N; }
static inline bool sameMove(const Move& a, const Move& b) { return a.from == b.from && a.to == b.to; }
static inline char lowerPiece(char ch) { return static_cast<char>(tolower(static_cast<unsigned char>(ch))); }
static inline Side otherSide(Side s) { return s == WHITE ? BLACK : WHITE; }
static inline int sideIndex(Side s) { return static_cast<int>(s); }
static inline bool isFriendly(char ch, Side s) {
    return ch != '.' && gPieceSide[(unsigned char)ch] == sideIndex(s);
}
static inline bool isEnemy(char ch, Side s) {
    return ch != '.' && gPieceSide[(unsigned char)ch] != 255 && gPieceSide[(unsigned char)ch] != sideIndex(s);
}

// ============================================================
// Piece Metadata Initialization
// ============================================================

static inline void registerPiece(char upper, char lower, PieceKind kind, int value, uint8_t whiteIndex, uint8_t blackIndex) {
    gMaterialValue[(unsigned char)upper] = value;
    gMaterialValue[(unsigned char)lower] = value;
    gSignedMaterial[(unsigned char)upper] = value;
    gSignedMaterial[(unsigned char)lower] = -value;

    gPieceKind[(unsigned char)upper] = kind;
    gPieceKind[(unsigned char)lower] = kind;
    gPieceSide[(unsigned char)upper] = WHITE;
    gPieceSide[(unsigned char)lower] = BLACK;
    gPieceIndex[(unsigned char)upper] = whiteIndex;
    gPieceIndex[(unsigned char)lower] = blackIndex;
}

static void initPieceTables() {
    memset(gMaterialValue, 0, sizeof(gMaterialValue));
    memset(gSignedMaterial, 0, sizeof(gSignedMaterial));
    memset(gPieceKind, 0, sizeof(gPieceKind));
    memset(gPieceSide, 255, sizeof(gPieceSide));
    memset(gPieceIndex, 0, sizeof(gPieceIndex));

    gPieceKind[(unsigned char)'.'] = PIECE_NONE;
    gPieceSide[(unsigned char)'.'] = 255;
    gPieceIndex[(unsigned char)'.'] = 0;

    registerPiece('B', 'b', BABY, static_cast<int>(CFG.babyValue), 1, 2);
    registerPiece('P', 'p', PRINCE, static_cast<int>(CFG.princeValue), 3, 4);
    registerPiece('X', 'x', PRINCESS, static_cast<int>(CFG.princessValue), 5, 6);
    registerPiece('Y', 'y', PONY, static_cast<int>(CFG.ponyValue), 7, 8);
    registerPiece('G', 'g', GUARD, static_cast<int>(CFG.guardValue), 9, 10);
    registerPiece('T', 't', TUTOR, static_cast<int>(CFG.tutorValue), 11, 12);
    registerPiece('S', 's', SCOUT, static_cast<int>(CFG.scoutValue), 13, 14);
    registerPiece('N', 'n', SIBLING, static_cast<int>(CFG.siblingValue), 15, 16);
}

static void initPstTables() {
    for (int k = 0; k < 9; ++k) {
        for (int s = 0; s < BOARD_SQ; ++s) {
            gPstValue[k][s] = 0;
        }
    }

    for (int s = 0; s < BOARD_SQ; ++s) {
        const int r = rowOf(s);
        const int c = colOf(s);
        const int advance = (N - 1) - r;
        const int center = 6 - abs(2 * c - (N - 1));
        gPstValue[BABY][s] = advance * CFG.pstBabyAdvanceWeight + center * CFG.pstBabyCenterWeight;
        gPstValue[PRINCE][s] = center * CFG.pstPrinceCenterWeight - advance * CFG.pstPrinceAdvanceWeight;
        gPstValue[SCOUT][s] = advance * CFG.pstScoutAdvanceWeight + center * CFG.pstScoutCenterWeight;
        gPstValue[SIBLING][s] = center * CFG.pstSiblingCenterWeight + advance * CFG.pstSiblingAdvanceWeight;
    }
}

static void initZobrist() {
    mt19937_64 rng(0xC0FFEE123456789ULL);
    for (int p = 0; p < 17; ++p) {
        for (int s = 0; s < BOARD_SQ; ++s) {
            gZobrist[p][s] = rng();
        }
    }
    gSideKey = rng();
}

static inline int pieceValue(char ch) {
    return gMaterialValue[(unsigned char)ch];
}

static inline int rootStaticEval() {
    return (gRootSide == WHITE) ? gWhiteEval : -gWhiteEval;
}

static inline int countPiecesForSide(Side s) {
    return gPieceCount[sideIndex(s)];
}

// ============================================================
// Draw-State Tracking, Recent Performance Buffer, and Leaf Scoring
// ============================================================

static inline double recentAverageMoveTimeSec() {
    const int window = max(1, min(CFG.recentMoveWindow, gRecentMoveTimesLen));
    if (window <= 0) return 0.0;

    double sum = 0.0;
    for (int i = gRecentMoveTimesLen - window; i < gRecentMoveTimesLen; ++i) {
        if (i >= 0) sum += gRecentMoveTimes[i];
    }
    return sum / window;
}

static inline double drawTimeBufferSec() {
    const double adaptive = CFG.drawTimeBufferBaseSec + recentAverageMoveTimeSec() * CFG.drawTimeBufferRecentScale;
    return max(0.03, min(0.12, adaptive));
}

static inline int repetitionCountForCurrentHash(int ply);
static inline bool canCaptureSquare(const Board& b, Side attacker, int targetSq);

// ============================================================
// Repetition Tracking and Anti-Draw Heuristics
// ============================================================

static inline bool currentNodeIsDraw(int ply) {
    return gHalfmoveClock >= 50 || repetitionCountForCurrentHash(ply) >= 3;
}

static inline bool currentNodeIsTwoFoldRepetition(int ply) {
    return repetitionCountForCurrentHash(ply) == 2;
}

static inline int drawOutcomeScore() {
    if (gMyTimeRemaining > gOppTimeRemaining) return static_cast<int>(CFG.drawTimeWinReward);
    if (gMyTimeRemaining < gOppTimeRemaining) return static_cast<int>(CFG.drawTimeLossPenalty);

    const int myPieces = countPiecesForSide(gRootSide);
    const int oppPieces = countPiecesForSide(otherSide(gRootSide));
    if (myPieces > oppPieces) return static_cast<int>(CFG.drawPieceWinReward);
    if (myPieces < oppPieces) return static_cast<int>(CFG.drawPieceLossPenalty);
    return static_cast<int>(CFG.drawTieScore);
}

static inline int timeSensitiveTwoFoldScore(int ply) {
    if (!CFG.avoidTwoFoldRepetition) return 0;
    if (!currentNodeIsTwoFoldRepetition(ply)) return 0;

    const double timeEdge = gMyTimeRemaining - gOppTimeRemaining;
    if (timeEdge <= CFG.twoFoldAvoidTimeGapSec) {
        // When time is tight or we are behind, steer away from 2-fold repetition.
        return -CFG.twoFoldRepetitionPenalty;
    }
    if (timeEdge >= CFG.twoFoldPreferTimeGapSec) {
        // When comfortably ahead on time, allow the search to lean toward repeatable lines.
        return CFG.twoFoldRepetitionBonus;
    }
    return 0;
}

static inline int mateScoreForPly(bool rootWon, int ply) {
    // Prefer faster wins and slower losses by encoding distance to mate.
    const int boundedPly = max(0, ply);
    return rootWon ? (MATE - boundedPly) : (-MATE + boundedPly);
}

static inline int princePressureScore() {
    if (!CFG.enablePrinceThreatAvoidance) return 0;

    const int myPrinceSq = (gRootSide == WHITE) ? gWhitePrinceSq : gBlackPrinceSq;
    const int enemyPrinceSq = (gRootSide == WHITE) ? gBlackPrinceSq : gWhitePrinceSq;
    const Side enemy = otherSide(gRootSide);

    int score = 0;
    if (enemyPrinceSq != -1 && canCaptureSquare(gBoard, gRootSide, enemyPrinceSq)) {
        // Small, cheap positional nudge for having the opponent's Prince under pressure.
        score += CFG.enemyPrinceThreatBonus;
    }
    if (myPrinceSq != -1 && canCaptureSquare(gBoard, enemy, myPrinceSq)) {
        score -= CFG.princeThreatPenalty;
    }
    return score;
}


static inline int leafEvalLikeOriginal(int ply, const Move* triggerMove = nullptr) {
    if (gRootSide == WHITE) {
        if (gWhitePrinceSq == -1) return mateScoreForPly(false, ply);
        if (gBlackPrinceSq == -1) return mateScoreForPly(true, ply);
    } else {
        if (gBlackPrinceSq == -1) return mateScoreForPly(false, ply);
        if (gWhitePrinceSq == -1) return mateScoreForPly(true, ply);
    }
    if (currentNodeIsDraw(ply)) {
        return drawOutcomeScore();
    }

    return rootStaticEval() + timeSensitiveTwoFoldScore(ply) + princePressureScore();
}

static inline int pieceContribution(char ch) {
    return gSignedMaterial[(unsigned char)ch];
}

static inline int pieceSquareContribution(char ch, int s) {
    if (ch == '.') return 0;
    const PieceKind kind = static_cast<PieceKind>(gPieceKind[(unsigned char)ch]);
    if (kind == PIECE_NONE) return 0;
    const Side side = static_cast<Side>(gPieceSide[(unsigned char)ch]);
    const int pstSq = (side == WHITE) ? s : mirrorSq(s);
    const int pst = gPstValue[static_cast<int>(kind)][pstSq];
    return (side == WHITE) ? pst : -pst;
}

static inline int pieceEvalContribution(char ch, int s) {
    return pieceContribution(ch) + pieceSquareContribution(ch, s);
}


// ============================================================
// Draw-State Helper Routines for Search Memory
// ============================================================

static inline void pushRecentMoveTime(double seconds) {
    if (seconds < 0.0) seconds = 0.0;
    if (gRecentMoveTimesLen < MAX_RECENT_PERF_SAMPLES) {
        gRecentMoveTimes[gRecentMoveTimesLen++] = seconds;
    } else {
        for (int i = 1; i < MAX_RECENT_PERF_SAMPLES; ++i) {
            gRecentMoveTimes[i - 1] = gRecentMoveTimes[i];
        }
        gRecentMoveTimes[MAX_RECENT_PERF_SAMPLES - 1] = seconds;
        gRecentMoveTimesLen = MAX_RECENT_PERF_SAMPLES;
    }
}

static inline void recordHistoryHash(uint64_t hash) {
    ++gHistoryCounts[hash];
}

static inline int repetitionCountForCurrentHash(int ply) {
    (void)ply;
    int count = 0;
    auto it = gHistoryCounts.find(gHash);
    if (it != gHistoryCounts.end()) count += it->second;
    for (int i = 0; i < gSearchHashStackLen; ++i) {
        if (gSearchHashStack[i] == gHash) ++count;
    }
    return count;
}

// ============================================================
// Time Control and Terminal Checks
// ============================================================

static inline bool outOfTime() {
    if (gTimedOut) return true;
    if ((gNodeCount & 255u) == 0u && chrono::steady_clock::now() >= gDeadline) {
        gTimedOut = true;
    }
    return gTimedOut;
}

static inline bool terminalState() {
    return gWhitePrinceSq == -1 || gBlackPrinceSq == -1;
}

// ============================================================
// Piece-List Helpers
// ============================================================

static inline void resetPieceLists() {
    for (int s = 0; s < 2; ++s) {
        gPieceCount[s] = 0;
        gPiecePos[s].fill(255);
    }
}

static inline void addPieceToList(Side s, uint8_t square) {
    const int si = sideIndex(s);
    const int idx = gPieceCount[si]++;
    gPieceList[si][idx] = square;
    gPiecePos[si][square] = static_cast<uint8_t>(idx);
}

static inline void rebuildPieceListsFromBoard() {
    resetPieceLists();
    for (int s = 0; s < BOARD_SQ; ++s) {
        const char ch = gBoard[s];
        if (ch == '.') continue;
        const Side side = static_cast<Side>(gPieceSide[(unsigned char)ch]);
        addPieceToList(side, static_cast<uint8_t>(s));
    }
}

// ============================================================
// Precomputed Move Geometry
// ============================================================

static inline void addTemplate(int sideIdx, PieceKind kind, int from, int to, int pathLen, int path0, int path1, int baseOrder) {
    TemplateList& tl = gMoveTemplates[sideIdx][static_cast<int>(kind)][from];
    if (tl.size >= MAX_TEMPLATES_PER_LIST) return;
    MoveTemplate& mt = tl.items[tl.size++];
    mt.from = static_cast<uint8_t>(from);
    mt.to = static_cast<uint8_t>(to);
    mt.pathLen = static_cast<uint8_t>(pathLen);
    mt.path0 = static_cast<uint8_t>(path0);
    mt.path1 = static_cast<uint8_t>(path1);
    mt.baseOrder = baseOrder;
}

static void initMoveTemplates() {
    for (int side = 0; side < 2; ++side) {
        for (int kind = 0; kind < 9; ++kind) {
            for (int s = 0; s < BOARD_SQ; ++s) {
                gMoveTemplates[side][kind][s].size = 0;
            }
        }
    }

    for (int s = 0; s < BOARD_SQ; ++s) {
        const int sr = rowOf(s);
        const int sc = colOf(s);

        for (int side = 0; side < 2; ++side) {
            const int f = (side == WHITE) ? -1 : 1;

            // Baby: 1-2 straight forward.
            for (int step = 1; step <= 2; ++step) {
                const int dr = sr + f * step;
                const int dc = sc;
                if (!inBounds(dr, dc)) continue;
                const int to = sq(dr, dc);
                const int pathLen = (step == 1 ? 0 : 1);
                const int mid = (step == 1 ? -1 : sq(sr + f, sc));
                addTemplate(side, BABY, s, to, pathLen, mid, -1, 10 - step);
            }

            // Prince: one step in any direction.
            for (int dr = -1; dr <= 1; ++dr) {
                for (int dc = -1; dc <= 1; ++dc) {
                    if (dr == 0 && dc == 0) continue;
                    const int nr = sr + dr;
                    const int nc = sc + dc;
                    if (!inBounds(nr, nc)) continue;
                    addTemplate(side, PRINCE, s, sq(nr, nc), 0, -1, -1, 5);
                }
            }

            // Princess: 1-3 in any direction, no jumping.
            for (int dr = -1; dr <= 1; ++dr) {
                for (int dc = -1; dc <= 1; ++dc) {
                    if (dr == 0 && dc == 0) continue;
                    for (int step = 1; step <= 3; ++step) {
                        const int nr = sr + dr * step;
                        const int nc = sc + dc * step;
                        if (!inBounds(nr, nc)) break;
                        const int pathLen = step - 1;
                        const int p0 = (step >= 2) ? sq(sr + dr, sc + dc) : -1;
                        const int p1 = (step >= 3) ? sq(sr + dr * 2, sc + dc * 2) : -1;
                        addTemplate(side, PRINCESS, s, sq(nr, nc), pathLen, p0, p1, 4);
                    }
                }
            }

            // Pony: one step diagonally.
            for (int dr : {-1, 1}) {
                for (int dc : {-1, 1}) {
                    const int nr = sr + dr;
                    const int nc = sc + dc;
                    if (!inBounds(nr, nc)) continue;
                    addTemplate(side, PONY, s, sq(nr, nc), 0, -1, -1, 6);
                }
            }

            // Guard: up to 2 orthogonal, no jumping.
            static constexpr int ortho[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            for (const auto& d : ortho) {
                for (int step = 1; step <= 2; ++step) {
                    const int nr = sr + d[0] * step;
                    const int nc = sc + d[1] * step;
                    if (!inBounds(nr, nc)) break;
                    const int p0 = (step >= 2) ? sq(sr + d[0], sc + d[1]) : -1;
                    addTemplate(side, GUARD, s, sq(nr, nc), step - 1, p0, -1, 4 - step);
                }
            }

            // Tutor: up to 2 diagonal, no jumping.
            static constexpr int diag[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
            for (const auto& d : diag) {
                for (int step = 1; step <= 2; ++step) {
                    const int nr = sr + d[0] * step;
                    const int nc = sc + d[1] * step;
                    if (!inBounds(nr, nc)) break;
                    const int p0 = (step >= 2) ? sq(sr + d[0], sc + d[1]) : -1;
                    addTemplate(side, TUTOR, s, sq(nr, nc), step - 1, p0, -1, 4 - step);
                }
            }

            // Scout: 1-3 forward with optional one-square sideways drift, jumping allowed.
            for (int step = 1; step <= 3; ++step) {
                const int nr = sr + f * step;
                if (!inBounds(nr, sc)) break;
                addTemplate(side, SCOUT, s, sq(nr, sc), 0, -1, -1, 20 - step);
                if (sc - 1 >= 0) addTemplate(side, SCOUT, s, sq(nr, sc - 1), 0, -1, -1, 20 - step);
                if (sc + 1 < N) addTemplate(side, SCOUT, s, sq(nr, sc + 1), 0, -1, -1, 20 - step);
            }

            // Sibling: one step in any direction, must end adjacent to a friendly piece.
            for (int dr = -1; dr <= 1; ++dr) {
                for (int dc = -1; dc <= 1; ++dc) {
                    if (dr == 0 && dc == 0) continue;
                    const int nr = sr + dr;
                    const int nc = sc + dc;
                    if (!inBounds(nr, nc)) continue;
                    addTemplate(side, SIBLING, s, sq(nr, nc), 0, -1, -1, 3);
                }
            }
        }
    }
}

static inline bool siblingHasFriendlyNeighborAfterMove(const Board& b, Side s, int sr, int sc, int dr, int dc) {
    for (int rr = dr - 1; rr <= dr + 1; ++rr) {
        for (int cc = dc - 1; cc <= dc + 1; ++cc) {
            if (!inBounds(rr, cc)) continue;
            if (rr == dr && cc == dc) continue;
            if (rr == sr && cc == sc) continue;
            if (isFriendly(b[sq(rr, cc)], s)) return true;
        }
    }
    return false;
}

static inline bool pathClear(const Board& b, const MoveTemplate& mt) {
    if (mt.pathLen >= 1 && b[mt.path0] != '.') return false;
    if (mt.pathLen >= 2 && b[mt.path1] != '.') return false;
    return true;
}

// ============================================================
// Tactical Capture Ordering Helpers (SEE-lite)
// ============================================================

static inline bool squareOccupiedAfterMove(const Board& b, int sqIdx, int emptyA, int emptyB) {
    return sqIdx != emptyA && sqIdx != emptyB && b[sqIdx] != '.';
}

static inline bool pathClearAfterMove(const Board& b, const MoveTemplate& mt, int emptyA, int emptyB) {
    if (mt.pathLen >= 1 && squareOccupiedAfterMove(b, mt.path0, emptyA, emptyB)) return false;
    if (mt.pathLen >= 2 && squareOccupiedAfterMove(b, mt.path1, emptyA, emptyB)) return false;
    return true;
}

static inline bool siblingHasFriendlyNeighborAfterMove(const Board& b, Side s, int sr, int sc, int dr, int dc, int emptyA, int emptyB) {
    for (int rr = dr - 1; rr <= dr + 1; ++rr) {
        for (int cc = dc - 1; cc <= dc + 1; ++cc) {
            if (!inBounds(rr, cc)) continue;
            if (rr == dr && cc == dc) continue;
            if (rr == sr && cc == sc) continue;
            const int sqIdx = sq(rr, cc);
            if (!squareOccupiedAfterMove(b, sqIdx, emptyA, emptyB)) continue;
            if (isFriendly(b[sqIdx], s)) return true;
        }
    }
    return false;
}

static inline bool canCaptureSquareAfterMove(const Board& b, Side attacker, int targetSq, int emptyA, int emptyB) {
    if (targetSq < 0 || targetSq >= BOARD_SQ) return false;

    const char target = b[targetSq];
    if (target != '.' && !isEnemy(target, attacker)) return false;

    const int si = sideIndex(attacker);
    for (int pi = 0; pi < gPieceCount[si]; ++pi) {
        const int from = gPieceList[si][pi];
        const char ch = b[from];
        if (!isFriendly(ch, attacker)) continue;

        const PieceKind kind = static_cast<PieceKind>(gPieceKind[(unsigned char)ch]);
        const TemplateList& tl = gMoveTemplates[si][static_cast<int>(kind)][from];
        for (int i = 0; i < tl.size; ++i) {
            const MoveTemplate& mt = tl.items[i];
            if (mt.to != targetSq) continue;
            if (!pathClearAfterMove(b, mt, emptyA, emptyB)) continue;
            if (kind == SIBLING && !siblingHasFriendlyNeighborAfterMove(b, attacker, rowOf(from), colOf(from), rowOf(mt.to), colOf(mt.to), emptyA, emptyB)) continue;
            return true;
        }
    }
    return false;
}

static inline int captureOrderScore(const Board& b, Side moverSide, const MoveTemplate& mt, char mover, char captured) {
    const int victimValue = pieceValue(captured);
    const int moverValue = pieceValue(mover);
    int order = CFG.captureOrderThreshold + victimValue * 8 + mt.baseOrder;

    if (gPieceKind[(unsigned char)captured] == PRINCE) {
        return order + CFG.seePrinceCaptureBonus;
    }

    const bool recapturable = canCaptureSquareAfterMove(b, otherSide(moverSide), mt.to, mt.from, mt.to);
    if (recapturable) order -= moverValue * CFG.seeRecapturePenaltyScale;
    else order += CFG.seeSafeCaptureBonus;
    return order;
}

// ============================================================
// Move-Learning Helpers
// ============================================================

static inline int moveIndexCode(const Move& m) {
    return static_cast<int>(m.from) * BOARD_SQ + static_cast<int>(m.to);
}

static inline bool isValidStoredMove(const Move& m) {
    return m.from != 255 && m.to != 255;
}

static inline void clearMoveLearningTables() {
    for (auto& row : gKillerMoves) {
        row[0] = Move{255, 255, 0};
        row[1] = Move{255, 255, 0};
    }
    for (auto& sideTable : gCounterMoves) {
        for (auto& entry : sideTable) {
            entry = Move{255, 255, 0};
        }
    }
    for (auto& sideTable : gHistoryScores) {
        for (auto& row : sideTable) {
            row.fill(0);
        }
    }
}

static inline void storeKillerMove(int ply, const Move& m) {
    if (ply < 0 || ply >= MAX_SEARCH_PLY) return;
    if (sameMove(gKillerMoves[ply][0], m)) return;
    if (sameMove(gKillerMoves[ply][1], m)) return;
    gKillerMoves[ply][1] = gKillerMoves[ply][0];
    gKillerMoves[ply][0] = m;
}

static inline int moveLearningBonus(int ply, const Move* triggerMove, const Move& m) {
    if (!CFG.enableMoveLearning) return 0;

    int bonus = 0;
    if (ply >= 0 && ply < MAX_SEARCH_PLY) {
        if (sameMove(gKillerMoves[ply][0], m)) bonus += CFG.killerMoveBonus;
        else if (sameMove(gKillerMoves[ply][1], m)) bonus += CFG.secondKillerMoveBonus;
    }

    const int si = sideIndex(gSideToMove);
    bonus += min(CFG.historyMoveCap, gHistoryScores[si][m.from][m.to] / max(1, CFG.historyMoveScale));

    if (triggerMove) {
        const int prevCode = moveIndexCode(*triggerMove);
        if (sameMove(gCounterMoves[si][prevCode], m)) bonus += CFG.countermoveBonus;
    }

    return bonus;
}

static inline void updateMoveLearning(const Move& m, int depth, const Move* triggerMove) {
    if (!CFG.enableMoveLearning) return;
    if (depth <= 0) return;

    const bool isCapture = gBoard[m.to] != '.';
    if (isCapture) return;

    const int si = sideIndex(gSideToMove);
    const int gain = depth * depth;
    int& hist = gHistoryScores[si][m.from][m.to];
    hist = min(CFG.historyMoveCap, hist + gain);

    storeKillerMove(depth, m);

    if (triggerMove) {
        gCounterMoves[si][moveIndexCode(*triggerMove)] = m;
    }
}

// ============================================================
// Move Ordering Helpers
// ============================================================

static inline void orderMovesHeuristic(MoveList& out) {
    array<Move, MAX_MOVES> captures{};
    array<Move, MAX_MOVES> quiets{};
    int captureCount = 0;
    int quietCount = 0;

    for (int i = 0; i < out.size; ++i) {
        if (out.moves[i].order >= CFG.captureOrderThreshold) captures[captureCount++] = out.moves[i];
        else quiets[quietCount++] = out.moves[i];
    }

    // Captures only: a small insertion sort is usually cheaper than sorting the whole move list.
    for (int i = 1; i < captureCount; ++i) {
        Move key = captures[i];
        int j = i - 1;
        while (j >= 0 && captures[j].order < key.order) {
            captures[j + 1] = captures[j];
            --j;
        }
        captures[j + 1] = key;
    }

    // Quiet moves are also lightly ordered so killer / history / countermove scores matter.
    for (int i = 1; i < quietCount; ++i) {
        Move key = quiets[i];
        int j = i - 1;
        while (j >= 0 && quiets[j].order < key.order) {
            quiets[j + 1] = quiets[j];
            --j;
        }
        quiets[j + 1] = key;
    }

    int idx = 0;
    for (int i = 0; i < captureCount; ++i) out.moves[idx++] = captures[i];
    for (int i = 0; i < quietCount; ++i) out.moves[idx++] = quiets[i];
    out.size = idx;
}

static inline bool moveExistsInList(const MoveList& list, const Move& m) {
    for (int i = 0; i < list.size; ++i) {
        if (sameMove(list.moves[i], m)) return true;
    }
    return false;
}

// ============================================================
// Legal Move Generation
// ============================================================

static void generateMoves(const Board& b, Side s, MoveList& out, int ply, const Move* triggerMove) {
    out.size = 0;
    const int si = sideIndex(s);

    // Use the maintained piece lists instead of scanning the whole board.
    for (int pi = 0; pi < gPieceCount[si]; ++pi) {
        const int from = gPieceList[si][pi];
        const char ch = b[from];
        if (!isFriendly(ch, s)) continue;

        const PieceKind kind = static_cast<PieceKind>(gPieceKind[(unsigned char)ch]);
        const TemplateList& tl = gMoveTemplates[si][static_cast<int>(kind)][from];

        for (int i = 0; i < tl.size; ++i) {
            const MoveTemplate& mt = tl.items[i];
            const char dest = b[mt.to];
            if (isFriendly(dest, s)) continue;
            if (!pathClear(b, mt)) continue;
            if (kind == SIBLING && !siblingHasFriendlyNeighborAfterMove(b, s, rowOf(from), colOf(from), rowOf(mt.to), colOf(mt.to))) continue;

            Move m;
            m.from = mt.from;
            m.to = mt.to;
            m.order = mt.baseOrder;
            if (dest != '.') {
                m.order = captureOrderScore(b, s, mt, ch, dest);
            } else {
                m.order += moveLearningBonus(ply, triggerMove, m);
            }
            if (out.size < MAX_MOVES) out.moves[out.size++] = m;
        }
    }

    // Keep the move list light-weightly ordered without a full sort.
    orderMovesHeuristic(out);
}


// ============================================================
// Quiescence Search Move Generation
// ============================================================

static void generateCaptureMoves(const Board& b, Side s, MoveList& out) {
    out.size = 0;
    const int si = sideIndex(s);

    // Only expand captures so the quiescence layer stays small and tactical.
    for (int pi = 0; pi < gPieceCount[si]; ++pi) {
        const int from = gPieceList[si][pi];
        const char ch = b[from];
        if (!isFriendly(ch, s)) continue;

        const PieceKind kind = static_cast<PieceKind>(gPieceKind[(unsigned char)ch]);
        const TemplateList& tl = gMoveTemplates[si][static_cast<int>(kind)][from];

        for (int i = 0; i < tl.size; ++i) {
            const MoveTemplate& mt = tl.items[i];
            const char dest = b[mt.to];
            if (!isEnemy(dest, s)) continue;
            if (!pathClear(b, mt)) continue;
            if (kind == SIBLING && !siblingHasFriendlyNeighborAfterMove(b, s, rowOf(from), colOf(from), rowOf(mt.to), colOf(mt.to))) continue;

            Move m;
            m.from = mt.from;
            m.to = mt.to;
            m.order = captureOrderScore(b, s, mt, ch, dest);
            if (out.size < MAX_MOVES) out.moves[out.size++] = m;
        }
    }

    orderMovesHeuristic(out);
}

// ============================================================
// Selective Depth Extension Helpers
// ============================================================

static inline int totalPieceCount() {
    return gPieceCount[WHITE] + gPieceCount[BLACK];
}

static inline int princeSquareForSide(Side s) {
    return (s == WHITE) ? gWhitePrinceSq : gBlackPrinceSq;
}

static inline bool canCaptureSquare(const Board& b, Side attacker, int targetSq) {
    if (targetSq < 0 || targetSq >= BOARD_SQ) return false;
    const char target = b[targetSq];
    if (!isEnemy(target, attacker)) return false;

    const int si = sideIndex(attacker);
    for (int pi = 0; pi < gPieceCount[si]; ++pi) {
        const int from = gPieceList[si][pi];
        const char ch = b[from];
        if (!isFriendly(ch, attacker)) continue;

        const PieceKind kind = static_cast<PieceKind>(gPieceKind[(unsigned char)ch]);
        const TemplateList& tl = gMoveTemplates[si][static_cast<int>(kind)][from];
        for (int i = 0; i < tl.size; ++i) {
            const MoveTemplate& mt = tl.items[i];
            if (mt.to != targetSq) continue;
            if (!pathClear(b, mt)) continue;
            if (kind == SIBLING && !siblingHasFriendlyNeighborAfterMove(b, attacker, rowOf(from), colOf(from), rowOf(mt.to), colOf(mt.to))) continue;
            return true;
        }
    }
    return false;
}

static inline int rootPhaseExtensionBonus() {
    if (!CFG.enableDepthExtensions) return 0;
    return (totalPieceCount() <= CFG.phaseEndgamePieceThreshold) ? CFG.phaseEndgameRootBonus : 0;
}

static inline int rootAdvantageExtensionBonus() {
    if (!CFG.enableDepthExtensions) return 0;
    return (abs(rootStaticEval()) >= CFG.advantageEvalMargin) ? CFG.advantageRootBonus : 0;
}

static inline int nodeSelectiveExtensionBonus(const MoveList& moves, int depth) {
    if (!CFG.enableDepthExtensions) return 0;
    if (depth > CFG.extensionTriggerDepth) return 0;

    const Side us = gSideToMove;
    const Side them = otherSide(us);
    const int ourPrince = princeSquareForSide(us);
    const int theirPrince = princeSquareForSide(them);

    const bool ourPrinceThreatened = (ourPrince != -1) && canCaptureSquare(gBoard, them, ourPrince);
    const bool theirPrinceThreatened = (theirPrince != -1) && canCaptureSquare(gBoard, us, theirPrince);

    // Keep extensions tightly focused on immediate Prince races.
    if (!ourPrinceThreatened && !theirPrinceThreatened) return 0;

    int bonus = 0;
    if (ourPrinceThreatened) {
        bonus += CFG.princeThreatExtensionBonus;
    }
    if (theirPrinceThreatened) {
        bonus += CFG.tacticalThreatExtensionBonus;
    }
    if (moves.size <= CFG.mobilityCrunchMoveThreshold) {
        bonus += CFG.mobilityCrunchExtensionBonus;
    }

    return min(bonus, CFG.maxTotalExtensionPerPath);
}

// ============================================================
// Hard Total-Depth Cap Helpers
// ============================================================

static inline int hardDepthRemaining(int ply) {
    return max(0, CFG.maxTotalSearchDepth - ply);
}

// ============================================================
// Make / Unmake Move
// ============================================================

static inline Undo makeMove(const Move& m) {
    Undo u;
    u.from = m.from;
    u.to = m.to;
    u.mover = gBoard[m.from];
    u.captured = gBoard[m.to];
    u.prevWhiteEval = gWhiteEval;
    u.prevHash = gHash;
    u.prevWhitePrinceSq = gWhitePrinceSq;
    u.prevBlackPrinceSq = gBlackPrinceSq;
    u.prevSideToMove = gSideToMove;
    u.prevHalfmoveClock = gHalfmoveClock;
    u.prevWhiteCount = static_cast<uint8_t>(gPieceCount[WHITE]);
    u.prevBlackCount = static_cast<uint8_t>(gPieceCount[BLACK]);

    const Side moverSide = gSideToMove;
    const Side enemySide = otherSide(moverSide);
    const int moverSI = sideIndex(moverSide);
    const int enemySI = sideIndex(enemySide);

    // Incremental evaluation (material + PST).
    gWhiteEval -= pieceEvalContribution(u.mover, m.from);
    gWhiteEval += pieceEvalContribution(u.mover, m.to);
    if (u.captured != '.') {
        gWhiteEval -= pieceEvalContribution(u.captured, m.to);
    }

    // Incremental Zobrist hash.
    const int moverPI = gPieceIndex[(unsigned char)u.mover];
    const int destPI = gPieceIndex[(unsigned char)u.captured];
    gHash ^= gZobrist[moverPI][m.from];
    gHash ^= gZobrist[moverPI][m.to];
    if (u.captured != '.') {
        gHash ^= gZobrist[destPI][m.to];
    }
    gHash ^= gSideKey;

    // Board update.
    gBoard[m.to] = u.mover;
    gBoard[m.from] = '.';

    // Halfmove clock updates for repetition / 50-move draw tracking.
    if (u.captured != '.' || lowerPiece(u.mover) == 'b') gHalfmoveClock = 0;
    else ++gHalfmoveClock;

    // Piece-list update for the mover.
    u.moverListIndex = gPiecePos[moverSI][m.from];
    gPieceList[moverSI][u.moverListIndex] = m.to;
    gPiecePos[moverSI][m.to] = u.moverListIndex;
    gPiecePos[moverSI][m.from] = 255;

    // Piece-list update for the captured piece, if any.
    if (u.captured != '.') {
        u.capturedListIndex = gPiecePos[enemySI][m.to];
        const uint8_t enemyLastIdx = static_cast<uint8_t>(gPieceCount[enemySI] - 1);
        const uint8_t enemyLastSq = gPieceList[enemySI][enemyLastIdx];

        if (u.capturedListIndex != enemyLastIdx) {
            gPieceList[enemySI][u.capturedListIndex] = enemyLastSq;
            gPiecePos[enemySI][enemyLastSq] = u.capturedListIndex;
        }
        gPiecePos[enemySI][m.to] = 255;
        --gPieceCount[enemySI];
    }

    // Prince location update.
    if (u.mover == 'P') gWhitePrinceSq = m.to;
    else if (u.mover == 'p') gBlackPrinceSq = m.to;
    if (u.captured == 'P') gWhitePrinceSq = -1;
    else if (u.captured == 'p') gBlackPrinceSq = -1;

    gSideToMove = otherSide(gSideToMove);
    if (gSearchHashStackLen < MAX_GAME_HISTORY) {
        gSearchHashStack[gSearchHashStackLen++] = gHash;
    }
    return u;
}

static inline void unmakeMove(const Undo& u) {
    const Side moverSide = u.prevSideToMove;
    const Side enemySide = otherSide(moverSide);
    const int moverSI = sideIndex(moverSide);
    const int enemySI = sideIndex(enemySide);

    gSideToMove = u.prevSideToMove;
    gWhiteEval = u.prevWhiteEval;
    gHash = u.prevHash;
    gWhitePrinceSq = u.prevWhitePrinceSq;
    gBlackPrinceSq = u.prevBlackPrinceSq;
    gHalfmoveClock = u.prevHalfmoveClock;
    gPieceCount[WHITE] = u.prevWhiteCount;
    gPieceCount[BLACK] = u.prevBlackCount;
    if (gSearchHashStackLen > 0) --gSearchHashStackLen;

    // Restore board.
    gBoard[u.from] = u.mover;
    gBoard[u.to] = u.captured;

    // Restore mover piece-list entry.
    gPieceList[moverSI][u.moverListIndex] = u.from;
    gPiecePos[moverSI][u.from] = u.moverListIndex;
    gPiecePos[moverSI][u.to] = 255;

    // Restore captured piece-list entry, if a capture happened.
    if (u.captured != '.') {
        const uint8_t restoredLastIdx = static_cast<uint8_t>(gPieceCount[enemySI] - 1);
        if (u.capturedListIndex != restoredLastIdx) {
            const uint8_t movedSq = gPieceList[enemySI][u.capturedListIndex];
            gPieceList[enemySI][restoredLastIdx] = movedSq;
            gPiecePos[enemySI][movedSq] = restoredLastIdx;
        }
        gPieceList[enemySI][u.capturedListIndex] = u.to;
        gPiecePos[enemySI][u.to] = u.capturedListIndex;
    }
}

// ============================================================
// Transposition Table Helpers
// ============================================================

static inline bool isMateScore(int score) {
    // Mate scores are encoded near +/- MATE, while ordinary evaluation and
    // draw scores stay much farther away from that range.
    return score >= (MATE - 100000) || score <= (-MATE + 100000);
}

static inline int normalizeMateScoreForTT(int score, int ply) {
    if (isMateScore(score)) {
        return (score > 0) ? (score + ply) : (score - ply);
    }
    return score;
}

static inline int denormalizeMateScoreFromTT(int score, int ply) {
    if (isMateScore(score)) {
        return (score > 0) ? (score - ply) : (score + ply);
    }
    return score;
}

static inline TTEntry* ttProbe(uint64_t key) {
    const size_t base = static_cast<size_t>(key) & TT_MASK;
    TTEntry* bestEmpty = nullptr;
    for (size_t i = 0; i < 4; ++i) {
        TTEntry& e = gTT[(base + i) & TT_MASK];
        if (e.key == key) return &e;
        if (e.key == 0 && bestEmpty == nullptr) bestEmpty = &e;
    }
    return bestEmpty;
}

static inline void ttStore(uint64_t key, int depth, int score, uint8_t flag, const Move* bestMove, int ply) {
    const size_t base = static_cast<size_t>(key) & TT_MASK;
    TTEntry* slot = nullptr;
    TTEntry* bestReplacement = nullptr;
    int bestReplacementDepth = INT_MAX;
    uint16_t bestReplacementAge = 0;

    for (size_t i = 0; i < 4; ++i) {
        TTEntry& e = gTT[(base + i) & TT_MASK];
        if (e.key == key) {
            slot = &e;
            break;
        }
        if (e.key == 0) {
            slot = &e;
            break;
        }

        const uint16_t age = static_cast<uint16_t>(gTTGeneration - e.generation);
        if (bestReplacement == nullptr || e.depth < bestReplacementDepth || (e.depth == bestReplacementDepth && age > bestReplacementAge)) {
            bestReplacement = &e;
            bestReplacementDepth = e.depth;
            bestReplacementAge = age;
        }
    }

    if (!slot) slot = bestReplacement ? bestReplacement : &gTT[base];

    if (slot->key != 0 && slot->key != key) {
        const uint16_t age = static_cast<uint16_t>(gTTGeneration - slot->generation);
        if (slot->depth > depth && age <= static_cast<uint16_t>(max(0, CFG.ttAgeLimit))) return;
    }

    slot->key = key;
    slot->depth = depth;
    slot->score = normalizeMateScoreForTT(score, ply);
    slot->flag = flag;
    slot->generation = gTTGeneration;
    if (bestMove) {
        slot->bestFrom = bestMove->from;
        slot->bestTo = bestMove->to;
    } else {
        slot->bestFrom = 255;
        slot->bestTo = 255;
    }
}

// ============================================================
// Quiescence Search
// ============================================================

static int quiescence(int alpha, int beta, int qDepth, int ply, const Move* triggerMove = nullptr) {
    ++gNodeCount;
    if (outOfTime()) {
        return leafEvalLikeOriginal(ply, triggerMove);
    }

    const int hardRemaining = hardDepthRemaining(ply);
    if (hardRemaining <= 0) {
        return leafEvalLikeOriginal(ply, triggerMove);
    }

    qDepth = min(qDepth, hardRemaining);
    if (!CFG.enableQSearch || qDepth <= 0) {
        return leafEvalLikeOriginal(ply, triggerMove);
    }

    if (terminalState()) {
        return leafEvalLikeOriginal(ply, triggerMove);
    }

    if (currentNodeIsDraw(ply)) {
        return leafEvalLikeOriginal(ply, triggerMove);
    }

    if (gQSearchNodeCount >= static_cast<uint64_t>(max(0, CFG.qSearchMaxNodes))) {
        return leafEvalLikeOriginal(ply, triggerMove);
    }
    ++gQSearchNodeCount;

    const int standPat = leafEvalLikeOriginal(ply, triggerMove);

    if (gSideToMove == gRootSide) {
        if (standPat >= beta) return standPat;
        alpha = max(alpha, standPat);
    } else {
        if (standPat <= alpha) return standPat;
        beta = min(beta, standPat);
    }

    MoveList moves;
    generateCaptureMoves(gBoard, gSideToMove, moves);
    if (moves.size == 0) {
        return standPat;
    }

    if (gSideToMove == gRootSide) {
        int bestScore = standPat;
        for (int i = 0; i < moves.size; ++i) {
            const Move& m = moves.moves[i];
            const int capturedValue = pieceValue(gBoard[m.to]);
            if (standPat + capturedValue + CFG.qSearchDeltaMargin <= alpha) continue;

            Undo u = makeMove(m);
            const int val = quiescence(alpha, beta, qDepth - 1, ply + 1, &m);
            unmakeMove(u);
            if (val > bestScore) bestScore = val;
            alpha = max(alpha, bestScore);
            if (alpha >= beta) break;
            if (gTimedOut) break;
        }
        return bestScore;
    } else {
        int bestScore = standPat;
        for (int i = 0; i < moves.size; ++i) {
            const Move& m = moves.moves[i];
            const int capturedValue = pieceValue(gBoard[m.to]);
            if (standPat - capturedValue - CFG.qSearchDeltaMargin >= beta) continue;

            Undo u = makeMove(m);
            const int val = quiescence(alpha, beta, qDepth - 1, ply + 1, &m);
            unmakeMove(u);
            if (val < bestScore) bestScore = val;
            beta = min(beta, bestScore);
            if (alpha >= beta) break;
            if (gTimedOut) break;
        }
        return bestScore;
    }
}

// ============================================================
// Principal Variation Search (PVS) Helpers
// ============================================================

static inline bool usePVSAtNode(int depth, int moveCount) {
    return CFG.enablePVS && depth >= CFG.pvsMinDepth && moveCount > 1;
}

// ============================================================
// Late Move Reduction Helpers
// ============================================================

static inline int lmrReduction(int depth, int moveIndex, int moveCount, const Move& m, bool isPVSNode, bool isCapture) {
    if (!CFG.enableLMR) return 0;
    if (isCapture) return 0;
    if (depth < CFG.lmrMinDepth) return 0;
    if (moveIndex < CFG.lmrQuietMoveStart) return 0;
    if (isPVSNode && moveIndex == 0) return 0;

    int reduction = CFG.lmrBaseReduction;
    reduction += max(0, moveIndex - CFG.lmrQuietMoveStart) / max(1, CFG.lmrMoveCountScale);
    if (moveCount >= 16) ++reduction;
    if (moveCount >= 28) ++reduction;
    if (m.order >= CFG.captureOrderThreshold + 3000) reduction = max(0, reduction - 1);

    const char mover = gBoard[m.from];
    switch (static_cast<PieceKind>(gPieceKind[(unsigned char)mover])) {
        case BABY:
            reduction += CFG.lmrBabyExtraReduction;
            break;
        case PRINCE:
            reduction = max(0, reduction - CFG.lmrPrinceReductionBonus);
            break;
        case PRINCESS:
            reduction = max(0, reduction - CFG.lmrPrincessReductionBonus);
            break;
        case SCOUT:
            reduction = max(0, reduction - CFG.lmrScoutReductionBonus);
            break;
        case SIBLING:
            reduction = max(0, reduction - CFG.lmrSiblingReductionBonus);
            break;
        default:
            break;
    }

    reduction = min(reduction, CFG.lmrMaxReduction);
    reduction = min(reduction, depth - 1);
    return max(0, reduction);
}

// ============================================================
// Alpha-Beta Search
// ============================================================

static int minimax(int depth, int alpha, int beta, int ply, int extBudget, const Move* triggerMove = nullptr, Move* bestMoveOut = nullptr) {
    (void)ply;
    ++gNodeCount;
    if (outOfTime()) {
        return leafEvalLikeOriginal(ply, triggerMove);
    }

    if (terminalState()) {
        return leafEvalLikeOriginal(ply, triggerMove);
    }

    // Treat draw states as terminal immediately so the score is applied
    // at the first node that reaches repetition or the 50-move rule.
    if (currentNodeIsDraw(ply)) {
        return leafEvalLikeOriginal(ply, triggerMove);
    }

    const int hardRemaining = hardDepthRemaining(ply);
    if (hardRemaining <= 0) {
        return leafEvalLikeOriginal(ply, triggerMove);
    }

    if (depth > hardRemaining) {
        depth = hardRemaining;
    }

    if (depth == 0) {
        if (CFG.enableQSearch) {
            return quiescence(alpha, beta, min(CFG.qSearchMaxDepth, hardRemaining), ply, triggerMove);
        }
        return leafEvalLikeOriginal(ply, triggerMove);
    }

    const int alphaOrig = alpha;
    const int betaOrig = beta;
    const bool isMaxNode = (gSideToMove == gRootSide);

    Move ttMove;
    bool hasTTMove = false;
    if (TTEntry* e = ttProbe(gHash); e && e->key == gHash && e->depth >= depth) {
        const int ttScore = denormalizeMateScoreFromTT(e->score, ply);
        if (e->bestFrom != 255) {
            ttMove.from = e->bestFrom;
            ttMove.to = e->bestTo;
            hasTTMove = true;
        }
        if (e->flag == 0) return ttScore;
        if (e->flag == 1) alpha = max(alpha, ttScore);
        else if (e->flag == 2) beta = min(beta, ttScore);
        if (alpha >= beta) return ttScore;
    }

    MoveList moves;
    generateMoves(gBoard, gSideToMove, moves, ply, triggerMove);

    if (extBudget > 0 && depth <= CFG.extensionTriggerDepth) {
        int ext = min(extBudget, nodeSelectiveExtensionBonus(moves, depth));
        ext = min(ext, hardRemaining - depth);
        if (ext > 0) {
            depth += ext;
            extBudget -= ext;
        }
    }

    if (moves.size == 0) {
        return leafEvalLikeOriginal(ply, triggerMove);
    }

    if (hasTTMove) {
        for (int i = 0; i < moves.size; ++i) {
            if (sameMove(moves.moves[i], ttMove)) {
                swap(moves.moves[0], moves.moves[i]);
                break;
            }
        }
    }

    const bool enablePVS = usePVSAtNode(depth, moves.size);
    int bestScore = isMaxNode ? -INF : INF;
    Move bestMove = moves.moves[0];
    bool firstMove = true;

    if (isMaxNode) {
        for (int i = 0; i < moves.size; ++i) {
            const Move& m = moves.moves[i];
            const bool isCapture = (gBoard[m.to] != '.');
            Undo u = makeMove(m);
            int val;

            // ------------------------------------------------------------
            // Late move reduction setup
            // ------------------------------------------------------------
            const int moveReduction = lmrReduction(depth, i, moves.size, m, enablePVS, isCapture);
            const int reducedDepth = max(0, depth - 1 - moveReduction);
            const bool useReducedDepth = (moveReduction > 0);

            if (enablePVS && !firstMove) {
                val = minimax(reducedDepth, alpha, min(beta, alpha + 1), ply + 1, extBudget, &m, nullptr);
                if (!gTimedOut && useReducedDepth && val > alpha + CFG.lmrReSearchMargin) {
                    val = minimax(depth - 1, alpha, beta, ply + 1, extBudget, &m, nullptr);
                } else if (!gTimedOut && !useReducedDepth && val > alpha) {
                    val = minimax(depth - 1, alpha, beta, ply + 1, extBudget, &m, nullptr);
                }
            } else {
                val = minimax(reducedDepth, alpha, beta, ply + 1, extBudget, &m, nullptr);
                if (!gTimedOut && useReducedDepth && val > alpha + CFG.lmrReSearchMargin) {
                    val = minimax(depth - 1, alpha, beta, ply + 1, extBudget, &m, nullptr);
                }
            }

            unmakeMove(u);
            firstMove = false;

            if (gTimedOut) break;
            if (val > bestScore) {
                bestScore = val;
                bestMove = m;
            }
            if (val >= beta) {
                updateMoveLearning(m, depth, triggerMove);
                alpha = max(alpha, val);
                break;
            }
            alpha = max(alpha, bestScore);
            if (alpha >= beta) break;
        }
    } else {
        for (int i = 0; i < moves.size; ++i) {
            const Move& m = moves.moves[i];
            const bool isCapture = (gBoard[m.to] != '.');
            Undo u = makeMove(m);
            int val;

            // ------------------------------------------------------------
            // Late move reduction setup
            // ------------------------------------------------------------
            const int moveReduction = lmrReduction(depth, i, moves.size, m, enablePVS, isCapture);
            const int reducedDepth = max(0, depth - 1 - moveReduction);
            const bool useReducedDepth = (moveReduction > 0);

            if (enablePVS && !firstMove) {
                val = minimax(reducedDepth, max(alpha, beta - 1), beta, ply + 1, extBudget, &m, nullptr);
                if (!gTimedOut && useReducedDepth && val < beta - CFG.lmrReSearchMargin) {
                    val = minimax(depth - 1, alpha, beta, ply + 1, extBudget, &m, nullptr);
                } else if (!gTimedOut && !useReducedDepth && val < beta) {
                    val = minimax(depth - 1, alpha, beta, ply + 1, extBudget, &m, nullptr);
                }
            } else {
                val = minimax(reducedDepth, alpha, beta, ply + 1, extBudget, &m, nullptr);
                if (!gTimedOut && useReducedDepth && val < beta - CFG.lmrReSearchMargin) {
                    val = minimax(depth - 1, alpha, beta, ply + 1, extBudget, &m, nullptr);
                }
            }

            unmakeMove(u);
            firstMove = false;

            if (gTimedOut) break;
            if (val < bestScore) {
                bestScore = val;
                bestMove = m;
            }
            if (val <= alpha) {
                updateMoveLearning(m, depth, triggerMove);
                beta = min(beta, val);
                break;
            }
            beta = min(beta, bestScore);
            if (alpha >= beta) break;
        }
    }

    if (bestMoveOut) *bestMoveOut = bestMove;

    if (!gTimedOut) {
        uint8_t flag = 0;
        if (bestScore <= alphaOrig) flag = 2;
        else if (bestScore >= betaOrig) flag = 1;
        else flag = 0;
        ttStore(gHash, depth, bestScore, flag, &bestMove, ply);
    }

    return bestScore;
}
// ============================================================
// Root Search with Aspiration Windows
// ============================================================

static inline int rootTacticalGuardScoreAfterMove(const Move& m) {
    if (!CFG.enablePrinceThreatAvoidance) return 0;

    int score = 0;
    const char captured = gBoard[m.to];
    if (captured != '.' && gPieceKind[(unsigned char)captured] == PRINCE) {
        score += CFG.rootPrinceCaptureBonus;
    }

    Undo u = makeMove(m);
    const Side them = gSideToMove;
    const Side us = otherSide(them);
    const int ourPrinceSq = princeSquareForSide(us);
    const int theirPrinceSq = princeSquareForSide(them);

    if (theirPrinceSq != -1 && canCaptureSquare(gBoard, us, theirPrinceSq)) {
        score += CFG.rootPrincePressureBonus;
    }
    if (ourPrinceSq != -1 && canCaptureSquare(gBoard, them, ourPrinceSq)) {
        score -= CFG.rootPrinceDangerPenalty;
    }

    unmakeMove(u);
    return score;
}

static inline void applyRootTacticalGuard(MoveList& rootMoves) {
    if (!CFG.enablePrinceThreatAvoidance) return;

    for (int i = 0; i < rootMoves.size; ++i) {
        rootMoves.moves[i].order += rootTacticalGuardScoreAfterMove(rootMoves.moves[i]);
    }
    orderMovesHeuristic(rootMoves);
}

static int searchRootDepth(MoveList& rootMoves, int depth, int alpha, int beta, Move& bestMoveOut, int extBudget) {
    const bool enablePVS = usePVSAtNode(depth, rootMoves.size);
    int bestScore = -INF;
    bestMoveOut = rootMoves.moves[0];
    bool firstMove = true;

    for (int i = 0; i < rootMoves.size; ++i) {
        if (gTimedOut || chrono::steady_clock::now() >= gDeadline) {
            gTimedOut = true;
            break;
        }

        const Move& m = rootMoves.moves[i];
        Undo u = makeMove(m);
        int val;

        if (enablePVS && !firstMove) {
            val = minimax(depth - 1, alpha, min(beta, alpha + 1), 1, extBudget, &m, nullptr);
            if (!gTimedOut && val > alpha) {
                val = minimax(depth - 1, alpha, beta, 1, extBudget, &m, nullptr);
            }
        } else {
            val = minimax(depth - 1, alpha, beta, 1, extBudget, &m, nullptr);
        }

        unmakeMove(u);
        firstMove = false;

        if (val > bestScore) {
            bestScore = val;
            bestMoveOut = m;
        }
        alpha = max(alpha, bestScore);
        if (alpha >= beta) break;
        if (gTimedOut) break;
    }

    return bestScore;
}
// ============================================================
// Time Budget Policy
// ============================================================

static int chooseDepth(double myTime, double oppTime) {
    int depth = 0;
    if (myTime < CFG.depth1TimeCapSec) depth = 1;
    else if (myTime < CFG.depth2TimeCapSec) depth = 2;
    else if (myTime < CFG.depth3TimeCapSec) depth = 3;
    else depth = CFG.maxDepth;

    // Relative-time bonus, but only when we are not already time-poor.
    const double ratio = myTime / max(10.0, oppTime);
    if (myTime >= 1.0) {
        if (ratio >= 1.5) ++depth;
        if (ratio >= 2.0) ++depth;
    }

    return min(depth, CFG.maxTotalSearchDepth + 2);
}

// ============================================================
// Playdata Persistence for Repetition and Time Memory
// ============================================================

static void clearGameMemory() {
    gHistoryCounts.clear();
    gSearchHashStackLen = 0;
    gRecentMoveTimesLen = 0;
    gHalfmoveClock = 0;
    gHasPlaydataBoard = false;
    clearMoveLearningTables();
}

static bool inferAndLoadCurrentHalfmove(const Board& currentBoard) {
    if (!gHasPlaydataBoard) {
        gHalfmoveClock = 0;
        return false;
    }
    int fromSq = -1;
    int toSq = -1;
    char mover = '.';
    char captured = '.';

    for (int s = 0; s < BOARD_SQ; ++s) {
        const char before = gPlaydataBoard[s];
        const char after = currentBoard[s];
        if (before == after) continue;
        if (before != '.' && after == '.') {
            fromSq = s;
            mover = before;
        } else if (before == '.' && after != '.') {
            toSq = s;
            mover = after;
        } else if (before != '.' && after != '.') {
            toSq = s;
            mover = after;
            captured = before;
        }
    }

    if (fromSq >= 0 && toSq >= 0) {
        const bool reset = (captured != '.') || (lowerPiece(mover) == 'b');
        gHalfmoveClock = reset ? 0 : (gHalfmoveClock + 1);
    }
    return true;
}

static void loadPlaydata() {
    clearGameMemory();

    ifstream fin("playdata.txt");
    if (!fin.is_open()) return;

    string tag;
    while (fin >> tag) {
        if (tag == "HALFMOVE") {
            fin >> gHalfmoveClock;
        } else if (tag == "RECENT") {
            int len = 0;
            fin >> len;
            len = max(0, min(len, MAX_RECENT_PERF_SAMPLES));
            gRecentMoveTimesLen = len;
            for (int j = 0; j < len; ++j) fin >> gRecentMoveTimes[j];
        } else if (tag == "BOARD") {
            gHasPlaydataBoard = true;
            for (int r = 0; r < N; ++r) {
                string row;
                fin >> row;
                for (int c = 0; c < N; ++c) {
                    gPlaydataBoard[sq(r, c)] = row[c];
                }
            }
        } else if (tag == "HISTORY") {
            int entries = 0;
            fin >> entries;
            for (int j = 0; j < entries; ++j) {
                uint64_t key = 0;
                int count = 0;
                fin >> key >> count;
                if (count > 0) gHistoryCounts[key] = count;
            }
        }
    }
}

static void savePlaydata(const Board& boardToSave) {
    ofstream fout("playdata.txt", ios::trunc);
    if (!fout.is_open()) return;

    fout << "HALFMOVE " << gHalfmoveClock << '\n';
    fout << "RECENT " << gRecentMoveTimesLen;
    for (int j = 0; j < gRecentMoveTimesLen; ++j) {
        fout << ' ' << fixed << setprecision(6) << gRecentMoveTimes[j];
    }
    fout << '\n';

    fout << "BOARD" << '\n';
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            fout << boardToSave[sq(r, c)];
        }
        fout << '\n';
    }

    fout << "HISTORY " << gHistoryCounts.size() << '\n';
    for (const auto& kv : gHistoryCounts) {
        fout << kv.first << ' ' << kv.second << '\n';
    }
}


static string moveToString(const Move& m) {
    string out;
    out.push_back(COLS[colOf(m.from)]);
    out += to_string(N - rowOf(m.from));
    out.push_back(' ');
    out.push_back(COLS[colOf(m.to)]);
    out += to_string(N - rowOf(m.to));
    return out;
}


static void loadPositionFromInput(istream& fin, Side& side, double& myTime, double& oppTime) {
    string sideStr;
    fin >> sideStr;
    fin >> myTime >> oppTime;
    side = (sideStr == "WHITE" ? WHITE : BLACK);

    gMyTimeRemaining = myTime;
    gOppTimeRemaining = oppTime;

    for (int r = 0; r < N; ++r) {
        string row;
        fin >> row;
        for (int c = 0; c < N; ++c) {
            gBoard[sq(r, c)] = row[c];
        }
    }

    gWhiteEval = 0;
    gWhitePrinceSq = -1;
    gBlackPrinceSq = -1;
    gHash = 0;
    gSideToMove = side;

    for (int s = 0; s < BOARD_SQ; ++s) {
        const char ch = gBoard[s];
        if (ch == '.') continue;
        gWhiteEval += pieceEvalContribution(ch, s);
        const int pi = gPieceIndex[(unsigned char)ch];
        gHash ^= gZobrist[pi][s];
        if (ch == 'P') gWhitePrinceSq = s;
        else if (ch == 'p') gBlackPrinceSq = s;
    }

    if (gSideToMove == BLACK) {
        gHash ^= gSideKey;
    }

    rebuildPieceListsFromBoard();

    // Record the current position once so repetition counting includes this turn's input state.
    recordHistoryHash(gHash);

    // The search stack is reserved for positions reached along the current search line.
    gSearchHashStackLen = 0;
}

// ============================================================
// Main Driver
// ============================================================

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    initPieceTables();
    initPstTables();
    initZobrist();
    initMoveTemplates();
    loadPlaydata();

    ifstream fin("input.txt");
    ofstream fout("output.txt");
    if (!fin.is_open() || !fout.is_open()) return 0;

    double myTime = 0.0, oppTime = 0.0;
    loadPositionFromInput(fin, gRootSide, myTime, oppTime);


    // Draw-state tracking: reconstruct the current halfmove clock from the previous saved board.
    inferAndLoadCurrentHalfmove(gBoard);

    const double budget = max(0.0, myTime - CFG.safetyMarginSec);
    gDeadline = chrono::steady_clock::now() + chrono::duration_cast<chrono::steady_clock::duration>(chrono::duration<double>(budget));
    gTimedOut = false;
    gNodeCount = 0;
    gQSearchNodeCount = 0;

    const auto searchStart = chrono::steady_clock::now();

    MoveList rootMoves;
    generateMoves(gBoard, gRootSide, rootMoves, 0, nullptr);
    applyRootTacticalGuard(rootMoves);
    if (rootMoves.size == 0) {
        // The engine guarantees at least one legal move, but keep a fallback.
        fout << "a1 a1\n";
        return 0;
    }

    const int baseDepthLimit = chooseDepth(myTime, oppTime);
    const int staticRootBonus = CFG.enableDepthExtensions ? (rootPhaseExtensionBonus() + rootAdvantageExtensionBonus()) : 0;
    const int rootBonusCap = max(0, CFG.maxRootDepthBonus);
    int dynamicRootBonus = 0;
    int depthLimit = min(baseDepthLimit + staticRootBonus, baseDepthLimit + rootBonusCap);
    depthLimit = min(depthLimit, max(baseDepthLimit, CFG.maxTotalSearchDepth));
    Move bestMove = rootMoves.moves[0];
    int bestScore = -INF;
    int prevDepthScore = 0;
    bool havePrevDepthScore = false;

    for (int depth = 1; depth <= depthLimit; ++depth) {
        if (gTimedOut || chrono::steady_clock::now() >= gDeadline) break;
        ++gTTGeneration;

        // Keep the previous iteration's best move at the front for better move ordering.
        for (int i = 0; i < rootMoves.size; ++i) {
            if (sameMove(rootMoves.moves[i], bestMove)) {
                swap(rootMoves.moves[0], rootMoves.moves[i]);
                break;
            }
        }

        Move depthBest = bestMove;
        int depthBestScore = -INF;

        // Aspiration window: a narrow root window often saves nodes when the score is stable.
        int alpha = -INF;
        int beta = INF;
        if (havePrevDepthScore && depth > 1) {
            const int window = 8000 + depth * 1000;
            alpha = max(-INF, prevDepthScore - window);
            beta = min(INF, prevDepthScore + window);
        }

        depthBestScore = searchRootDepth(rootMoves, depth, alpha, beta, depthBest, CFG.maxTotalExtensionPerPath);

        // If the aspiration window was too narrow, fall back to a full-width search.
        if (!gTimedOut && havePrevDepthScore && depth > 1 && (depthBestScore <= alpha || depthBestScore >= beta)) {
            depthBestScore = searchRootDepth(rootMoves, depth, -INF, INF, depthBest, CFG.maxTotalExtensionPerPath);
        }

        if (!gTimedOut) {
            const bool aspirationFailed = havePrevDepthScore && depth > 1 && (depthBestScore <= alpha || depthBestScore >= beta);
            const bool scoreJump = havePrevDepthScore && abs(depthBestScore - prevDepthScore) >= CFG.rootInstabilityScoreJump;
            const bool moveFlip = havePrevDepthScore && !sameMove(depthBest, bestMove);

            int rootVolatilityStep = 0;
            if (aspirationFailed) rootVolatilityStep += CFG.rootInstabilityAspirationBonus;
            if (moveFlip) rootVolatilityStep += CFG.rootInstabilityMoveFlipBonus;
            if (scoreJump) ++rootVolatilityStep;

            if (rootVolatilityStep > 0) {
                dynamicRootBonus = min(rootBonusCap, dynamicRootBonus + rootVolatilityStep);
                depthLimit = min(baseDepthLimit + staticRootBonus + dynamicRootBonus, baseDepthLimit + rootBonusCap);
            } else if (dynamicRootBonus > 0 && depth > 2) {
                --dynamicRootBonus;
                depthLimit = min(baseDepthLimit + staticRootBonus + dynamicRootBonus, baseDepthLimit + rootBonusCap);
            }

            bestMove = depthBest;
            bestScore = depthBestScore;
            prevDepthScore = depthBestScore;
            havePrevDepthScore = true;
        }
    }

    (void)bestScore;
    const auto searchEnd = chrono::steady_clock::now();
    const double elapsed = chrono::duration<double>(searchEnd - searchStart).count();
    pushRecentMoveTime(elapsed);

    // Final legality guard: search should already choose from rootMoves, but this
    // ensures we never output a move that is not present in the current legal list.
    if (!moveExistsInList(rootMoves, bestMove) && rootMoves.size > 0) {
        bestMove = rootMoves.moves[0];
    }

    // Persist time-memory data for the next turn. The current input hash was already
    // recorded when the position was loaded, so we do not record the post-move hash here.
    makeMove(bestMove);
    savePlaydata(gBoard);

    fout << moveToString(bestMove) << '\n';
    return 0;
}
