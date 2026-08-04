#pragma once

#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>

namespace swirski::services::firmware_update
{
    enum class BuildComparison
    {
        AvailableIsNewer,
        InstalledIsSameOrNewer,
        Unknown
    };

    inline std::optional<std::uint32_t> parseBuildNumber(
        std::string_view version)
    {
        if (version.size() < 3 || version.front() != 'b')
        {
            return std::nullopt;
        }

        std::uint32_t buildNumber = 0;
        bool hasDigit = false;

        for (std::size_t index = 1; index < version.size(); ++index)
        {
            const char character = version[index];

            if (character == '-')
            {
                return hasDigit && index + 1 < version.size()
                    ? std::optional<std::uint32_t>{buildNumber}
                    : std::nullopt;
            }

            if (character < '0' || character > '9')
            {
                return std::nullopt;
            }

            const std::uint32_t digit =
                static_cast<std::uint32_t>(character - '0');

            if (
                buildNumber >
                (std::numeric_limits<std::uint32_t>::max() - digit) /
                    10U)
            {
                return std::nullopt;
            }

            buildNumber = buildNumber * 10U + digit;
            hasDigit = true;
        }

        return std::nullopt;
    }

    inline BuildComparison compareBuildVersions(
        std::string_view installedVersion,
        std::string_view availableVersion)
    {
        const auto installedBuild =
            parseBuildNumber(installedVersion);
        const auto availableBuild =
            parseBuildNumber(availableVersion);

        if (installedBuild.has_value() && availableBuild.has_value())
        {
            return *availableBuild > *installedBuild
                ? BuildComparison::AvailableIsNewer
                : BuildComparison::InstalledIsSameOrNewer;
        }

        // A numbered release can migrate devices from the old, unordered
        // git-describe format. A numbered local build must never install an
        // old unordered release, since that could only be a downgrade.
        if (availableBuild.has_value())
        {
            return BuildComparison::AvailableIsNewer;
        }

        if (installedBuild.has_value())
        {
            return BuildComparison::InstalledIsSameOrNewer;
        }

        return BuildComparison::Unknown;
    }
}
