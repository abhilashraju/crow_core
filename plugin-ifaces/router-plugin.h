
#pragma once
#include "app.hpp"
#include "bmcweb-plugin.hpp"

#include <string>
namespace bmcweb {
class RouterPlugin : public BmcWebPlugin {
public:
  virtual std::string
  registerRoutes(crow::App &,
                 boost::asio::any_io_executor &ex) = 0; // Pure virtual function
  virtual ~RouterPlugin() = default;                    // Virtual destructor
  static const char *iid() { return "iid_router"; }
};
} // namespace bmcweb
