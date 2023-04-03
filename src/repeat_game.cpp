#include <algorithm>
#include <iostream>
#include <vector>
#include <array>
#include <sstream>
#include "BoardGame.h"
#include "BoardGameAI.h"

template<uint32_t kMaxVal, bool kTrackP1, int16_t kEvalShift = 0,
         bool kDebug = false>
class RepeatGameExt : public BoardGame<
    RepeatGameExt<kMaxVal, kTrackP1, kEvalShift, kDebug>,
    false, kMaxVal + 1> {
private:
    using Parent = BoardGame<
        RepeatGameExt<kMaxVal, kTrackP1, kEvalShift, kDebug>,
        false, kMaxVal + 1>;

    std::vector<char> visited;
    uint64_t visitedHash;
    uint32_t currNumber;
    uint32_t maxPtNumber;
    bool isPtTurn;
    bool favorWinsInEval;
    std::vector<uint32_t> currNumberList;
    std::vector<uint32_t> maxPtNumberList;
    bool won;
    static constexpr uint64_t kRandBias = 12963258741717719282ULL;
    static constexpr uint64_t kRandMult = 11142652954828712569ULL;

    bool calculateHaswon() {
        for (size_t i = 1; i <= kNumPossibleMoves; i++) {
            if (!isBlocked(move(i, currNumber))) {
                return false;
            }
        }
        return true;
    }

    void reset() {
        std::fill(visited.begin(), visited.end(), 0);
        visited[1] = 1;
        visitedHash = kRandBias + kRandMult;
        currNumber = 1;
        maxPtNumber = kTrackP1 ? currNumber : 0;
        isPtTurn = kTrackP1;
        favorWinsInEval = false;
        currNumberList.clear();
        maxPtNumberList.clear();
        won = calculateHaswon();
    }

public:
    static constexpr size_t kNumPossibleMoves = 3;

    static uint32_t move(Move move, uint32_t number) {
        switch (move) {
            case 1: return number / 3;
            case 2: return 2 * number + 1;
            case 3: default: return 2 * number + 5;
        }
    }

    static uint32_t largestNext(uint32_t number) {
        uint32_t largestSeen = 0;
        for (Move i = 1; i <= kNumPossibleMoves; i++) {
            uint32_t next = move(i, number);
            if (next > largestSeen) {
                largestSeen = next;
            }
        }
        return largestSeen;
    }

    bool isBlocked(uint32_t nextValue) const {
        if (nextValue > kMaxVal) return true;
        bool result = false;
        for (Move i = 1; i <= kNumPossibleMoves; i++) {
            uint32_t nextNextValue = move(i, nextValue);
            result = result || (nextNextValue <= kMaxVal &&
                (visited[nextNextValue] || nextNextValue == nextValue));
        }
        return result;
    }

    std::string explainIllegalMove(Move m) const {
        uint32_t nextValue = move(m, currNumber);
        std::string nextValueStr = std::to_string(nextValue);
        if (nextValue > kMaxVal) return nextValueStr + " is out of bounds";
        for (Move i = 1; i <= kNumPossibleMoves; i++) {
            uint32_t nextNextValue = move(i, nextValue);
            if (nextNextValue <= kMaxVal &&
                (visited[nextNextValue] || nextNextValue == nextValue)) {
                return nextValueStr + " loses to response " + \
                    std::to_string(nextNextValue);
            }
        }
        return "Legal move";
    }

    RepeatGameExt(): visited(kMaxVal + 1, 0) {
        reset();
    }

    ~RepeatGameExt() {};

    uint32_t getCurrNumber() const {
        return currNumber;
    }

    uint32_t getMaxPtNumber() const {
        return maxPtNumber;
    }

    bool getIsPtTurn() const {
        return isPtTurn;
    }

    uint64_t hash_impl() const {
        return std::hash<uint64_t>{}(visitedHash) ^
            std::hash<uint32_t>{}(currNumber) ^
            std::hash<uint32_t>{}(maxPtNumber);
    }

    void makeMove_impl(Move n) {
        uint32_t next = move(n, currNumber);
        visited[next] = 1;
        visitedHash ^= kRandBias + kRandMult * next;
        currNumberList.push_back(currNumber);
        maxPtNumberList.push_back(maxPtNumber);
        if (!isPtTurn && next > maxPtNumber) {
            maxPtNumber = next;
            isPtTurn = true;
        } else {
            isPtTurn = !isPtTurn;
        }
        uint32_t currNumberOrig = currNumber;
        currNumber = next;
        won = calculateHaswon();
        if (kDebug) {
            std::cout << std::endl << "Made move " << "from "
                << currNumberOrig << " to " << currNumber << std::endl;
            std::cout << toString() << std::endl;
        }
    }

    void undoMove_impl() {
        visited[currNumber] = false;
        visitedHash ^= kRandBias + kRandMult * currNumber;
        currNumber = currNumberList.back();
        currNumberList.pop_back();
        maxPtNumber = maxPtNumberList.back();
        maxPtNumberList.pop_back();
        isPtTurn = !isPtTurn;
        won = false;
    }

    bool hasWon_impl() const {
        return won;
    }

    bool isGameOver_impl() const {
        return won;
    }

    void setFavorWinsInEval(bool value) {
        favorWinsInEval = value;
    }

    int16_t evaluateNum_impl(uint32_t number) const {
        // Pt wants to maximize the number, and PtOpp wants to minimize
        uint32_t eval = kMaxVal + 1 - number;
        int16_t sign = 1;
        if (won || isPtTurn) {
            sign = -1;
        }
        if (won && favorWinsInEval) {
            uint32_t bias = 1;
            bias = (bias << kEvalShift) - 1;
            uint32_t bonus = (
                (kMaxVal + 1 + bias) >> kEvalShift) << kEvalShift;
            eval += bonus;
        }
        int16_t result = sign * ((eval >> kEvalShift) + 1);
        if (kDebug) {
            std::cout << "Evaluation " << sign * static_cast<int32_t>(eval)
                << " (" << result << ")" << std::endl;
        }
        return result;
    }

    int16_t evaluate_impl() const {
        return evaluateNum_impl(maxPtNumber);
        // return evaluateNum_impl(currNumber);
    }

    int16_t maxPossibleEvaluation_impl() const {
        constexpr uint32_t maxEval = kMaxVal + 1;
        constexpr uint32_t bias = (
            static_cast<uint32_t>(1) << kEvalShift) - 1;
        constexpr uint32_t maxEvalBias = 2 * maxEval + bias;
        constexpr uint32_t maxEval16 = (maxEvalBias >> kEvalShift) + 1;
        static_assert(maxEval16 < 32000);
        return (maxEval >> kEvalShift) + 1;
    }

    static void interpretEval(
            int16_t eval, bool* advantage, uint32_t* ptNumberUpper) {
        // Only makes sense if maxPtNumber is used for evaluation.
        *advantage = true;
        if (eval < 0) {
            *advantage = false;
            eval = -eval;
        }
        eval -= 1;
        *ptNumberUpper = eval;
        *ptNumberUpper = *ptNumberUpper << kEvalShift;
        uint32_t bias = 1;
        bias = (bias << kEvalShift) - 1;
        uint32_t bonus = (
            (kMaxVal + 1 + bias) >> kEvalShift) << kEvalShift;
        if (*ptNumberUpper > bonus) *ptNumberUpper -= bonus;
        *ptNumberUpper = kMaxVal + 1 - *ptNumberUpper;
    }

    constexpr static int counterMoveStates_impl() {
        return kMaxVal + 1;
    }

    class MoveGenerator {
    private:
        const RepeatGameExt* game = nullptr;
        size_t index = 0;
    public:
        MoveGenerator() {}
        constexpr MoveGenerator(
            const RepeatGameExt* game) : game(game), index(0) {}

        Move nextMove() {
            size_t currentIndex;
            while ((currentIndex = index++) < kNumPossibleMoves) {
                Move prefMove = (currentIndex + 1);
                uint32_t nextValue = move(prefMove, game->getCurrNumber());
                if (!game->isBlocked(nextValue)) {
                    return prefMove;
                }
            }
            return Parent::noMoveValue();
        }
    };

    MoveGenerator moveGenerator_impl() const {
        return MoveGenerator(this);
    }

    std::string toString() {
        std::stringstream strm;
        strm << "[Game state]" << std::endl;
        strm << "Visited:";
        for (uint32_t i = 0; i < kMaxVal + 1; i++) {
            if (visited[i]) {
                strm << " " << i;
            }
        }
        strm << std::endl;
        strm << "Current value: " << currNumber << std::endl;
        const char* pt = "P2";
        const char* ptOpp = "P1";
        if (kTrackP1) {
            pt = "P1";
            ptOpp = "P2";
        }
        strm << "Max " << pt << " number: " << maxPtNumber << std::endl;
        strm << "Turn: " << (isPtTurn ? pt : ptOpp);
        if (hasWon_impl()) {
            strm  << std::endl << "GAME OVER";
        }
        return strm.str();
    }
};

static constexpr uint32_t kGameMaxVal = 10000000;
static constexpr uint32_t kGameMaxDepth = 20;
static constexpr bool kGameTrackP1 = true;
static constexpr size_t kSolverTtSizeMb = 2048;
static constexpr bool kInteract = true;
static constexpr bool kQuietInteract = true;
using RepeatGame = RepeatGameExt<kGameMaxVal, kGameTrackP1, 10, false>;

int main() {
    std::shared_ptr<RepeatGame> game =
        std::shared_ptr<RepeatGame>(new RepeatGame());
    auto ai = AIBuilder<RepeatGame>{}
        .iterativeDeepening().useTTable<kSolverTtSizeMb>().create();
    SearchResult result = ai.search(game, kGameMaxDepth);
    std::cout << "Value: " << result.value << ", Best Move: "
        << result.bestMove << ", Depth: " << result.depth << std::endl;
    bool advantage;
    uint32_t ptNumberUpper;
    const char* pt = "P2";
    if (kGameTrackP1) pt = "P1";
    RepeatGame::interpretEval(result.value, &advantage, &ptNumberUpper);
    std::cout << std::boolalpha << "P1 advantage: " << advantage
        << ", " << pt << " max value bound: " << ptNumberUpper << std::endl;
    bool enoughDepth = static_cast<uint32_t>(result.depth) < kGameMaxDepth;
    std::cout << std::boolalpha << "Used enough depth: "
        << enoughDepth << std::endl;
    std::cout << std::boolalpha << pt << " == loser: "
        << (kGameTrackP1 != advantage) << std::endl;
    uint32_t ptNextUpper = RepeatGame::largestNext(ptNumberUpper);
    std::cout << std::boolalpha << "Compare " << ptNextUpper << " <= "
        << kGameMaxVal << ": " << (ptNextUpper <= kGameMaxVal)
        << std::endl;
    if (!kInteract) {
        return 0;
    }

    std::cout << std::endl << "Now starting interactive session" << std::endl;
    game->setFavorWinsInEval(true);
    while (true) {
        bool aiVsItself;
        std::cout << "Let AI play itself? (Enter 1 or 0): " << std::flush;
        std::cin >> aiVsItself;

        bool playAsP1 = true;
        if (!aiVsItself) {
            std::cout << "Play as P1? (Enter 1 or 0): " << std::flush;
            std::cin >> playAsP1;
        }

        if (kQuietInteract) {
            std::cout << game->getCurrNumber() << std::flush;
        } else {
            std::cout << game->toString() << std::endl;
        }
        while (!game->isGameOver()) {
            bool isP1Turn = !game->currentPlayer();
            if (isP1Turn == playAsP1) {
                std::vector<Move> moves;
                game->moves(moves);
                if (!kQuietInteract) {
                    result = ai.search(game, kGameMaxDepth);
                    RepeatGame::interpretEval(
                        result.value, &advantage, &ptNumberUpper);
                    if (enoughDepth) {
                        std::cout << ((advantage == isP1Turn) ? "P1" : "P2")
                            << " win, " << pt << " max value bound: "
                            << ptNumberUpper << std::endl;
                    } else {
                        std::cout << "Unconfirmed " << pt
                            << " max value bound: " << ptNumberUpper
                            << std::endl;
                    }
                    std::cout << "AI recommends move " << result.bestMove
                        << std::endl;
                }
                Move playerMove;
                if (aiVsItself) {
                    result = ai.search(game, kGameMaxDepth);
                    playerMove = result.bestMove;
                } else {
                    if (kQuietInteract) std::cout << std::endl;
                    std::cout << "Your possible moves:";
                    for (size_t i = 0; i < moves.size(); i++) {
                        std::cout << " " << moves[i];
                    }
                    std::cout << std::endl;
                    while (true) {
                        std::cout << "You play: " << std::flush;
                        std::cin >> playerMove;
                        if (std::find(moves.begin(), moves.end(),
                            playerMove) != moves.end()) {
                            break;
                        } else {
                            std::cout << game->explainIllegalMove(playerMove)
                                << std::endl;
                        }
                    }
                }
                game->makeMove(playerMove);
                if (!kQuietInteract) {
                    std::cout << game->toString() << std::endl;
                } else {
                    std::cout << " " << game->getCurrNumber() << std::flush;
                }
            } else {
                SearchResult result = ai.search(game, kGameMaxDepth);
                if (!kQuietInteract) {
                    std::cout << "AI plays: " << result.bestMove << std::endl;
                }
                game->makeMove(result.bestMove);
                if (!kQuietInteract) {
                    std::cout << game->toString() << std::endl;
                } else {
                    std::cout << " " << game->getCurrNumber() << std::flush;
                }
            }
        }
        if (kQuietInteract) std::cout << std::endl;
        bool isP1Turn = !game->currentPlayer();
        if (!aiVsItself) {
            if (isP1Turn == playAsP1) {
                std::cout << "YOU LOSE" << std::endl;
            } else {
                std::cout << "YOU WIN" << std::endl;
            }
            std::cout << "Explanation:" << std::endl;
            for (Move i = 0; i < RepeatGame::kNumPossibleMoves; i++) {
                std::cout << game->explainIllegalMove(i) << std::endl;
            }
        } else {
            const char* winner = isP1Turn ? "P2" : "P1";
            std::cout << "Winner: " << winner << std::endl;
        }
        std::cout << "Max observed " << pt << " number: "
            << game->getMaxPtNumber() << std::endl;
        std::cout << "Resetting game..." << std::endl << std::endl;
        game->clear();
    }
}