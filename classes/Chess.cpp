#include "Chess.h"
#include <limits>
#include <cmath>
#include <cctype>
#include "Bitboard.h"   // for BitMove + BitboardElement
#include "../imgui/imgui.h"
#include <iostream>
// ===========================================================
// Knight Move Lookup Table (64 entries)
// ===========================================================
static const uint64_t KnightMoves[64] = {
    0x0000000000020400ULL, 0x0000000000050800ULL, 0x00000000000A1100ULL, 0x0000000000142200ULL,
    0x0000000000284400ULL, 0x0000000000508800ULL, 0x0000000000A01000ULL, 0x0000000000402000ULL,
    0x0000000002040004ULL, 0x0000000005080008ULL, 0x000000000A110011ULL, 0x0000000014220022ULL,
    0x0000000028440044ULL, 0x0000000050880088ULL, 0x00000000A0100010ULL, 0x0000000040200020ULL,
    0x0000000204000402ULL, 0x0000000508000805ULL, 0x0000000A1100110AULL, 0x0000001422002214ULL,
    0x0000002844004428ULL, 0x0000005088008850ULL, 0x000000A0100010A0ULL, 0x0000004020002040ULL,
    0x0000020400040200ULL, 0x0000050800080500ULL, 0x00000A1100110A00ULL, 0x0000142200221400ULL,
    0x0000284400442800ULL, 0x0000508800885000ULL, 0x0000A0100010A000ULL, 0x0000402000204000ULL,
    0x0002040004020000ULL, 0x0005080008050000ULL, 0x000A1100110A0000ULL, 0x0014220022140000ULL,
    0x0028440044280000ULL, 0x0050880088500000ULL, 0x00A0100010A00000ULL, 0x0040200020400000ULL,
    0x0204000402000000ULL, 0x0508000805000000ULL, 0x0A1100110A000000ULL, 0x1422002214000000ULL,
    0x2844004428000000ULL, 0x5088008850000000ULL, 0xA0100010A0000000ULL, 0x4020002040000000ULL,
    0x0400040200000000ULL, 0x0800080500000000ULL, 0x1100110A00000000ULL, 0x2200221400000000ULL,
    0x4400442800000000ULL, 0x8800885000000000ULL, 0x100010A000000000ULL, 0x2000204000000000ULL,
    0x0004020000000000ULL, 0x0008050000000000ULL, 0x00110A0000000000ULL, 0x0022140000000000ULL,
    0x0044280000000000ULL, 0x0088500000000000ULL, 0x0010A00000000000ULL, 0x0020400000000000ULL
};

// ===========================================================
// Constructor / Destructor
// ===========================================================

Chess::Chess()
{
    _grid = new Grid(8, 8);
}

Chess::~Chess()
{
    delete _grid;
}

// ===========================================================
// Helpers
// ===========================================================

char Chess::pieceNotation(int x, int y) const
{
    const char *wpieces = { "0PNBRQK" };
    const char *bpieces = { "0pnbrqk" };
    Bit *bit = _grid->getSquare(x, y)->bit();
    char notation = '0';
    if (bit) {
        notation = bit->gameTag() < 128 ? wpieces[bit->gameTag()] : bpieces[bit->gameTag() - 128];
    }
    return notation;
}

// Convert Bit.gameTag → PieceInfo
Chess::PieceInfo Chess::getPieceInfo(const Bit& bit) const
{
    PieceInfo info{};
    int tag = bit.gameTag();

    if (tag >= 128) {
        info.isWhite = false;
        tag -= 128;
    } else {
        info.isWhite = true;
    }

    switch (tag) {
        case 1: info.type = PT_Pawn; break;
        case 2: info.type = PT_Knight; break;
        case 3: info.type = PT_Bishop; break;
        case 4: info.type = PT_Rook; break;
        case 5: info.type = PT_Queen; break;
        case 6: info.type = PT_King; break;
        default: info.type = PT_None; break;
    }

    return info;
}

// Scan grid to find coordinates of a BitHolder
bool Chess::getCoordsForHolder(BitHolder& holder, int& xOut, int& yOut)
{
    bool found = false;
    BitHolder* target = &holder;

    _grid->forEachSquare([&](ChessSquare* sq, int x, int y) {
        if ((BitHolder*)sq == target) {
            xOut = x;
            yOut = y;
            found = true;
        }
    });

    return found;
}

// Convert x,y to 0..63 (a1=0, h8=63)
int Chess::boardIndex(int x, int y) const
{
    int rank = 7 - y;
    int file = x;
    return rank * 8 + file;
}


// ===========================================================
// FEN Loader
// ===========================================================

Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece)
{
    const char* pieces[] = {
        "pawn.png", "knight.png", "bishop.png",
        "rook.png", "queen.png", "king.png"
    };

    Bit* bit = new Bit();
    std::string spritePath = std::string("") +
        (playerNumber == 0 ? "w_" : "b_") +
        pieces[piece - 1];

    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);

    // Set gameTag so pieceNotation works
    int tag = static_cast<int>(piece);
    if (playerNumber == 1) tag += 128;
    bit->setGameTag(tag);

    return bit;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    startGame();

    // Standard starting position
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
}

void Chess::FENtoBoard(const std::string& fen)
{
    std::string boardField;
    size_t sp = fen.find(' ');
    boardField = (sp == std::string::npos) ? fen : fen.substr(0, sp);

    int slashCount = 0;
    for (char c : boardField) if (c == '/') ++slashCount;
    if (slashCount != 7) return;

    _grid->forEachSquare([](ChessSquare* sq, int, int) { sq->destroyBit(); });

    int x = 0, y = 0;

    auto placePiece = [&](char pch, int file, int rank) {
        int player = std::isupper((unsigned char) pch) ? 0 : 1;
        char l = (char) std::tolower((unsigned char)pch);

        ChessPiece piece = NoPiece;
        switch (l) {
            case 'p': piece = Pawn;   break;
            case 'n': piece = Knight; break;
            case 'b': piece = Bishop; break;
            case 'r': piece = Rook;   break;
            case 'q': piece = Queen;  break;
            case 'k': piece = King;   break;
            default:  return;
        }

        ChessSquare* sq = _grid->getSquare(file, rank);
        if (!sq) return;

        Bit* bit = PieceForPlayer(player, piece);
        sq->setBit(bit);

        // IMPORTANT: tie the bit to this square like dropBitAtPoint does
        bit->setParent(sq);
        bit->moveTo(sq->getPosition());
    };

    for (char c : boardField)
    {
        if (c == '/') {
            if (x != 8) return;
            x = 0;
            y++;
            continue;
        }

        if (std::isdigit((unsigned char)c)) {
            x += (c - '0');
            continue;
        }

        placePiece(c, x, y);
        x++;
    }
}


// ===========================================================
// Movement Rules (Selection + Trying Moves)
// ===========================================================

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{

    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    return (bit.gameTag() & 128) == currentPlayer;
}


bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{

    if (!canBitMoveFrom(bit, src))
        return false;

    int sx, sy, dx, dy;
    if (!getCoordsForHolder(src, sx, sy)) return false;
    if (!getCoordsForHolder(dst, dx, dy)) return false;

    int dxSigned = dx - sx;
    int dySigned = dy - sy;
    int ax = std::abs(dxSigned);
    int ay = std::abs(dySigned);

    ChessSquare* dstSq = _grid->getSquare(dx, dy);
    Bit* dstBit = dstSq ? dstSq->bit() : nullptr;

    PieceInfo info = getPieceInfo(bit);
    bool isWhite = info.isWhite;

    // Cannot capture own color
    if (dstBit) {
        PieceInfo d = getPieceInfo(*dstBit);
        if (d.isWhite == isWhite)
            return false;
    }

    switch (info.type)
    {
        case PT_Pawn:
        {
            int dir = isWhite ? -1 : 1;
            int startY = isWhite ? 6 : 1;

            // forward move
            if (dxSigned == 0)
            {
                if (dySigned == dir && !dstBit)
                    return true;

                if (sy == startY && dySigned == 2 * dir) {
                    ChessSquare* mid = _grid->getSquare(sx, sy + dir);
                    if (mid && !mid->bit() && !dstBit)
                        return true;
                }

                return false;
            }

            // diagonal capture
            if (ax == 1 && dySigned == dir && dstBit)
                return true;

            return false;
        }

        case PT_Knight:
            return (ax == 1 && ay == 2) || (ax == 2 && ay == 1);

        case PT_King:
            return (ax <= 1 && ay <= 1 && (ax + ay) != 0);

        default:
            return false;
    }
}


// ===========================================================
// Bitboard Occupancy Helpers
// ===========================================================

uint64_t Chess::getOccupancy() const
{
    uint64_t occ = 0ULL;
    _grid->forEachSquare([&](ChessSquare* sq, int x, int y) {
        if (sq->bit()) occ |= (1ULL << boardIndex(x, y));
    });
    return occ;
}

uint64_t Chess::getColorOccupancy(int player) const
{
    uint64_t occ = 0ULL;
    _grid->forEachSquare([&](ChessSquare* sq, int x, int y) {
        Bit* b = sq->bit();
        if (b && b->getOwner()->playerNumber() == player)
            occ |= (1ULL << boardIndex(x, y));
    });
    return occ;
}


// ===========================================================
// Move Generator (Pawn, Knight, King)
// Returns ~20 moves from starting position
// ===========================================================

void Chess::generateMovesForCurrentPlayer(BitMove* moves, int& count)
{
    count = 0;
    int player = getCurrentPlayer()->playerNumber();

    uint64_t own = getColorOccupancy(player);
    (void)own;  // May be used in move generation logic

    _grid->forEachSquare([&](ChessSquare* sq, int x, int y)
    {
        Bit* bit = sq->bit();
        if (!bit) return;
        if (bit->getOwner()->playerNumber() != player) return;

        PieceInfo info = getPieceInfo(*bit);
        int from = boardIndex(x, y);

        // -------------------------------
        // PAWN MOVES
        // -------------------------------
        if (info.type == PT_Pawn)
        {
            int dir       = info.isWhite ? -1 : 1;   // white goes "up", black "down"
            int startRank = info.isWhite ? 6  : 1;   // rank index in your grid
            int ny        = y + dir;

            // ----- forward 1 -----
            if (ny >= 0 && ny < 8)
            {
                ChessSquare* one = _grid->getSquare(x, ny);
                if (one && !one->bit())
                {
                    moves[count++] = BitMove(from, boardIndex(x, ny), Pawn);

                    // ----- forward 2 from starting rank -----
                    if (y == startRank) {
                        int ny2 = y + 2 * dir;
                        if (ny2 >= 0 && ny2 < 8) {
                            ChessSquare* mid = _grid->getSquare(x, y + dir);
                            ChessSquare* two = _grid->getSquare(x, ny2);
                            if (mid && two && !mid->bit() && !two->bit()) {
                                moves[count++] = BitMove(from, boardIndex(x, ny2), Pawn);
                            }
                        }
                    }
                }
            }

            // ----- capture diagonals -----
            int left  = x - 1;
            int right = x + 1;

            if (left >= 0 && ny >= 0 && ny < 8)
            {
                ChessSquare* c = _grid->getSquare(left, ny);
                if (c && c->bit() && c->bit()->getOwner()->playerNumber() != player)
                    moves[count++] = BitMove(from, boardIndex(left, ny), Pawn);
            }

            if (right < 8 && ny >= 0 && ny < 8)
            {
                ChessSquare* c = _grid->getSquare(right, ny);
                if (c && c->bit() && c->bit()->getOwner()->playerNumber() != player)
                    moves[count++] = BitMove(from, boardIndex(right, ny), Pawn);
            }
        }


        // -------------------------------
        // KNIGHT MOVES
        // -------------------------------
        if (info.type == PT_Knight)
        {
            uint64_t attacks = KnightMoves[from] & ~own;

            BitboardElement bb(attacks);
            bb.forEachBit([&](int to) {
                moves[count++] = BitMove(from, to, Knight);
            });
        }

        // -------------------------------
        // KING MOVES
        // -------------------------------
        if (info.type == PT_King)
        {
            uint64_t k = (1ULL << from);

            uint64_t attacks =
                ((k << 1) & 0xfefefefefefefefeULL) |
                ((k >> 1) & 0x7f7f7f7f7f7f7f7fULL) |
                (k << 8) |
                (k >> 8) |
                ((k << 9) & 0xfefefefefefefefeULL) |
                ((k << 7) & 0x7f7f7f7f7f7f7f7fULL) |
                ((k >> 9) & 0x7f7f7f7f7f7f7f7fULL) |
                ((k >> 7) & 0xfefefefefefefefeULL);

            attacks &= ~own;

            BitboardElement bb(attacks);
            bb.forEachBit([&](int to) {
                moves[count++] = BitMove(from, to, King);
            });
        }

    });

    // Should be exactly 20 moves in starting position
}


// ===========================================================
// Game end check
// ===========================================================

Player* Chess::ownerAt(int x, int y) const
{
    if (! (x >= 0 && x < 8 && y >= 0 && y < 8)) return nullptr;
    ChessSquare* sq = _grid->getSquare(x, y);
    if (!sq || !sq->bit()) return nullptr;
    return sq->bit()->getOwner();
}

Player* Chess::checkForWinner()
{
    return nullptr;
}

bool Chess::checkForDraw()
{
    return false;
}


// ===========================================================
// State String Serialization
// ===========================================================

std::string Chess::initialStateString()
{
    return stateString();
}

std::string Chess::stateString()
{
    std::string s;
    s.reserve(64);
    _grid->forEachSquare([&](ChessSquare* sq, int x, int y) {
        s += pieceNotation(x, y);
    });
    return s;
}

void Chess::setStateString(const std::string &s)
{
    _grid->forEachSquare([&](ChessSquare* sq, int x, int y) {
        char c = s[y * 8 + x];
        if (c == '0') sq->setBit(nullptr);
    });
}

void Chess::stopGame()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
}
