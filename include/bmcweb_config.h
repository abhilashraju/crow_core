#pragma once

#include <cstddef>
#include <cstdint>

// clang-format off
constexpr const int bmcwebInsecureDisableXssPrevention =
    0;

constexpr const bool bmcwebInsecureEnableQueryParams = 0 == 1;

constexpr const size_t bmcwebHttpReqBodyLimitMb = 30;

constexpr const char* mesonInstallPrefix = "/usr/local";

constexpr const bool bmcwebInsecureEnableHttpPushStyleEventing = 0 == 1;

constexpr const char* bmcwebLoggingLevel = "debug";

constexpr const bool bmcwebEnableHealthPopulate = 0 == 1;

constexpr const bool bmcwebEnableProcMemStatus = 0 == 1;

constexpr const bool bmcwebEnableMultiHost = 0 == 1;

constexpr const bool bmcwebEnableHTTP2 = 0 == 1;
// clang-format on
