#include "tetris_screen.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>

#include "lvgl.h"

#include "screen_manager.hpp"
#include "swirski_ui.hpp"

namespace swirski::screens::tetris_screen
{
    namespace
    {
        constexpr int columns = 10;
        constexpr int rows = 18;
        constexpr int cellSize = 9;
        constexpr int boardWidth = columns * cellSize;
        constexpr int boardHeight = rows * cellSize;

        constexpr std::array<std::uint16_t, 7> pieceMasks{
            0x00F0, // I
            0x0660, // O
            0x0270, // T
            0x0360, // S
            0x0630, // Z
            0x0710, // J
            0x0740  // L
        };

        std::array<std::array<std::uint8_t, columns>, rows> board{};
        alignas(LV_DRAW_BUF_ALIGN)
            std::array<
                std::uint8_t,
                LV_DRAW_BUF_SIZE(
                    boardWidth,
                    boardHeight,
                    LV_COLOR_FORMAT_RGB565)>
                boardBuffer{};
        lv_draw_buf_t boardDrawBuffer{};

        lv_obj_t *boardCanvas = nullptr;
        lv_obj_t *scoreLabel = nullptr;
        lv_obj_t *linesLabel = nullptr;
        lv_obj_t *statusLabel = nullptr;
        lv_timer_t *gameTimer = nullptr;

        int currentPiece = 0;
        int nextPiece = 0;
        int pieceX = 3;
        int pieceY = -1;
        int rotation = 0;
        int score = 0;
        int clearedLines = 0;
        bool gameOver = false;
        bool randomSeeded = false;

        bool pieceBlock(int piece, int x, int y, int pieceRotation)
        {
            int sourceX = x;
            int sourceY = y;

            switch (pieceRotation % 4)
            {
            case 1:
                sourceX = y;
                sourceY = 3 - x;
                break;
            case 2:
                sourceX = 3 - x;
                sourceY = 3 - y;
                break;
            case 3:
                sourceX = 3 - y;
                sourceY = x;
                break;
            default:
                break;
            }

            const int bit = sourceY * 4 + sourceX;
            return (pieceMasks[piece] & (1U << bit)) != 0;
        }

        lv_color_t pieceColor(int piece)
        {
            switch (piece)
            {
            case 0:
                return lv_color_hex(0x00C2FF);
            case 1:
                return swirski::ui::swirski_ui::color::accentWarm();
            case 2:
                return lv_color_hex(0xA855F7);
            case 3:
                return lv_color_hex(0x22C55E);
            case 4:
                return swirski::ui::swirski_ui::color::accentBright();
            case 5:
                return swirski::ui::swirski_ui::color::accent();
            default:
                return lv_color_hex(0xFF7A00);
            }
        }

        bool canPlace(int x, int y, int pieceRotation)
        {
            for (int localY = 0; localY < 4; ++localY)
            {
                for (int localX = 0; localX < 4; ++localX)
                {
                    if (!pieceBlock(
                            currentPiece,
                            localX,
                            localY,
                            pieceRotation))
                    {
                        continue;
                    }

                    const int boardX = x + localX;
                    const int boardY = y + localY;

                    if (
                        boardX < 0 ||
                        boardX >= columns ||
                        boardY >= rows ||
                        (boardY >= 0 && board[boardY][boardX] != 0))
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        void updateLabels()
        {
            lv_label_set_text_fmt(scoreLabel, "SCORE\n%d", score);
            lv_label_set_text_fmt(linesLabel, "LINES\n%d", clearedLines);
            lv_label_set_text(
                statusLabel,
                gameOver ? "GAME OVER" : "PLAYING");
        }

        bool activePieceAt(int x, int y)
        {
            const int localX = x - pieceX;
            const int localY = y - pieceY;

            return
                !gameOver &&
                localX >= 0 && localX < 4 &&
                localY >= 0 && localY < 4 &&
                pieceBlock(
                    currentPiece,
                    localX,
                    localY,
                    rotation);
        }

        void updateBoard()
        {
            lv_canvas_fill_bg(
                boardCanvas,
                swirski::ui::swirski_ui::color::ink(),
                LV_OPA_COVER);

            for (int y = 0; y < rows; ++y)
            {
                for (int x = 0; x < columns; ++x)
                {
                    lv_color_t color =
                        swirski::ui::swirski_ui::color::surfaceSoft();

                    if (activePieceAt(x, y))
                    {
                        color = pieceColor(currentPiece);
                    }
                    else if (board[y][x] != 0)
                    {
                        color = pieceColor(board[y][x] - 1);
                    }

                    const std::uint16_t pixelColor =
                        lv_color_to_u16(color);

                    for (int pixelY = 0; pixelY < cellSize - 1; ++pixelY)
                    {
                        auto *pixelRow =
                            reinterpret_cast<std::uint16_t *>(
                                static_cast<std::uint8_t *>(
                                    boardDrawBuffer.data) +
                                (y * cellSize + pixelY) *
                                    boardDrawBuffer.header.stride);

                        for (int pixelX = 0; pixelX < cellSize - 1; ++pixelX)
                        {
                            pixelRow[x * cellSize + pixelX] = pixelColor;
                        }
                    }
                }
            }

            lv_obj_invalidate(boardCanvas);
            updateLabels();
        }

        int clearCompletedLines()
        {
            int lines = 0;

            for (int y = rows - 1; y >= 0;)
            {
                bool full = true;

                for (int x = 0; x < columns; ++x)
                {
                    if (board[y][x] == 0)
                    {
                        full = false;
                        break;
                    }
                }

                if (!full)
                {
                    y--;
                    continue;
                }

                for (int moveY = y; moveY > 0; --moveY)
                {
                    board[moveY] = board[moveY - 1];
                }

                board[0].fill(0);
                lines++;
            }

            return lines;
        }

        void spawnPiece()
        {
            currentPiece = nextPiece;
            nextPiece = std::rand() % pieceMasks.size();
            pieceX = 3;
            pieceY = -1;
            rotation = 0;

            if (!canPlace(pieceX, pieceY, rotation))
            {
                gameOver = true;
            }
        }

        void lockPiece()
        {
            for (int localY = 0; localY < 4; ++localY)
            {
                for (int localX = 0; localX < 4; ++localX)
                {
                    if (!pieceBlock(
                            currentPiece,
                            localX,
                            localY,
                            rotation))
                    {
                        continue;
                    }

                    const int boardX = pieceX + localX;
                    const int boardY = pieceY + localY;

                    if (boardY < 0)
                    {
                        gameOver = true;
                        updateBoard();
                        return;
                    }

                    board[boardY][boardX] =
                        static_cast<std::uint8_t>(currentPiece + 1);
                }
            }

            const int lines = clearCompletedLines();
            constexpr std::array<int, 5> lineScores{
                0, 100, 300, 500, 800};

            clearedLines += lines;
            score += lineScores[lines];
            spawnPiece();
            updateBoard();
        }

        void updateGame(lv_timer_t *)
        {
            if (gameOver)
            {
                return;
            }

            if (canPlace(pieceX, pieceY + 1, rotation))
            {
                pieceY++;
                updateBoard();
            }
            else
            {
                lockPiece();
            }
        }

        void startGame()
        {
            for (auto &row : board)
            {
                row.fill(0);
            }

            score = 0;
            clearedLines = 0;
            gameOver = false;
            nextPiece = std::rand() % pieceMasks.size();
            spawnPiece();
            updateBoard();
        }

        void cleanUp(lv_event_t *)
        {
            if (gameTimer != nullptr)
            {
                lv_timer_delete(gameTimer);
                gameTimer = nullptr;
            }

            boardCanvas = nullptr;
            scoreLabel = nullptr;
            linesLabel = nullptr;
            statusLabel = nullptr;
        }
    }

    void render()
    {
        if (!randomSeeded)
        {
            std::srand(static_cast<unsigned int>(lv_tick_get()));
            randomSeeded = true;
        }

        lv_obj_t *pageRoot =
            swirski::screens::manager::createPageRoot();

        lv_obj_add_event_cb(
            pageRoot,
            cleanUp,
            LV_EVENT_DELETE,
            nullptr);

        lv_obj_t *boardRoot = lv_obj_create(pageRoot);

        lv_obj_set_size(
            boardRoot,
            boardWidth + 6,
            boardHeight + 6);
        lv_obj_align(boardRoot, LV_ALIGN_TOP_LEFT, 34, 8);
        lv_obj_set_style_pad_all(boardRoot, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(boardRoot, 0, LV_PART_MAIN);
        lv_obj_set_style_border_width(boardRoot, 3, LV_PART_MAIN);
        lv_obj_set_style_border_color(
            boardRoot,
            swirski::ui::swirski_ui::color::ink(),
            LV_PART_MAIN);
        lv_obj_set_style_bg_color(
            boardRoot,
            swirski::ui::swirski_ui::color::ink(),
            LV_PART_MAIN);
        lv_obj_set_style_bg_opa(boardRoot, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_clear_flag(boardRoot, LV_OBJ_FLAG_SCROLLABLE);

        lv_draw_buf_init(
            &boardDrawBuffer,
            boardWidth,
            boardHeight,
            LV_COLOR_FORMAT_RGB565,
            LV_DRAW_BUF_STRIDE(
                boardWidth,
                LV_COLOR_FORMAT_RGB565),
            boardBuffer.data(),
            boardBuffer.size());
        lv_draw_buf_set_flag(
            &boardDrawBuffer,
            LV_IMAGE_FLAGS_MODIFIABLE);
        boardCanvas = lv_canvas_create(boardRoot);
        lv_canvas_set_draw_buf(boardCanvas, &boardDrawBuffer);
        lv_obj_set_pos(boardCanvas, 0, 0);

        lv_obj_t *badge =
            swirski::ui::swirski_ui::createBadge(
                pageRoot,
                "Tetris");
        lv_obj_set_pos(badge, 158, 14);

        scoreLabel = lv_label_create(pageRoot);
        lv_obj_set_pos(scoreLabel, 158, 50);

        linesLabel = lv_label_create(pageRoot);
        lv_obj_set_pos(linesLabel, 158, 94);

        statusLabel = lv_label_create(pageRoot);
        lv_obj_set_pos(statusLabel, 158, 142);
        lv_obj_set_style_text_color(
            statusLabel,
            swirski::ui::swirski_ui::color::accent(),
            LV_PART_MAIN);

        startGame();

        gameTimer = lv_timer_create(
            updateGame,
            450,
            nullptr);
    }

    void handleInput(swirski::input::input_action action)
    {
        switch (action)
        {
        case swirski::input::input_action::Previous:
            if (!gameOver && canPlace(pieceX - 1, pieceY, rotation))
            {
                pieceX--;
                updateBoard();
            }
            break;

        case swirski::input::input_action::Next:
            if (!gameOver && canPlace(pieceX + 1, pieceY, rotation))
            {
                pieceX++;
                updateBoard();
            }
            break;

        case swirski::input::input_action::Confirm:
        {
            if (gameOver)
            {
                startGame();
                break;
            }

            constexpr std::array<int, 3> rotationOffsets{0, -1, 1};

            for (int offset : rotationOffsets)
            {
                const int nextRotation = (rotation + 1) % 4;

                if (canPlace(pieceX + offset, pieceY, nextRotation))
                {
                    pieceX += offset;
                    rotation = nextRotation;
                    updateBoard();
                    break;
                }
            }
            break;
        }

        case swirski::input::input_action::Back:
            swirski::screens::manager::showScreen(
                swirski::screens::manager::Screen::Games);
            break;

        case swirski::input::input_action::Home:
            swirski::screens::manager::showScreen(
                swirski::screens::manager::Screen::Home);
            break;
        }
    }
}
