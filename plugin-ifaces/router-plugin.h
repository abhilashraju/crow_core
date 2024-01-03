
#pragma once
#include "app.hpp"
#include "bmcweb-plugin.hpp"

#include <string>
namespace bmcweb
{
class RouterPlugin : public BmcWebPlugin
{
  public:
    virtual std::string registerRoutes(crow::App&) = 0; // Pure virtual function
    virtual ~RouterPlugin() = default;                  // Virtual destructor
    static const char* iid()
    {
        return "iid_router";
    }
};
} // namespace bmcweb
