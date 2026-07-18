

#include "input.hpp"

#include "screen_manager.hpp"
#include "notifications_screen.hpp"
#include "notification_screen.hpp"
#include "notification_service.hpp"
#include "swirski_ui.hpp"

#include <vector>

#include <iostream>

#include <string>

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

            std::string id;
            std::string appName;
            std::string title;
            std::string body;
        };

        std::vector<NotificationRow> notificationRows = {};

        std::size_t selectedNotificationIndex = 0;

        int renderedNotificationRevision = -1;

        constexpr std::size_t MAX_RENDERED_NOTIFICATIONS = 20;

    }

    void update()
    {
        for (
            std::size_t i = 0;
            i < notificationRows.size();
            ++i)
        {
            NotificationRow &row =
                notificationRows[i];

            const bool isSelected =
                i == selectedNotificationIndex;

            lv_label_set_text(
                row.titleLabel,
                row.title.c_str());

            const lv_color_t titleColor =
                isSelected
                    ? swirski::ui::swirski_ui::color::surface()
                    : swirski::ui::swirski_ui::color::text();

            const lv_color_t appNameColor =
                swirski::ui::swirski_ui::color::ink();

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
                appNameColor,
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

        if (!notificationRows.empty())
        {
            lv_obj_scroll_to_view(
                notificationRows[selectedNotificationIndex]
                    .container,
                LV_ANIM_OFF);
        }
    }

    void render()
    {

        notificationRows.clear();

        const auto &notifications =
            swirski::services::notification_service::getNotifications();

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

        if (notifications.empty())
        {
            lv_obj_t *emptyCard =
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
        }

        for (const auto &notification : notifications)
        {
            if (
                notificationRows.size() >=
                MAX_RENDERED_NOTIFICATIONS)
            {
                break;
            }

            const std::string rowTitle =
                !notification.title.empty()
                    ? notification.title
                : !notification.body.empty()
                    ? notification.body
                    : "New notification";

            const std::string bodyPreview =
                notification.body.empty()
                    ? "No preview available"
                    : notification.body;

            lv_obj_t *container =
                swirski::ui::swirski_ui::createCard(
                    notificationList,
                    80);

            swirski::ui::swirski_ui::removeShadow(container);

            // App name

            lv_obj_t *appNameLabel =
                swirski::ui::swirski_ui::createBadge(
                    container,
                    notification.appName.empty()
                        ? "APP"
                        : notification.appName.c_str());

            // Title

            lv_obj_t *titleLabel =
                swirski::ui::swirski_ui::createLabel(
                    container,
                    rowTitle.c_str(),
                    swirski::ui::swirski_ui::TextTone::Default,
                    24,
                    18);

            // Body

            lv_obj_t *bodyLabel =
                swirski::ui::swirski_ui::createLabel(
                    container,
                    bodyPreview.c_str(),
                    swirski::ui::swirski_ui::TextTone::Muted,
                    44,
                    18);

            notificationRows.push_back({
                container,
                appNameLabel,
                titleLabel,
                bodyLabel,
                notification.id,
                notification.appName,
                rowTitle,
                bodyPreview,
            });
        }

        if (notificationRows.empty())
        {
            selectedNotificationIndex = 0;
        }
        else if (selectedNotificationIndex >= notificationRows.size())
        {
            selectedNotificationIndex =
                notificationRows.size() - 1;
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

        render();
    }

    void handleInput(swirski::input::input_action action)
    {

        if (notificationRows.empty())
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
                selectedNotificationIndex = notificationRows.size() - 1;
            }
            else
                selectedNotificationIndex--;

            update();

            break;
        case swirski::input::input_action::Next:
            std::cout << "Next" << std::endl;
            if (selectedNotificationIndex == notificationRows.size() - 1)
            {
                selectedNotificationIndex = 0;
            }
            else
                selectedNotificationIndex++;

            update();

            break;
        case swirski::input::input_action::Confirm:
            std::cout << "Confirm" << std::endl;

            swirski::screens::manager::showNotificationScreen(notificationRows[selectedNotificationIndex].id);

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
