#include "snake_screen.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>

#include "lvgl.h"

#include "screen_manager.hpp"
#include "swirski_ui.hpp"

namespace swirski::screens::snake_screen
{
    namespace
    {
        constexpr int columns = 18;
        constexpr int rows = 14;
        constexpr int cellSize = 8;
        constexpr int boardWidth = columns * cellSize;
        constexpr int boardHeight = rows * cellSize;

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
        lv_obj_t *statusLabel = nullptr;
        lv_timer_t *gameTimer = nullptr;

        bool snakeAt(Point point)
        {
            for (int i = 0; i < snakeLength; ++i)
            {
                if (snake[i].x == point.x && snake[i].y == point.y)
                {
                    return true;
                }
            }

            return false;
        }

        void placeFood()
        {
            do
            {
                food.x = std::rand() % columns;
                food.y = std::rand() % rows;
            } while (snakeAt(food));
        }

        void drawCell(Point point, lv_color_t color)
        {
            const std::uint16_t pixelColor = lv_color_to_u16(color);

            for (int pixelY = 0; pixelY < cellSize - 1; ++pixelY)
            {
                auto *pixelRow =
                    reinterpret_cast<std::uint16_t *>(
                        static_cast<std::uint8_t *>(
                            boardDrawBuffer.data) +
                        (point.y * cellSize + pixelY) *
                            boardDrawBuffer.header.stride);

                for (int pixelX = 0; pixelX < cellSize - 1; ++pixelX)
                {
                    pixelRow[point.x * cellSize + pixelX] = pixelColor;
                }
            }
        }

        void updateScreen()
        {
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
                        swirski::ui::swirski_ui::color::surfaceSoft());
                }
            }

            drawCell(
                food,
                swirski::ui::swirski_ui::color::accentWarm());

            for (int i = snakeLength - 1; i >= 0; --i)
            {
                drawCell(
                    snake[i],
                    i == 0
                        ? swirski::ui::swirski_ui::color::accentBright()
                        : swirski::ui::swirski_ui::color::accent());
            }

            lv_obj_invalidate(boardCanvas);
            lv_label_set_text_fmt(scoreLabel, "SCORE\n%d", score);
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
            placeFood();
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

        void updateGame(lv_timer_t *)
        {
            if (!playing || gameOver)
            {
                return;
            }

            const Point head = nextHead();

            if (
                head.x < 0 || head.x >= columns ||
                head.y < 0 || head.y >= rows ||
                snakeAt(head))
            {
                gameOver = true;
                playing = false;
                updateScreen();
                return;
            }

            const bool ateFood =
                head.x == food.x && head.y == food.y;

            if (ateFood && snakeLength < static_cast<int>(snake.size()))
            {
                snakeLength++;
                score += 10;
            }

            for (int i = snakeLength - 1; i > 0; --i)
            {
                snake[i] = snake[i - 1];
            }

            snake[0] = head;

            if (ateFood)
            {
                placeFood();
            }

            updateScreen();
        }

        void turnLeft()
        {
            direction = static_cast<Direction>(
                (static_cast<int>(direction) + 3) % 4);
        }

        void turnRight()
        {
            direction = static_cast<Direction>(
                (static_cast<int>(direction) + 1) % 4);
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
        lv_obj_set_size(boardRoot, boardWidth + 6, boardHeight + 6);
        lv_obj_align(boardRoot, LV_ALIGN_TOP_LEFT, 12, 22);
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
            swirski::ui::swirski_ui::createBadge(pageRoot, "Snake");
        lv_obj_set_pos(badge, 190, 28);

        scoreLabel = lv_label_create(pageRoot);
        lv_obj_set_pos(scoreLabel, 190, 68);

        statusLabel = lv_label_create(pageRoot);
        lv_obj_set_pos(statusLabel, 190, 120);
        lv_obj_set_style_text_color(
            statusLabel,
            swirski::ui::swirski_ui::color::accent(),
            LV_PART_MAIN);

        startGame();

        gameTimer = lv_timer_create(updateGame, 180, nullptr);
    }

    void handleInput(swirski::input::input_action action)
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
            }
            else
            {
                playing = !playing;
                updateScreen();
            }
            break;

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
