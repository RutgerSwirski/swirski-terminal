#pragma once

#include <cstdint>

namespace swirski::ui::notification_toast
{
    void requestMessage(
        const char *title,
        const char *body,
        std::uint32_t durationMs = 4500);
    void requestSyncProgress(int percent);
    void clearSyncProgress();
    bool dismiss();
    void update();
}
