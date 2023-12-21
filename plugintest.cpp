#include "plugin-ifaces/router-plugin.h"

#include "http/app.hpp"
#include "shared_library.hpp"
std::shared_ptr<boost::asio::io_context> getIoContext() {
  static std::shared_ptr<boost::asio::io_context> io =
      std::make_shared<boost::asio::io_context>();
  return io;
}
int main() {
  std::shared_ptr<boost::asio::io_context> io = getIoContext();
  crow::App app(io);
  bmcweb::PluginDb db("/tmp");
  auto interfaces = db.getInterFaces<bmcweb::RouterPlugin>();
  for (auto &plugin : interfaces) {
    BMCWEB_LOG_INFO("Registering plugin ");
    BMCWEB_LOG_INFO("{}", plugin->registerRoutes(app));
  }
  io->run();
  return 0;
}
