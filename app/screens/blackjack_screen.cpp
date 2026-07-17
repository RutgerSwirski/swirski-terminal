#include "blackjack_screen.hpp"

#include <array>
#include <cstdlib>
#include <string>

#include "lvgl.h"

#include "screen_manager.hpp"
#include "swirski_ui.hpp"

namespace swirski::screens::blackjack_screen
{
    namespace
    {
        constexpr std::size_t maximumCards = 10;

        std::array<int, maximumCards> playerCards{};
        std::array<int, maximumCards> dealerCards{};
        std::size_t playerCardCount = 0;
        std::size_t dealerCardCount = 0;

        lv_obj_t *dealerLabel = nullptr;
        lv_obj_t *playerLabel = nullptr;
        lv_obj_t *resultLabel = nullptr;
        std::array<lv_obj_t *, 2> actionLabels{};

        int selectedAction = 0;
        bool roundOver = false;
        bool randomSeeded = false;
        std::string resultText;

        int drawCard()
        {
            return 1 + std::rand() % 13;
        }

        int handTotal(
            const std::array<int, maximumCards> &cards,
            std::size_t cardCount)
        {
            int total = 0;
            int aceCount = 0;

            for (std::size_t i = 0; i < cardCount; ++i)
            {
                if (cards[i] == 1)
                {
                    total += 11;
                    aceCount++;
                }
                else
                {
                    total += cards[i] > 10
                                 ? 10
                                 : cards[i];
                }
            }

            while (total > 21 && aceCount > 0)
            {
                total -= 10;
                aceCount--;
            }

            return total;
        }

        std::string cardName(int card)
        {
            switch (card)
            {
            case 1:
                return "A";
            case 11:
                return "J";
            case 12:
                return "Q";
            case 13:
                return "K";
            default:
                return std::to_string(card);
            }
        }

        std::string handText(
            const std::array<int, maximumCards> &cards,
            std::size_t cardCount,
            bool hideSecondCard)
        {
            std::string text;

            for (std::size_t i = 0; i < cardCount; ++i)
            {
                if (!text.empty())
                {
                    text += ", ";
                }

                text += hideSecondCard && i == 1
                            ? "?"
                            : cardName(cards[i]);
            }

            return text;
        }

        void updateActionSelection()
        {
            const std::array<const char *, 2> actions{
                "Hit",
                "Stand"};

            for (std::size_t i = 0; i < actions.size(); ++i)
            {
                lv_label_set_text(actionLabels[i], actions[i]);

                swirski::ui::swirski_ui::styleMenuItem(
                    actionLabels[i],
                    static_cast<int>(i) == selectedAction);

                lv_obj_set_width(actionLabels[i], 90);
                lv_obj_set_style_text_align(
                    actionLabels[i],
                    LV_TEXT_ALIGN_CENTER,
                    LV_PART_MAIN);
            }
        }

        void updateScreen()
        {
            const std::string dealerText =
                "Dealer: " +
                handText(
                    dealerCards,
                    dealerCardCount,
                    !roundOver);

            const std::string playerText =
                "You: " +
                handText(
                    playerCards,
                    playerCardCount,
                    false) +
                "  (" +
                std::to_string(
                    handTotal(playerCards, playerCardCount)) +
                ")";

            lv_label_set_text(dealerLabel, dealerText.c_str());
            lv_label_set_text(playerLabel, playerText.c_str());
            lv_label_set_text(resultLabel, resultText.c_str());

            if (roundOver)
            {
                lv_obj_add_flag(
                    actionLabels[0],
                    LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(
                    actionLabels[1],
                    LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                lv_obj_clear_flag(
                    actionLabels[0],
                    LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(
                    actionLabels[1],
                    LV_OBJ_FLAG_HIDDEN);
                updateActionSelection();
            }
        }

        void finishRound(const char *message)
        {
            roundOver = true;
            resultText = message;
            resultText += " - PRESS TO DEAL";
            updateScreen();
        }

        void stand()
        {
            while (
                handTotal(dealerCards, dealerCardCount) < 17 &&
                dealerCardCount < maximumCards)
            {
                dealerCards[dealerCardCount++] = drawCard();
            }

            const int playerTotal =
                handTotal(playerCards, playerCardCount);

            const int dealerTotal =
                handTotal(dealerCards, dealerCardCount);

            if (dealerTotal > 21 || playerTotal > dealerTotal)
            {
                finishRound("YOU WIN");
            }
            else if (playerTotal < dealerTotal)
            {
                finishRound("DEALER WINS");
            }
            else
            {
                finishRound("PUSH");
            }
        }

        void hit()
        {
            if (playerCardCount >= maximumCards)
            {
                return;
            }

            playerCards[playerCardCount++] = drawCard();

            const int total =
                handTotal(playerCards, playerCardCount);

            if (total > 21)
            {
                finishRound("BUST - DEALER WINS");
            }
            else if (total == 21)
            {
                stand();
            }
            else
            {
                updateScreen();
            }
        }

        void deal()
        {
            playerCardCount = 0;
            dealerCardCount = 0;
            selectedAction = 0;
            roundOver = false;
            resultText = "CHOOSE YOUR MOVE";

            playerCards[playerCardCount++] = drawCard();
            dealerCards[dealerCardCount++] = drawCard();
            playerCards[playerCardCount++] = drawCard();
            dealerCards[dealerCardCount++] = drawCard();

            const int playerTotal =
                handTotal(playerCards, playerCardCount);

            const int dealerTotal =
                handTotal(dealerCards, dealerCardCount);

            if (playerTotal == 21 && dealerTotal == 21)
            {
                finishRound("BLACKJACK PUSH");
            }
            else if (playerTotal == 21)
            {
                finishRound("BLACKJACK - YOU WIN");
            }
            else if (dealerTotal == 21)
            {
                finishRound("DEALER BLACKJACK");
            }
            else
            {
                updateScreen();
            }
        }
    }

    void render()
    {
        if (!randomSeeded)
        {
            std::srand(
                static_cast<unsigned int>(
                    lv_tick_get()));
            randomSeeded = true;
        }

        lv_obj_t *pageRoot =
            swirski::screens::manager::createPageRoot();

        lv_obj_t *container =
            swirski::ui::swirski_ui::createCard(
                pageRoot,
                175);

        lv_obj_set_width(container, 290);
        lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 10);

        swirski::ui::swirski_ui::createBadge(
            container,
            "Blackjack");

        dealerLabel =
            swirski::ui::swirski_ui::createLabel(
                container,
                "",
                swirski::ui::swirski_ui::TextTone::Muted,
                30,
                22);

        playerLabel =
            swirski::ui::swirski_ui::createLabel(
                container,
                "",
                swirski::ui::swirski_ui::TextTone::Default,
                62,
                22);

        resultLabel =
            swirski::ui::swirski_ui::createLabel(
                container,
                "",
                swirski::ui::swirski_ui::TextTone::Accent,
                96,
                22);

        lv_obj_t *actionList = lv_obj_create(container);

        lv_obj_remove_style_all(actionList);
        lv_obj_set_pos(actionList, 18, 128);
        lv_obj_set_size(actionList, 200, 26);
        lv_obj_set_layout(actionList, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(actionList, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_column(
            actionList,
            swirski::ui::swirski_ui::space::md,
            LV_PART_MAIN);
        lv_obj_clear_flag(actionList, LV_OBJ_FLAG_SCROLLABLE);

        for (lv_obj_t *&actionLabel : actionLabels)
        {
            actionLabel = lv_label_create(actionList);
        }

        deal();
    }

    void handleInput(
        swirski::input::input_action action)
    {
        switch (action)
        {
        case swirski::input::input_action::Previous:
        case swirski::input::input_action::Next:
            if (!roundOver)
            {
                selectedAction = selectedAction == 0 ? 1 : 0;
                updateActionSelection();
            }
            break;

        case swirski::input::input_action::Confirm:
            if (roundOver)
            {
                deal();
            }
            else if (selectedAction == 0)
            {
                hit();
            }
            else
            {
                stand();
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
