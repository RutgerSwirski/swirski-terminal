#include "snake_screen.hpp"

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

namespace swirski::screens::snake_screen
{
    namespace
    {
        constexpr int columns = 18;
        constexpr int rows = 14;
        constexpr int cellSize = 8;

        constexpr int boardWidth =
            columns * cellSize;

        constexpr int boardHeight =
            rows * cellSize;

        constexpr std::uint32_t gamePeriodMs = 180;

        struct Point
        {
            int x;
            int y;
        };

        enum class Direction
        {
            Up,
            Right,
            Down,
            Left
        };

        std::array<Point, columns * rows> snake{};

        int snakeLength = 4;
        Point food{12, 7};
        Direction direction = Direction::Right;
        int score = 0;

        bool playing = false;
        bool gameOver = false;
        bool randomSeeded = false;

        constexpr std::size_t boardBufferSize =
            LV_DRAW_BUF_SIZE(
                boardWidth,
                boardHeight,
                LV_COLOR_FORMAT_RGB565);

        /*
         * This is allocated lazily when Snake is first opened.
         *
         * On ESP32 it must come from PSRAM. We intentionally retain the
         * allocation for the lifetime of the application and reuse it each
         * time Snake opens.
         *
         * This prevents:
         * - permanent internal-RAM pressure before Snake is opened;
         * - freeing the backing buffer while LVGL is deleting the canvas;
         * - repeated allocation and fragmentation every time Snake opens.
         */
        std::uint8_t *boardBuffer = nullptr;

        lv_draw_buf_t boardDrawBuffer{};

        lv_obj_t *boardCanvas = nullptr;
        lv_obj_t *scoreLabel = nullptr;
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

            if (boardBuffer == nullptr)
            {
                ESP_LOGE(
                    "SNAKE",
                    "Could not allocate %u-byte canvas in PSRAM",
                    static_cast<unsigned>(
                        boardBufferSize));

                return false;
            }

            ESP_LOGI(
                "SNAKE",
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
            playing = false;

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

        bool snakeAt(
            Point point,
            int segmentCount)
        {
            if (segmentCount < 0)
            {
                segmentCount = 0;
            }

            if (segmentCount > snakeLength)
            {
                segmentCount = snakeLength;
            }

            for (int i = 0; i < segmentCount; ++i)
            {
                if (
                    snake[i].x == point.x &&
                    snake[i].y == point.y)
                {
                    return true;
                }
            }

            return false;
        }

        bool snakeAt(Point point)
        {
            return snakeAt(
                point,
                snakeLength);
        }

        /*
         * Select a random free cell without repeatedly guessing.
         *
         * The old do/while approach could take a very long time when the
         * board was nearly full and would loop forever when completely full.
         */
        bool placeFood()
        {
            const int totalCells =
                columns * rows;

            const int freeCellCount =
                totalCells - snakeLength;

            if (freeCellCount <= 0)
            {
                playing = false;
                gameOver = true;
                return false;
            }

            int selectedFreeCell =
                std::rand() % freeCellCount;

            for (int y = 0; y < rows; ++y)
            {
                for (int x = 0; x < columns; ++x)
                {
                    const Point candidate{x, y};

                    if (snakeAt(candidate))
                    {
                        continue;
                    }

                    if (selectedFreeCell == 0)
                    {
                        food = candidate;
                        return true;
                    }

                    selectedFreeCell--;
                }
            }

            playing = false;
            gameOver = true;
            return false;
        }

        void drawCell(
            Point point,
            lv_color_t color)
        {
            if (
                point.x < 0 ||
                point.x >= columns ||
                point.y < 0 ||
                point.y >= rows ||
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
                        (
                            point.y * cellSize +
                            pixelY
                        ) *
                            boardDrawBuffer.header.stride);

                for (
                    int pixelX = 0;
                    pixelX < cellSize - 1;
                    ++pixelX)
                {
                    pixelRow[
                        point.x * cellSize +
                        pixelX] = pixelColor;
                }
            }
        }

        void updateScreen()
        {
            if (
                boardCanvas == nullptr ||
                scoreLabel == nullptr ||
                statusLabel == nullptr ||
                boardDrawBuffer.data == nullptr)
            {
                return;
            }

            lv_canvas_fill_bg(
                boardCanvas,
                swirski::ui::swirski_ui::color::ink(),
                LV_OPA_COVER);

            for (int y = 0; y < rows; ++y)
            {
                for (int x = 0; x < columns; ++x)
                {
                    drawCell(
                        {x, y},
                        swirski::ui::swirski_ui::color::
                            surfaceSoft());
                }
            }

            drawCell(
                food,
                swirski::ui::swirski_ui::color::
                    accentWarm());

            for (
                int i = snakeLength - 1;
                i >= 0;
                --i)
            {
                drawCell(
                    snake[i],
                    i == 0
                        ? swirski::ui::swirski_ui::
                              color::accentBright()
                        : swirski::ui::swirski_ui::
                              color::accent());
            }

            lv_obj_invalidate(boardCanvas);

            lv_label_set_text_fmt(
                scoreLabel,
                "SCORE\n%d",
                score);

            lv_label_set_text(
                statusLabel,
                gameOver
                    ? "GAME OVER"
                : playing
                    ? "PLAYING"
                    : "PAUSED");
        }

        void startGame()
        {
            snakeLength = 4;

            snake[0] = {8, 7};
            snake[1] = {7, 7};
            snake[2] = {6, 7};
            snake[3] = {5, 7};

            direction = Direction::Right;
            score = 0;
            gameOver = false;
            playing = true;

            if (!placeFood())
            {
                playing = false;
                gameOver = true;
            }

            updateScreen();
        }

        Point nextHead()
        {
            Point head = snake[0];

            switch (direction)
            {
            case Direction::Up:
                head.y--;
                break;

            case Direction::Right:
                head.x++;
                break;

            case Direction::Down:
                head.y++;
                break;

            case Direction::Left:
                head.x--;
                break;
            }

            return head;
        }

        void finishGame(lv_timer_t *timer)
        {
            gameOver = true;
            playing = false;

            updateScreen();

            if (timer != nullptr)
            {
                lv_timer_pause(timer);
            }
        }

        void updateGame(lv_timer_t *timer)
        {
            if (
                !playing ||
                gameOver ||
                boardCanvas == nullptr)
            {
                return;
            }

            const Point head = nextHead();

            if (
                head.x < 0 ||
                head.x >= columns ||
                head.y < 0 ||
                head.y >= rows)
            {
                finishGame(timer);
                return;
            }

            const bool ateFood =
                head.x == food.x &&
                head.y == food.y;

            /*
             * When the snake is not eating, its tail moves away during this
             * tick. Therefore the old tail cell should not count as a
             * collision.
             *
             * When eating, the tail remains, so all segments are checked.
             */
            const int collisionSegmentCount =
                ateFood
                    ? snakeLength
                    : snakeLength - 1;

            if (
                snakeAt(
                    head,
                    collisionSegmentCount))
            {
                finishGame(timer);
                return;
            }

            if (
                ateFood &&
                snakeLength <
                    static_cast<int>(snake.size()))
            {
                snakeLength++;
                score += 10;
            }

            for (
                int i = snakeLength - 1;
                i > 0;
                --i)
            {
                snake[i] = snake[i - 1];
            }

            snake[0] = head;

            if (ateFood)
            {
                if (!placeFood())
                {
                    updateScreen();
                    pauseGameTimer();
                    return;
                }
            }

            updateScreen();
        }

        void turnLeft()
        {
            direction =
                static_cast<Direction>(
                    (
                        static_cast<int>(
                            direction) +
                        3
                    ) %
                    4);
        }

        void turnRight()
        {
            direction =
                static_cast<Direction>(
                    (
                        static_cast<int>(
                            direction) +
                        1
                    ) %
                    4);
        }

        void cleanUp(lv_event_t *)
        {
            stopGameTimer();

            /*
             * LVGL owns and deletes these objects as children of pageRoot.
             * We only clear our non-owning pointers.
             *
             * boardBuffer is intentionally retained in PSRAM and reused.
             */
            boardCanvas = nullptr;
            scoreLabel = nullptr;
            statusLabel = nullptr;
        }
    }

    void render()
    {
        /*
         * Defensive cleanup in case render() is called unexpectedly while an
         * old Snake timer still exists.
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
                "Snake canvas in PSRAM");

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
            12,
            22);

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
            swirski::ui::swirski_ui::color::ink(),
            LV_PART_MAIN);

        lv_obj_set_style_bg_color(
            boardRoot,
            swirski::ui::swirski_ui::color::ink(),
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
                    "Snake");

        lv_obj_set_pos(
            badge,
            190,
            28);

        scoreLabel =
            lv_label_create(pageRoot);

        lv_obj_set_pos(
            scoreLabel,
            190,
            68);

        statusLabel =
            lv_label_create(pageRoot);

        lv_obj_set_pos(
            statusLabel,
            190,
            120);

        lv_obj_set_style_text_color(
            statusLabel,
            swirski::ui::swirski_ui::color::accent(),
            LV_PART_MAIN);

        startGame();

        gameTimer =
            lv_timer_create(
                updateGame,
                gamePeriodMs,
                nullptr);
    }

    void handleInput(
        swirski::input::input_action action)
    {
        switch (action)
        {
        case swirski::input::input_action::Previous:
            if (!gameOver)
            {
                turnLeft();
            }
            break;

        case swirski::input::input_action::Next:
            if (!gameOver)
            {
                turnRight();
            }
            break;

        case swirski::input::input_action::Confirm:
            if (gameOver)
            {
                startGame();
                resumeGameTimer();
            }
            else
            {
                playing = !playing;

                if (playing)
                {
                    resumeGameTimer();
                }
                else
                {
                    pauseGameTimer();
                }

                updateScreen();
            }
            break;

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