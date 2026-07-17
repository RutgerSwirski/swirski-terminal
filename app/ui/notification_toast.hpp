#pragma once

namespace swirski::ui::notification_toast
{
    void requestSyncProgress(int percent);
    void clearSyncProgress();
    void update();
}
