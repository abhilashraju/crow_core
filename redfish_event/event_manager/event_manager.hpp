#pragma once

#include "http/http_subscriber.hpp"
using namespace reactor;
namespace bmcweb {
struct EventServiceManager {
  std::vector<reactor::HttpSubscriber> subscribers;
  net::any_io_executor &ioContext;
  EventServiceManager(net::any_io_executor &ioc) : ioContext(ioc) {
    addSubscription("https://9.3.84.101:8443/events");
  }
  ~EventServiceManager() {}
  EventServiceManager(const EventServiceManager &) = delete;
  EventServiceManager &operator=(const EventServiceManager &) = delete;
  EventServiceManager(EventServiceManager &&) = delete;
  EventServiceManager &operator=(EventServiceManager &&) = delete;

  static EventServiceManager &getInstance(net::any_io_executor &ioc) {
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

    HttpSubscriber subscriber(ioContext, destUrl);
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
} // namespace bmcweb