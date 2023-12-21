#include "event_manager/event_manager.hpp"
#include "http/app.hpp"
#include "nlohmann/json.hpp"
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
  //   logEntryJson["EventTimestamp"] = getCurrentTime();
  logEntryJson["Context"] = "TestContext";

  nlohmann::json msg;
  msg["@odata.type"] = "#Event.v1_4_0.Event";
  // msg["Id"] = std::to_string(eventSeqNum);
  msg["Name"] = "Event Log";
  msg["Events"] = logEntryArray;

  std::string strMsg =
      msg.dump(2, ' ', true, nlohmann::json::error_handler_t::replace);
  bmcweb::EventServiceManager::getInstance(app.ioContext().get_executor())
      .sendEvent(std::move(strMsg));
}
std::shared_ptr<boost::asio::io_context> getIoContext() {
  static std::shared_ptr<boost::asio::io_context> io =
      std::make_shared<boost::asio::io_context>();
  return io;
}
int main() {
  std::shared_ptr<boost::asio::io_context> io = getIoContext();
  crow::App app(io);
  sendTestEventLog(app);
  io->run();
  return 0;
}