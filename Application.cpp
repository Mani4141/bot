#include "Application.h"
#include "imgui/imgui.h"
#include "classes/TicTacToe.h"
#include "classes/Checkers.h"
#include "classes/Othello.h"
#include "classes/Connect4.h"
#include "classes/Chess.h"
#include "classes/Bitboard.h"   // for BitMove

namespace ClassGame {
        //
        // our global variables
        //
        Game *game = nullptr;
        bool gameOver = false;
        int gameWinner = -1;

        // simple debug state for the move generator
        static int  g_lastMoveCount = -1;
        static bool g_moveGenRan    = false;

        //
        // game starting point
        // this is called by the main render loop in main.cpp
        //
        void GameStartUp() 
        {
            game = nullptr;
            g_lastMoveCount = -1;
            g_moveGenRan    = false;
        }

        //
        // game render loop
        // this is called by the main render loop in main.cpp
        //
        void RenderGame() 
        {
                ImGui::DockSpaceOverViewport();

                //ImGui::ShowDemoWindow();

                ImGui::Begin("Settings");

                if (gameOver) {
                    ImGui::Text("Game Over!");
                    ImGui::Text("Winner: %d", gameWinner);
                    if (ImGui::Button("Reset Game")) {
                        game->stopGame();
                        game->setUpBoard();
                        gameOver = false;
                        gameWinner = -1;
                        g_lastMoveCount = -1;
                        g_moveGenRan    = false;
                    }
                }
                if (!game) {
                    if (ImGui::Button("Start Tic-Tac-Toe")) {
                        game = new TicTacToe();
                        game->setUpBoard();
                    }
                    if (ImGui::Button("Start Checkers")) {
                        game = new Checkers();
                        game->setUpBoard();
                    }
                    if (ImGui::Button("Start Othello")) {
                        game = new Othello();
                        game->setUpBoard();
                    }
                    if (ImGui::Button("Start Connect 4")) {
                        game = new Connect4();
                        game->setUpBoard();
                    }
                    if (ImGui::Button("Start Chess")) {
                        game = new Chess();
                        game->setUpBoard();
                    }
                } else {
                    ImGui::Text("Current Player Number: %d", game->getCurrentPlayer()->playerNumber());
                    std::string stateString = game->stateString();
                    int stride = game->_gameOptions.rowX;
                    int height = game->_gameOptions.rowY;

                    for (int y = 0; y < height; y++) {
                        ImGui::Text("%s", stateString.substr(y * stride, stride).c_str());
                    }
                    ImGui::TextWrapped("Current Board State: %s", game->stateString().c_str());

                    // --- Debug move generator (for Chess) ---
                    if (ImGui::Button("Debug Move Generator")) {
                        // assume you only press this while Chess is running
                        Chess* chessGame = static_cast<Chess*>(game);
                        BitMove moves[256];
                        int moveCount = 0;
                        chessGame->generateMovesForCurrentPlayer(moves, moveCount);

                        g_lastMoveCount = moveCount;
                        g_moveGenRan    = true;

                        // place a breakpoint on the next line if you want to inspect moves[] in the debugger
                        (void)moves;
                    }

                    if (g_moveGenRan) {
                        ImGui::Text("Last move generation: %d moves", g_lastMoveCount);
                        if (g_lastMoveCount == 20) {
                            ImGui::Text("✓ Starting position looks correct (20 moves).");
                        }
                        else if (g_lastMoveCount >= 0) {
                            ImGui::Text("Note: expected 20 moves from initial position.");
                        }
                    }
                }
                ImGui::End();

                // Game window
                ImGui::Begin("GameWindow");
                if (game) {
                    if (game->gameHasAI() && (game->getCurrentPlayer()->isAIPlayer() || game->_gameOptions.AIvsAI))
                    {
                        game->updateAI();
                    }

                    // Child region that holds the board; dragging here won't move the window
                    ImGui::BeginChild(
                        "BoardRegion",
                        ImVec2(0, 0),        // take all remaining space
                        false,
                        ImGuiWindowFlags_NoMove   // prevent child from being draggable
                    );

                    game->drawFrame();

                    ImGui::EndChild();
                }
                ImGui::End();
        }

        //
        // end turn is called by the game code at the end of each turn
        // this is where we check for a winner
        //
        void EndOfTurn() 
        {
            Player *winner = game->checkForWinner();
            if (winner)
            {
                gameOver = true;
                gameWinner = winner->playerNumber();
            }
            if (game->checkForDraw()) {
                gameOver = true;
                gameWinner = -1;
            }
        }
}
