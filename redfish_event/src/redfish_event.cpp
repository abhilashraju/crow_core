#include "router-plugin.h"

#include "event_manager/event_manager.hpp"

#include <app.hpp>
#include <boost/dll/alias.hpp> // for BOOST_DLL_ALIAS
#include <nlohmann/json.hpp>
#include <utils/json_utils.hpp>
#include <iostream>
#include <memory>
namespace redfish {
using namespace bmcweb;
using namespace reactor;
struct EventServiceManager {
  std::vector<reactor::HttpSubscriber> subscribers;
  boost::asio::io_context &ioContext;
  EventServiceManager(boost::asio::io_context &ioc) : ioContext(ioc) {}
  ~EventServiceManager() {}
  EventServiceManager(const EventServiceManager &) = delete;
  EventServiceManager &operator=(const EventServiceManager &) = delete;
  EventServiceManager(EventServiceManager &&) = delete;
  EventServiceManager &operator=(EventServiceManager &&) = delete;

  static EventServiceManager &getInstance(boost::asio::io_context &ioc) {
    static EventServiceManager instance(ioc);
    return instance;
  }
  void sendEvent(const std::string &event) {
    CLIENT_LOG_INFO("sentEvent");
    for (auto &subscriber : subscribers) {

      subscriber.sendEvent(event);
    }
  }
  void addSubscription(const std::string &destUrl) {
    CLIENT_LOG_INFO("addSubscription");
    HttpSubscriber subscriber(ioContext.get_executor(), destUrl);
    ssl::context ctx{ssl::context::tlsv12_client};
    ctx.set_verify_mode(ssl::verify_none);
    subscriber.withPolicy({.maxRetries = 3, .retryDelay = 10})
        .withSslContext(std::move(ctx))
        .withSuccessHandler([](const HttpSubscriber::Request &,
                               const HttpSubscriber::Response &response) {
          CLIENT_LOG_INFO("Response: {}", response.body());
        });
    subscribers.push_back(std::move(subscriber));
  }
};

class MyRouterPlugin : public RouterPlugin {

public:
  MyRouterPlugin() {}
  std::string registerRoutes(crow::App &app) {
    registerSubscriberUrl(app);
    requestRoutesSubmitTestEvent(app);
    registerTestRoutes(app);
    return "Registering aggregator routes";
  }
  void registerTestRoutes([[maybe_unused]]crow::App &app) {
    CLIENT_LOG_INFO("registerTestRoutes");
    // const char* url = "/redfish/v1/Test/";
    
    // app.template route<crow::black_magic::getParameterTag("/redfish/v1/Test/")>("/redfish/v1/Test/");
    BMCWEB_ROUTE(app, "/redfish/v1/Test/")
        // .privileges(redfish::privileges::postEventService)
        .methods(boost::beast::http::verb::post)(
            []([[maybe_unused]] const crow::Request & /*unused*/,
                   [[maybe_unused]] const std::shared_ptr<bmcweb::AsyncResp> &asyncResp) {
              CLIENT_LOG_INFO("called registerTestRoutes");
              asyncResp->res.jsonValue = {{"Test", "Test"}};
            });
  }
  void registerSubscriberUrl(crow::App &app) {
    CLIENT_LOG_INFO("registerSubscriberUrl");
    BMCWEB_ROUTE(app, "/redfish/v2/EventService/Subscriptions/")
        // .privileges(redfish::privileges::postEventDestinationCollection)
        .methods(boost::beast::http::verb::post)(
            [&app](const crow::Request &req,
                   const std::shared_ptr<bmcweb::AsyncResp> &asyncResp) {
              // if (!redfish::setUpRedfishRoute(app, req, asyncResp)) {
              //   return;
              // }
              CLIENT_LOG_INFO("called registerSubscriberUrl with {}",req.body());
              std::string destUrl;
              std::string protocol;
              std::optional<std::string> retryPolicy;
              std::optional<std::string> subscriptionType;
              // REACTOR_LOG_INFO("Subcription Data {}", req.body());
              if (!json_util::readJsonPatch(
                      req, asyncResp->res, "Destination", destUrl,
                      "DeliveryRetryPolicy", retryPolicy, "Protocol", protocol,
                      "SubscriptionType", subscriptionType)) {
                return;
              }

              EventServiceManager::getInstance(app.ioContext())
                  .addSubscription(destUrl);
            });
  }
  auto getCurrentTime() {
    auto currentTime = std::chrono::system_clock::now();
    std::time_t currentTime_t =
        std::chrono::system_clock::to_time_t(currentTime);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&currentTime_t), "%Y-%m-%dT%H:%M:%S");
    return ss.str();
  }
  void sendTestEventLog(crow::App &app) {
    nlohmann::json logEntryArray;
    logEntryArray.push_back({});
    nlohmann::json &logEntryJson = logEntryArray.back();

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

    std::string strMsg =
        msg.dump(2, ' ', true, nlohmann::json::error_handler_t::replace);
    EventServiceManager::getInstance(app.ioContext())
        .sendEvent(std::move(strMsg));
  }
  inline void requestRoutesSubmitTestEvent(crow::App &app) {
    BMCWEB_ROUTE(
        app, "/redfish/v2/EventService/Actions/EventService.SubmitTestEvent/")
        // .privileges(redfish::privileges::postEventService)
        .methods(boost::beast::http::verb::post)(
            [&app, this](const crow::Request & /*unused*/,
                         const std::shared_ptr<bmcweb::AsyncResp> &asyncResp) {
              // if (!redfish::setUpRedfishRoute(app, req, asyncResp)) {
              //   return;
              // }
              sendTestEventLog(app);
              asyncResp->res.result(boost::beast::http::status::no_content);
            });
  }
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
    return MyRouterPlugin::create();
}
} // namespace redfish
