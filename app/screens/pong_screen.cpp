#include "pong_screen.hpp"

#include "lvgl.h"

#include "screen_manager.hpp"
#include "swirski_ui.hpp"

namespace swirski::screens::pong_screen
{
    namespace
    {
        constexpr int courtWidth = 300;
        constexpr int courtHeight = 170;
        constexpr int paddleWidth = 5;
        constexpr int paddleHeight = 32;
        constexpr int ballSize = 6;

        enum class Difficulty
        {
            Easy,
            Normal,
            Hard,
            Insane
        };

        lv_obj_t *playerPaddle = nullptr;
        lv_obj_t *computerPaddle = nullptr;
        lv_obj_t *ball = nullptr;
        lv_obj_t *scoreLabel = nullptr;
        lv_obj_t *statusLabel = nullptr;
        lv_timer_t *gameTimer = nullptr;

        int playerY = 69;
        int computerY = 69;
        int ballX = 147;
        int ballY = 82;
        int ballXSpeed = 2;
        int ballYSpeed = 2;
        int playerScore = 0;
        int computerScore = 0;
        int computerMoveFrame = 0;
        bool playing = false;
        bool gameOver = false;
        bool hasStarted = false;
        Difficulty difficulty = Difficulty::Normal;

        const char *difficultyName()
        {
            switch (difficulty)
            {
            case Difficulty::Easy:
                return "EASY";
            case Difficulty::Normal:
                return "NORMAL";
            case Difficulty::Hard:
                return "HARD";
            case Difficulty::Insane:
                return "INSANE";
            }

            return "NORMAL";
        }

        int computerMoveInterval()
        {
            switch (difficulty)
            {
            case Difficulty::Easy:
                return 3;
            case Difficulty::Normal:
                return 2;
            case Difficulty::Hard:
            case Difficulty::Insane:
                return 1;
            }

            return 2;
        }

        int computerMoveSpeed()
        {
            return difficulty == Difficulty::Insane
                       ? 3
                       : 1;
        }

        void showStartMessage()
        {
            lv_label_set_text_fmt(
                statusLabel,
                "%s - PRESS TO PLAY",
                difficultyName());
        }

        void selectPreviousDifficulty()
        {
            switch (difficulty)
            {
            case Difficulty::Easy:
                difficulty = Difficulty::Insane;
                break;
            case Difficulty::Normal:
                difficulty = Difficulty::Easy;
                break;
            case Difficulty::Hard:
                difficulty = Difficulty::Normal;
                break;
            case Difficulty::Insane:
                difficulty = Difficulty::Hard;
                break;
            }

            showStartMessage();
        }

        void selectNextDifficulty()
        {
            switch (difficulty)
            {
            case Difficulty::Easy:
                difficulty = Difficulty::Normal;
                break;
            case Difficulty::Normal:
                difficulty = Difficulty::Hard;
                break;
            case Difficulty::Hard:
                difficulty = Difficulty::Insane;
                break;
            case Difficulty::Insane:
                difficulty = Difficulty::Easy;
                break;
            }

            showStartMessage();
        }

        int clampPaddleY(int y)
        {
            if (y < 3)
            {
                return 3;
            }

            const int maximumY =
                courtHeight - paddleHeight - 3;

            return y > maximumY
                       ? maximumY
                       : y;
        }

        void updateObjects()
        {
            lv_obj_set_pos(playerPaddle, 8, playerY);
            lv_obj_set_pos(computerPaddle, 287, computerY);
            lv_obj_set_pos(ball, ballX, ballY);

            lv_label_set_text_fmt(
                scoreLabel,
                "%d : %d",
                playerScore,
                computerScore);
        }

        void resetBall(int direction)
        {
            const int speed =
                difficulty == Difficulty::Insane
                    ? 3
                    : 2;

            ballX = 147;
            ballY = 82;
            ballXSpeed = speed * direction;
            ballYSpeed = ballYSpeed < 0 ? -speed : speed;
        }

        bool ballTouchesPaddle(
            int paddleX,
            int paddleY)
        {
            return ballX + ballSize >= paddleX &&
                   ballX <= paddleX + paddleWidth &&
                   ballY + ballSize >= paddleY &&
                   ballY <= paddleY + paddleHeight;
        }

        void finishRound()
        {
            if (playerScore < 5 && computerScore < 5)
            {
                return;
            }

            playing = false;
            gameOver = true;

            lv_label_set_text(
                statusLabel,
                playerScore == 5
                    ? "YOU WIN - PRESS TO RESTART"
                    : "CPU WINS - PRESS TO RESTART");

            lv_obj_clear_flag(
                statusLabel,
                LV_OBJ_FLAG_HIDDEN);
        }

        void updateGame(lv_timer_t *)
        {
            if (!playing)
            {
                return;
            }

            computerMoveFrame++;

            if (computerMoveFrame >= computerMoveInterval())
            {
                computerMoveFrame = 0;

                const int computerTarget =
                    ballY - (paddleHeight - ballSize) / 2;

                if (computerY < computerTarget)
                {
                    computerY += computerMoveSpeed();
                }
                else if (computerY > computerTarget)
                {
                    computerY -= computerMoveSpeed();
                }
            }

            computerY = clampPaddleY(computerY);
            ballX += ballXSpeed;
            ballY += ballYSpeed;

            if (ballY <= 2 || ballY + ballSize >= courtHeight - 2)
            {
                ballYSpeed = -ballYSpeed;
            }

            if (
                ballXSpeed < 0 &&
                ballTouchesPaddle(8, playerY))
            {
                ballX = 8 + paddleWidth;
                ballXSpeed = -ballXSpeed;
            }

            if (
                ballXSpeed > 0 &&
                ballTouchesPaddle(287, computerY))
            {
                ballX = 287 - ballSize;
                ballXSpeed = -ballXSpeed;
            }

            if (ballX < 0)
            {
                computerScore++;
                resetBall(1);
                finishRound();
            }
            else if (ballX > courtWidth)
            {
                playerScore++;
                resetBall(-1);
                finishRound();
            }

            updateObjects();
        }

        lv_obj_t *createGameObject(
            lv_obj_t *parent,
            int width,
            int height,
            lv_color_t color)
        {
            lv_obj_t *object = lv_obj_create(parent);

            lv_obj_remove_style_all(object);
            lv_obj_set_size(object, width, height);
            lv_obj_set_style_bg_color(object, color, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(object, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLLABLE);

            return object;
        }

        void cleanUp(lv_event_t *)
        {
            if (gameTimer != nullptr)
            {
                lv_timer_delete(gameTimer);
                gameTimer = nullptr;
            }

            playerPaddle = nullptr;
            computerPaddle = nullptr;
            ball = nullptr;
            scoreLabel = nullptr;
            statusLabel = nullptr;
        }
    }

    void render()
    {
        playerY = 69;
        computerY = 69;
        playerScore = 0;
        computerScore = 0;
        playing = false;
        gameOver = false;
        hasStarted = false;
        computerMoveFrame = 0;
        difficulty = Difficulty::Normal;
        resetBall(1);

        lv_obj_t *pageRoot =
            swirski::screens::manager::createPageRoot();

        lv_obj_add_event_cb(
            pageRoot,
            cleanUp,
            LV_EVENT_DELETE,
            nullptr);

        lv_obj_t *court = lv_obj_create(pageRoot);

        lv_obj_set_size(court, courtWidth, courtHeight);
        lv_obj_align(court, LV_ALIGN_TOP_MID, 0, 10);
        lv_obj_set_style_bg_color(
            court,
            swirski::ui::swirski_ui::color::surface(),
            LV_PART_MAIN);
        lv_obj_set_style_bg_opa(court, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(court, 3, LV_PART_MAIN);
        lv_obj_set_style_border_color(
            court,
            swirski::ui::swirski_ui::color::ink(),
            LV_PART_MAIN);
        lv_obj_set_style_radius(court, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(court, 0, LV_PART_MAIN);
        lv_obj_clear_flag(court, LV_OBJ_FLAG_SCROLLABLE);

        playerPaddle = createGameObject(
            court,
            paddleWidth,
            paddleHeight,
            swirski::ui::swirski_ui::color::accent());

        computerPaddle = createGameObject(
            court,
            paddleWidth,
            paddleHeight,
            swirski::ui::swirski_ui::color::accentBright());

        ball = createGameObject(
            court,
            ballSize,
            ballSize,
            swirski::ui::swirski_ui::color::accentWarm());

        scoreLabel = lv_label_create(court);
        lv_obj_align(scoreLabel, LV_ALIGN_TOP_MID, 0, 4);

        statusLabel = lv_label_create(court);
        showStartMessage();
        lv_obj_set_style_bg_color(
            statusLabel,
            swirski::ui::swirski_ui::color::accentWarm(),
            LV_PART_MAIN);
        lv_obj_set_style_bg_opa(statusLabel, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_pad_all(statusLabel, 4, LV_PART_MAIN);
        lv_obj_align(statusLabel, LV_ALIGN_CENTER, 0, 0);

        updateObjects();

        gameTimer = lv_timer_create(
            updateGame,
            25,
            nullptr);
    }

    void handleInput(
        swirski::input::input_action action)
    {
        switch (action)
        {
        case swirski::input::input_action::Previous:
            if (!hasStarted)
            {
                selectPreviousDifficulty();
                break;
            }

            playerY = clampPaddleY(playerY - 10);
            updateObjects();
            break;

        case swirski::input::input_action::Next:
            if (!hasStarted)
            {
                selectNextDifficulty();
                break;
            }

            playerY = clampPaddleY(playerY + 10);
            updateObjects();
            break;

        case swirski::input::input_action::Confirm:
            if (!hasStarted)
            {
                resetBall(1);
            }

            if (gameOver)
            {
                playerScore = 0;
                computerScore = 0;
                gameOver = false;
                resetBall(1);
            }

            hasStarted = true;

            playing = !playing;

            if (playing)
            {
                lv_obj_add_flag(
                    statusLabel,
                    LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                lv_label_set_text(statusLabel, "PAUSED");
                lv_obj_clear_flag(
                    statusLabel,
                    LV_OBJ_FLAG_HIDDEN);
            }

            updateObjects();
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
