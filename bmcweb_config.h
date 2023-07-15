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
// clang-format on
