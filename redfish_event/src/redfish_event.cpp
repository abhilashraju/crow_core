#include "router-plugin.h"

#include "http/http_subscriber.hpp"

#include <app.hpp>
#include <boost/dll/alias.hpp> // for BOOST_DLL_ALIAS
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
namespace bmcweb {
using namespace reactor;
struct EventServiceManager {
  std::vector<reactor::HttpSubscriber> subscribers;
  boost::asio::io_context &ioContext;
  EventServiceManager(boost::asio::io_context &ioc) : ioContext(ioc) {
    addSubscription("https://9.3.84.101:8443/events");
  }
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
    sendTestEventLog(app);
    return "MyRouterPlugin";
  }

  // Factory method
  static std::shared_ptr<MyRouterPlugin> create() {
    return std::make_shared<MyRouterPlugin>();
  }

  bool hasInterface(const std::string &interfaceId) override {
    return interfaceId == RouterPlugin::iid();
  }
  std::shared_ptr<BmcWebPlugin>
  getInterface(const std::string &interfaceId) override {
    if (interfaceId == RouterPlugin::iid()) {
      return this->shared_from_this();
    }
    return RouterPlugin::getInterface(interfaceId);
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
  std::string getCurrentTime() {
    auto currentTime = std::chrono::system_clock::now();
    std::time_t currentTime_t =
        std::chrono::system_clock::to_time_t(currentTime);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&currentTime_t), "%Y-%m-%dT%H:%M:%S");
    return ss.str();
  }
};

BMCWEB_SYMBOL_EXPORT std::shared_ptr<MyRouterPlugin> create_object() {
  return MyRouterPlugin::create();
}
} // namespace bmcweb
