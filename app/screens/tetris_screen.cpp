#include "tetris_screen.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "lvgl.h"

#include "screen_manager.hpp"
#include "swirski_ui.hpp"

#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#include "esp_log.h"
#endif

namespace swirski::screens::tetris_screen
{
    namespace
    {
        constexpr int columns = 10;
        constexpr int rows = 18;
        constexpr int cellSize = 9;

        constexpr int boardWidth =
            columns * cellSize;

        constexpr int boardHeight =
            rows * cellSize;

        constexpr std::uint32_t gamePeriodMs = 450;

        /*
         * Each piece is stored as a 4 × 4 bit mask.
         */
        constexpr std::array<std::uint16_t, 7> pieceMasks{
            0x00F0, // I
            0x0660, // O
            0x0270, // T
            0x0360, // S
            0x0630, // Z
            0x0710, // J
            0x0740  // L
        };

        /*
         * Zero means empty.
         *
         * Values 1–7 represent the seven pieces, allowing us to retain
         * each locked piece's colour.
         */
        std::array<
            std::array<std::uint8_t, columns>,
            rows>
            board{};

        int currentPiece = 0;
        int nextPiece = 0;

        int pieceX = 3;
        int pieceY = -1;
        int rotation = 0;

        int score = 0;
        int clearedLines = 0;

        bool gameOver = false;
        bool randomSeeded = false;

        constexpr std::size_t boardBufferSize =
            LV_DRAW_BUF_SIZE(
                boardWidth,
                boardHeight,
                LV_COLOR_FORMAT_RGB565);

        /*
         * Allocated lazily when Tetris is first opened.
         *
         * On ESP32 this must be allocated from PSRAM. It is deliberately
         * retained and reused rather than freed while LVGL is deleting the
         * canvas object.
         */
        std::uint8_t *boardBuffer = nullptr;

        lv_draw_buf_t boardDrawBuffer{};

        lv_obj_t *boardCanvas = nullptr;
        lv_obj_t *scoreLabel = nullptr;
        lv_obj_t *linesLabel = nullptr;
        lv_obj_t *statusLabel = nullptr;

        lv_timer_t *gameTimer = nullptr;

        bool allocateBoardBuffer()
        {
            if (boardBuffer != nullptr)
            {
                return true;
            }

#ifdef ESP_PLATFORM
            boardBuffer =
                static_cast<std::uint8_t *>(
                    heap_caps_aligned_alloc(
                        LV_DRAW_BUF_ALIGN,
                        boardBufferSize,
                        MALLOC_CAP_SPIRAM |
                            MALLOC_CAP_8BIT));

            /*
             * Do not fall back to internal RAM.
             *
             * The canvas is large enough that placing it in internal RAM
             * could leave too little memory for BLE.
             */
            if (boardBuffer == nullptr)
            {
                ESP_LOGE(
                    "TETRIS",
                    "Could not allocate %u-byte canvas in PSRAM",
                    static_cast<unsigned>(
                        boardBufferSize));

                return false;
            }

            ESP_LOGI(
                "TETRIS",
                "Allocated %u-byte canvas in PSRAM. "
                "Internal free: %u, PSRAM free: %u",
                static_cast<unsigned>(
                    boardBufferSize),
                static_cast<unsigned>(
                    heap_caps_get_free_size(
                        MALLOC_CAP_INTERNAL |
                        MALLOC_CAP_8BIT)),
                static_cast<unsigned>(
                    heap_caps_get_free_size(
                        MALLOC_CAP_SPIRAM |
                        MALLOC_CAP_8BIT)));
#else
            boardBuffer =
                static_cast<std::uint8_t *>(
                    lv_malloc(boardBufferSize));

            if (boardBuffer == nullptr)
            {
                return false;
            }
#endif

            std::memset(
                boardBuffer,
                0,
                boardBufferSize);

            return true;
        }

        void stopGameTimer()
        {
            if (gameTimer == nullptr)
            {
                return;
            }

            lv_timer_delete(gameTimer);
            gameTimer = nullptr;
        }

        void pauseGameTimer()
        {
            if (gameTimer != nullptr)
            {
                lv_timer_pause(gameTimer);
            }
        }

        void resumeGameTimer()
        {
            if (gameTimer != nullptr)
            {
                lv_timer_resume(gameTimer);
            }
        }

        bool pieceBlock(
            int piece,
            int x,
            int y,
            int pieceRotation)
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

            const int bit =
                sourceY * 4 + sourceX;

            return (
                       pieceMasks[piece] &
                       (1U << bit)) != 0;
        }

        lv_color_t pieceColor(int piece)
        {
            switch (piece)
            {
            case 0:
                return lv_color_hex(0x00C2FF);

            case 1:
                return swirski::ui::swirski_ui::
                    color::accentWarm();

            case 2:
                return lv_color_hex(0xA855F7);

            case 3:
                return lv_color_hex(0x22C55E);

            case 4:
                return swirski::ui::swirski_ui::
                    color::accentBright();

            case 5:
                return swirski::ui::swirski_ui::
                    color::accent();

            default:
                return lv_color_hex(0xFF7A00);
            }
        }

        bool canPlace(
            int x,
            int y,
            int pieceRotation)
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

                    const int boardX =
                        x + localX;

                    const int boardY =
                        y + localY;

                    if (
                        boardX < 0 ||
                        boardX >= columns ||
                        boardY >= rows)
                    {
                        return false;
                    }

                    /*
                     * Negative Y is allowed while a piece is entering
                     * from above the board.
                     */
                    if (
                        boardY >= 0 &&
                        board[boardY][boardX] != 0)
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        void updateLabels()
        {
            if (
                scoreLabel == nullptr ||
                linesLabel == nullptr ||
                statusLabel == nullptr)
            {
                return;
            }

            lv_label_set_text_fmt(
                scoreLabel,
                "SCORE\n%d",
                score);

            lv_label_set_text_fmt(
                linesLabel,
                "LINES\n%d",
                clearedLines);

            lv_label_set_text(
                statusLabel,
                gameOver
                    ? "GAME OVER"
                    : "PLAYING");
        }

        bool activePieceAt(
            int x,
            int y)
        {
            const int localX =
                x - pieceX;

            const int localY =
                y - pieceY;

            return !gameOver &&
                   localX >= 0 &&
                   localX < 4 &&
                   localY >= 0 &&
                   localY < 4 &&
                   pieceBlock(
                       currentPiece,
                       localX,
                       localY,
                       rotation);
        }

        void drawCell(
            int x,
            int y,
            lv_color_t color)
        {
            if (
                x < 0 ||
                x >= columns ||
                y < 0 ||
                y >= rows ||
                boardDrawBuffer.data == nullptr)
            {
                return;
            }

            const std::uint16_t pixelColor =
                lv_color_to_u16(color);

            for (
                int pixelY = 0;
                pixelY < cellSize - 1;
                ++pixelY)
            {
                auto *pixelRow =
                    reinterpret_cast<std::uint16_t *>(
                        static_cast<std::uint8_t *>(
                            boardDrawBuffer.data) +
                        (y * cellSize +
                         pixelY) *
                            boardDrawBuffer.header.stride);

                for (
                    int pixelX = 0;
                    pixelX < cellSize - 1;
                    ++pixelX)
                {
                    pixelRow[x * cellSize +
                             pixelX] = pixelColor;
                }
            }
        }

        void updateBoard()
        {
            if (
                boardCanvas == nullptr ||
                boardDrawBuffer.data == nullptr)
            {
                return;
            }

            lv_canvas_fill_bg(
                boardCanvas,
                swirski::ui::swirski_ui::
                    color::ink(),
                LV_OPA_COVER);

            for (int y = 0; y < rows; ++y)
            {
                for (int x = 0; x < columns; ++x)
                {
                    lv_color_t color =
                        swirski::ui::swirski_ui::
                            color::surfaceSoft();

                    if (activePieceAt(x, y))
                    {
                        color =
                            pieceColor(currentPiece);
                    }
                    else if (board[y][x] != 0)
                    {
                        color =
                            pieceColor(
                                board[y][x] - 1);
                    }

                    drawCell(
                        x,
                        y,
                        color);
                }
            }

            lv_obj_invalidate(boardCanvas);
            updateLabels();
        }

        int clearCompletedLines()
        {
            int lines = 0;

            /*
             * Do not decrement y immediately after clearing a line.
             * A second full line may have shifted into the same position.
             */
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

                for (
                    int moveY = y;
                    moveY > 0;
                    --moveY)
                {
                    board[moveY] =
                        board[moveY - 1];
                }

                board[0].fill(0);
                lines++;
            }

            return lines;
        }

        bool spawnPiece()
        {
            currentPiece = nextPiece;

            nextPiece =
                std::rand() %
                static_cast<int>(
                    pieceMasks.size());

            pieceX = 3;
            pieceY = -1;
            rotation = 0;

            if (!canPlace(
                    pieceX,
                    pieceY,
                    rotation))
            {
                gameOver = true;
                return false;
            }

            return true;
        }

        void finishGame(lv_timer_t *timer)
        {
            gameOver = true;

            updateBoard();

            if (timer != nullptr)
            {
                lv_timer_pause(timer);
            }
        }

        void lockPiece(lv_timer_t *timer)
        {
            /*
             * Validate the complete piece before writing any blocks.
             *
             * This avoids partially writing a piece if part of it is still
             * above the top of the board.
             */
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

                    const int boardY =
                        pieceY + localY;

                    if (boardY < 0)
                    {
                        finishGame(timer);
                        return;
                    }
                }
            }

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

                    const int boardX =
                        pieceX + localX;

                    const int boardY =
                        pieceY + localY;

                    if (
                        boardX < 0 ||
                        boardX >= columns ||
                        boardY < 0 ||
                        boardY >= rows)
                    {
                        finishGame(timer);
                        return;
                    }

                    board[boardY][boardX] =
                        static_cast<std::uint8_t>(
                            currentPiece + 1);
                }
            }

            const int lines =
                clearCompletedLines();

            constexpr std::array<int, 5>
                lineScores{
                    0,
                    100,
                    300,
                    500,
                    800};

            clearedLines += lines;

            if (
                lines >= 0 &&
                lines <
                    static_cast<int>(
                        lineScores.size()))
            {
                score += lineScores[lines];
            }

            if (!spawnPiece())
            {
                finishGame(timer);
                return;
            }

            updateBoard();
        }

        void updateGame(lv_timer_t *timer)
        {
            if (
                gameOver ||
                boardCanvas == nullptr)
            {
                return;
            }

            if (canPlace(
                    pieceX,
                    pieceY + 1,
                    rotation))
            {
                pieceY++;
                updateBoard();
                return;
            }

            lockPiece(timer);
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

            nextPiece =
                std::rand() %
                static_cast<int>(
                    pieceMasks.size());

            if (!spawnPiece())
            {
                gameOver = true;
            }

            updateBoard();
        }

        void cleanUp(lv_event_t *)
        {
            stopGameTimer();

            /*
             * LVGL deletes these objects because they are descendants of
             * pageRoot. These are only non-owning pointers.
             *
             * The PSRAM board buffer is intentionally retained for reuse.
             */
            boardCanvas = nullptr;
            scoreLabel = nullptr;
            linesLabel = nullptr;
            statusLabel = nullptr;
        }
    }

    void render()
    {
        /*
         * Defensive cleanup in case render() is called while an old Tetris
         * timer still exists.
         */
        stopGameTimer();

        if (!randomSeeded)
        {
            std::srand(
                static_cast<unsigned int>(
                    lv_tick_get()));

            randomSeeded = true;
        }

        lv_obj_t *pageRoot =
            swirski::screens::manager::
                createPageRoot();

        lv_obj_add_event_cb(
            pageRoot,
            cleanUp,
            LV_EVENT_DELETE,
            nullptr);

        if (!allocateBoardBuffer())
        {
            lv_obj_t *errorLabel =
                lv_label_create(pageRoot);

            lv_label_set_text(
                errorLabel,
                "Unable to allocate\n"
                "Tetris canvas in PSRAM");

            lv_obj_center(errorLabel);
            return;
        }

        lv_obj_t *boardRoot =
            lv_obj_create(pageRoot);

        lv_obj_set_size(
            boardRoot,
            boardWidth + 6,
            boardHeight + 6);

        lv_obj_align(
            boardRoot,
            LV_ALIGN_TOP_LEFT,
            34,
            8);

        lv_obj_set_style_pad_all(
            boardRoot,
            0,
            LV_PART_MAIN);

        lv_obj_set_style_radius(
            boardRoot,
            0,
            LV_PART_MAIN);

        lv_obj_set_style_border_width(
            boardRoot,
            3,
            LV_PART_MAIN);

        lv_obj_set_style_border_color(
            boardRoot,
            swirski::ui::swirski_ui::
                color::ink(),
            LV_PART_MAIN);

        lv_obj_set_style_bg_color(
            boardRoot,
            swirski::ui::swirski_ui::
                color::ink(),
            LV_PART_MAIN);

        lv_obj_set_style_bg_opa(
            boardRoot,
            LV_OPA_COVER,
            LV_PART_MAIN);

        lv_obj_clear_flag(
            boardRoot,
            LV_OBJ_FLAG_SCROLLABLE);

        lv_draw_buf_init(
            &boardDrawBuffer,
            boardWidth,
            boardHeight,
            LV_COLOR_FORMAT_RGB565,
            LV_DRAW_BUF_STRIDE(
                boardWidth,
                LV_COLOR_FORMAT_RGB565),
            boardBuffer,
            boardBufferSize);

        lv_draw_buf_set_flag(
            &boardDrawBuffer,
            LV_IMAGE_FLAGS_MODIFIABLE);

        boardCanvas =
            lv_canvas_create(boardRoot);

        lv_canvas_set_draw_buf(
            boardCanvas,
            &boardDrawBuffer);

        lv_obj_set_pos(
            boardCanvas,
            0,
            0);

        lv_obj_t *badge =
            swirski::ui::swirski_ui::
                createBadge(
                    pageRoot,
                    "Tetris");

        lv_obj_set_pos(
            badge,
            158,
            14);

        scoreLabel =
            lv_label_create(pageRoot);

        lv_obj_set_pos(
            scoreLabel,
            158,
            50);

        linesLabel =
            lv_label_create(pageRoot);

        lv_obj_set_pos(
            linesLabel,
            158,
            94);

        statusLabel =
            lv_label_create(pageRoot);

        lv_obj_set_pos(
            statusLabel,
            158,
            142);

        lv_obj_set_style_text_color(
            statusLabel,
            swirski::ui::swirski_ui::
                color::accent(),
            LV_PART_MAIN);

        startGame();

        gameTimer =
            lv_timer_create(
                updateGame,
                gamePeriodMs,
                nullptr);

        if (gameOver)
        {
            pauseGameTimer();
        }
    }

    void handleInput(
        swirski::input::input_action action)
    {
        switch (action)
        {
        case swirski::input::input_action::Previous:
            if (
                !gameOver &&
                canPlace(
                    pieceX - 1,
                    pieceY,
                    rotation))
            {
                pieceX--;
                updateBoard();
            }
            break;

        case swirski::input::input_action::Next:
            if (
                !gameOver &&
                canPlace(
                    pieceX + 1,
                    pieceY,
                    rotation))
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

                if (!gameOver)
                {
                    resumeGameTimer();
                }

                break;
            }

            const int nextRotation =
                (rotation + 1) % 4;

            /*
             * Try the rotation normally, then perform a small wall kick
             * to the left or right.
             */
            constexpr std::array<int, 3>
                rotationOffsets{
                    0,
                    -1,
                    1};

            for (const int offset : rotationOffsets)
            {
                if (canPlace(
                        pieceX + offset,
                        pieceY,
                        nextRotation))
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
            stopGameTimer();

            swirski::screens::manager::showScreen(
                swirski::screens::manager::
                    Screen::Games);
            break;

        case swirski::input::input_action::Home:
            stopGameTimer();

            swirski::screens::manager::showScreen(
                swirski::screens::manager::
                    Screen::Home);
            break;
        }
    }
}