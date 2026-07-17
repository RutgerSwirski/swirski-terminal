

#pragma once

#include <string>

struct Notification
{
    std::string id;
    std::string packageName;
    std::string appName;
    std::string title;
    std::string body;
    std::int64_t postedAt = 0;
};