#include "logging.hpp"
#define CLIENT_LOG_DEBUG(message, ...) BMCWEB_LOG_DEBUG(message, ##__VA_ARGS__)
#define CLIENT_LOG_INFO(message, ...) BMCWEB_LOG_INFO(message, ##__VA_ARGS__)
#define CLIENT_LOG_WARNING(message, ...)                                       \
  BMCWEB_LOG_WARNING(message, ##__VA_ARGS__)
#define CLIENT_LOG_ERROR(message, ...) BMCWEB_LOG_ERROR(message, ##__VA_ARGS__)
