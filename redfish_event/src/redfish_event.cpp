#include "router-plugin.h"

#include "http/http_subscriber.hpp"

#include <app.hpp>
#include <boost/dll/alias.hpp> // for BOOST_DLL_ALIAS

#include <iostream>
#include <memory>
namespace bmcweb
{
class MyRouterPlugin : public RouterPlugin
{
  public:
    MyRouterPlugin() {}
    std::string registerRoutes(crow::App& app)
    {
        return "Registering aggregator routes";
    }

    // Factory method
    static std::shared_ptr<MyRouterPlugin> create()
    {
        return std::make_shared<MyRouterPlugin>();
    }

    bool hasInterface(const std::string& interfaceId) override
    {
        return interfaceId == RouterPlugin::iid();
    }
    std::shared_ptr<BmcWebPlugin>
        getInterface(const std::string& interfaceId) override
    {
        if (interfaceId == RouterPlugin::iid())
        {
            return this->shared_from_this();
        }
        return RouterPlugin::getInterface(interfaceId);
    }
};

BMCWEB_SYMBOL_EXPORT std::shared_ptr<MyRouterPlugin> create_object()
{
    return bmcweb::MyRouterPlugin::create();
}
} // namespace bmcweb
