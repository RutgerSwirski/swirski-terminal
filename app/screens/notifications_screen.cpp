

#include "input.hpp"

#include "screen_manager.hpp"
#include "notifications_screen.hpp"
#include "notification_screen.hpp"
#include "notification_service.hpp"
#include "display_text.hpp"
#include "swirski_ui.hpp"

#include <algorithm>
#include <vector>

#include <iostream>

#include "lvgl.h"

namespace swirski::screens::notifications_screen
{

    namespace
    {

        struct NotificationRow
        {
            lv_obj_t *container;
            lv_obj_t *appNameLabel;
            lv_obj_t *titleLabel;
            lv_obj_t *bodyLabel;
        };

        std::vector<NotificationRow> notificationRows = {};

        lv_obj_t *emptyCard = nullptr;

        std::size_t selectedNotificationIndex = 0;
        std::size_t windowStartIndex = 0;

        int renderedNotificationRevision = -1;

        constexpr std::size_t RENDERED_NOTIFICATION_COUNT = 3;

        void ensureSelectionVisible(
            std::size_t notificationCount)
        {
            if (notificationCount == 0)
            {
                selectedNotificationIndex = 0;
                windowStartIndex = 0;
                return;
            }

            selectedNotificationIndex =
                std::min(
                    selectedNotificationIndex,
                    notificationCount - 1);

            if (selectedNotificationIndex < windowStartIndex)
            {
                windowStartIndex = selectedNotificationIndex;
            }
            else if (
                selectedNotificationIndex >=
                windowStartIndex +
                    RENDERED_NOTIFICATION_COUNT)
            {
                windowStartIndex =
                    selectedNotificationIndex -
                    RENDERED_NOTIFICATION_COUNT +
                    1;
            }

            const std::size_t maximumWindowStart =
                notificationCount >
                        RENDERED_NOTIFICATION_COUNT
                    ? notificationCount -
                          RENDERED_NOTIFICATION_COUNT
                    : 0;

            windowStartIndex =
                std::min(
                    windowStartIndex,
                    maximumWindowStart);
        }

        void updateRowStyle(
            NotificationRow &row,
            bool isSelected)
        {
            const lv_color_t titleColor =
                isSelected
                    ? swirski::ui::swirski_ui::color::surface()
                    : swirski::ui::swirski_ui::color::text();

            const lv_color_t bodyColor =
                isSelected
                    ? swirski::ui::swirski_ui::color::surface()
                    : swirski::ui::swirski_ui::color::textMuted();

            lv_obj_set_style_bg_color(
                row.container,
                isSelected
                    ? swirski::ui::swirski_ui::color::accent()
                    : swirski::ui::swirski_ui::color::surface(),
                LV_PART_MAIN);

            lv_obj_set_style_shadow_color(
                row.container,
                isSelected
                    ? swirski::ui::swirski_ui::color::accentBright()
                    : swirski::ui::swirski_ui::color::accent(),
                LV_PART_MAIN);

            lv_obj_set_style_text_color(
                row.titleLabel,
                titleColor,
                LV_PART_MAIN);

            lv_obj_set_style_text_color(
                row.appNameLabel,
                swirski::ui::swirski_ui::color::ink(),
                LV_PART_MAIN);

            lv_obj_set_style_bg_color(
                row.appNameLabel,
                swirski::ui::swirski_ui::color::accentWarm(),
                LV_PART_MAIN);

            lv_obj_set_style_text_color(
                row.bodyLabel,
                bodyColor,
                LV_PART_MAIN);
        }

    }

    void update()
    {
        const auto &notifications =
            swirski::services::notification_service::
                getNotifications();

        ensureSelectionVisible(notifications.size());

        if (emptyCard != nullptr)
        {
            if (notifications.empty())
            {
                lv_obj_remove_flag(
                    emptyCard,
                    LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                lv_obj_add_flag(
                    emptyCard,
                    LV_OBJ_FLAG_HIDDEN);
            }
        }

        for (
            std::size_t rowIndex = 0;
            rowIndex < notificationRows.size();
            ++rowIndex)
        {
            NotificationRow &row =
                notificationRows[rowIndex];

            const std::size_t notificationIndex =
                windowStartIndex + rowIndex;

            if (notificationIndex >= notifications.size())
            {
                lv_obj_add_flag(
                    row.container,
                    LV_OBJ_FLAG_HIDDEN);
                continue;
            }

            lv_obj_remove_flag(
                row.container,
                LV_OBJ_FLAG_HIDDEN);

            const auto &notification =
                notifications[notificationIndex];

            const char *rowTitle =
                !notification.title.empty()
                    ? notification.title.c_str()
                : !notification.body.empty()
                    ? notification.body.c_str()
                    : "New notification";

            const std::string bodyPreview =
                notification.body.empty()
                    ? "No preview available"
                    : swirski::services::display_text::lastLine(
                          notification.body);

            lv_label_set_text(
                row.appNameLabel,
                notification.appName.empty()
                    ? "APP"
                    : notification.appName.c_str());

            lv_label_set_text(
                row.titleLabel,
                rowTitle);

            lv_label_set_text(
                row.bodyLabel,
                bodyPreview.c_str());

            updateRowStyle(
                row,
                notificationIndex ==
                    selectedNotificationIndex);
        }

        if (
            !notifications.empty() &&
            !notificationRows.empty())
        {
            const std::size_t selectedRowIndex =
                selectedNotificationIndex -
                windowStartIndex;

            lv_obj_scroll_to_view(
                notificationRows[selectedRowIndex]
                    .container,
                LV_ANIM_OFF);
        }
    }

    void render()
    {

        notificationRows.clear();

        std::cout << "Rendering notifications screen" << std::endl;

        lv_obj_t *pageRoot = swirski::screens::manager::createPageRoot();

        lv_obj_t *notificationList = lv_obj_create(pageRoot);

        lv_obj_set_size(notificationList, LV_PCT(100), LV_PCT(95));

        swirski::ui::swirski_ui::stylePanel(notificationList);
        swirski::ui::swirski_ui::styleScrollbar(notificationList);

        lv_obj_align(notificationList, LV_ALIGN_TOP_MID, 0, 5);

        lv_obj_set_flex_flow(notificationList, LV_FLEX_FLOW_COLUMN);

        lv_obj_set_flex_align(
            notificationList,
            LV_FLEX_ALIGN_START,
            LV_FLEX_ALIGN_CENTER,
            LV_FLEX_ALIGN_CENTER);

        lv_obj_set_scroll_dir(notificationList, LV_DIR_VER);

        lv_obj_set_scrollbar_mode(notificationList, LV_SCROLLBAR_MODE_AUTO);

        lv_obj_set_style_pad_all(
            notificationList,
            0,
            LV_PART_MAIN);

        lv_obj_set_style_pad_top(
            notificationList,
            swirski::ui::swirski_ui::space::sm,
            LV_PART_MAIN);

        lv_obj_set_style_pad_bottom(
            notificationList,
            swirski::ui::swirski_ui::space::xl,
            LV_PART_MAIN);

        lv_obj_set_style_pad_row(
            notificationList,
            swirski::ui::swirski_ui::space::md,
            LV_PART_MAIN);

        emptyCard =
            swirski::ui::swirski_ui::createCard(
                notificationList,
                60);

        swirski::ui::swirski_ui::createBadge(
            emptyCard,
            "Notifications");

        swirski::ui::swirski_ui::createLabel(
            emptyCard,
            "No notifications",
            swirski::ui::swirski_ui::TextTone::Muted,
            26,
            18);

        notificationRows.reserve(
            RENDERED_NOTIFICATION_COUNT);

        for (
            std::size_t rowIndex = 0;
            rowIndex < RENDERED_NOTIFICATION_COUNT;
            ++rowIndex)
        {
            lv_obj_t *container =
                swirski::ui::swirski_ui::createCard(
                    notificationList,
                    80);

            swirski::ui::swirski_ui::removeShadow(container);

            // App name

            lv_obj_t *appNameLabel =
                swirski::ui::swirski_ui::createBadge(
                    container,
                    "APP");

            // Title

            lv_obj_t *titleLabel =
                swirski::ui::swirski_ui::createLabel(
                    container,
                    "",
                    swirski::ui::swirski_ui::TextTone::Default,
                    24,
                    18);

            // Body

            lv_obj_t *bodyLabel =
                swirski::ui::swirski_ui::createLabel(
                    container,
                    "",
                    swirski::ui::swirski_ui::TextTone::Muted,
                    44,
                    18);

            notificationRows.push_back({
                container,
                appNameLabel,
                titleLabel,
                bodyLabel,
            });
        }

        renderedNotificationRevision =
            swirski::services::notification_service::
                revision;

        update();
    }

    void refreshIfNeeded()
    {
        if (
            renderedNotificationRevision ==
            swirski::services::notification_service::
                revision)
        {
            return;
        }

        renderedNotificationRevision =
            swirski::services::notification_service::
                revision;

        update();
    }

    void handleInput(swirski::input::input_action action)
    {

        const auto &notifications =
            swirski::services::notification_service::
                getNotifications();

        if (notifications.empty())
        {
            if (
                action ==
                swirski::input::input_action::Back)
            {
                swirski::screens::manager::showScreen(
                    swirski::screens::manager::Screen::Home);
            }

            return;
        }

        switch (action)
        {

        case swirski::input::input_action::Previous:
            std::cout << "Previous" << std::endl;
            if (selectedNotificationIndex == 0)
            {
                selectedNotificationIndex =
                    notifications.size() - 1;
            }
            else
                selectedNotificationIndex--;

            update();

            break;
        case swirski::input::input_action::Next:
            std::cout << "Next" << std::endl;
            if (
                selectedNotificationIndex ==
                notifications.size() - 1)
            {
                selectedNotificationIndex = 0;
            }
            else
                selectedNotificationIndex++;

            update();

            break;
        case swirski::input::input_action::Confirm:
            std::cout << "Confirm" << std::endl;

            swirski::screens::manager::showNotificationScreen(
                notifications[selectedNotificationIndex].id);

            break;
        case swirski::input::input_action::Back:
            std::cout << "Back" << std::endl;

            swirski::screens::manager::showScreen(swirski::screens::manager::Screen::Home);
            break;
        case swirski::input::input_action::Home:
            break;
        }
    }
}
