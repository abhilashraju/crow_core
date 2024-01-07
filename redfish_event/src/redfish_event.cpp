#include "router-plugin.h"

#include "event_manager/event_manager.hpp"

#include <app.hpp>
#include <boost/dll/alias.hpp> // for BOOST_DLL_ALIAS
#include <nlohmann/json.hpp>

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
        BMCWEB_ROUTE(app, "/redfish/v1/MyRouterPlugin/")
            // .privileges(redfish::privileges::getMyRouterPlugin)
            .methods(boost::beast::http::verb::post)(
                [this](const crow::Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                    asyncResp->res.jsonValue = {{"@odata.type",
                                                 "#MyRouterPlugin.v1_0_0.MyRouterPlugin"},
                                                {"@odata.id",
                                                 "/redfish/v1/MyRouterPlugin"},
                                                {"@odata.context",
                                                 "/redfish/v1/$metadata#MyRouterPlugin.MyRouterPlugin"},
                                                {"Id", "MyRouterPlugin"},
                                                {"Name", "MyRouterPlugin"},
                                                {"Description", "MyRouterPlugin"},
                                                {"MyRouterPluginProperty", "MyRouterPluginProperty"}};
                });
        // sendTestEventLog(app);
        return "MyRouterPlugin";
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
    void sendTestEventLog(crow::App& app)
    {
        nlohmann::json logEntryArray;
        logEntryArray.push_back({});
        nlohmann::json& logEntryJson = logEntryArray.back();

        logEntryJson["EventId"] = "TestID";
        logEntryJson["EventType"] = "Event";
        logEntryJson["Severity"] = "OK";
        logEntryJson["Message"] = "Generated test event";
        logEntryJson["MessageId"] = "OpenBMC.0.2.TestEventLog";
        logEntryJson["MessageArgs"] = nlohmann::json::array();
        logEntryJson["EventTimestamp"] = getCurrentTime();
        logEntryJson["Context"] = "TestContext";

        nlohmann::json msg;
        msg["@odata.type"] = "#Event.v1_4_0.Event";
        // msg["Id"] = std::to_string(eventSeqNum);
        msg["Name"] = "Event Log";
        msg["Events"] = logEntryArray;

        std::string strMsg = msg.dump(2, ' ', true,
                                      nlohmann::json::error_handler_t::replace);

        EventServiceManager::getInstance(app.ioContext().get_executor()).sendEvent(std::move(strMsg));
    }
    std::string getCurrentTime()
    {
        auto currentTime = std::chrono::system_clock::now();
        std::time_t currentTime_t =
            std::chrono::system_clock::to_time_t(currentTime);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&currentTime_t),
                            "%Y-%m-%dT%H:%M:%S");
        return ss.str();
    }
};

BMCWEB_SYMBOL_EXPORT std::shared_ptr<MyRouterPlugin> create_object()
{
    return MyRouterPlugin::create();
}
} // namespace bmcweb
