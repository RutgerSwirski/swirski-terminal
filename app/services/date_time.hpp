
#include <ctime>

#include <string>

namespace swirski::service::date_time
{

    enum class SyncState
    {
        NotSynced,
        Syncing,
        Synced,
        Failed
    };

    void initialise(time_t initialTimestamp);

    bool update();

    std::string getTimeText();

    std::string getDateText();

    void setFromTimestamp(std::time_t timestamp);

    std::time_t getTimestamp();

}