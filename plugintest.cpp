#include "plugin-ifaces/router-plugin.h"

#include "http/app.hpp"
#include "shared_library.hpp"
int main() {
  std::shared_ptr<boost::asio::io_context> io =
      std::make_shared<boost::asio::io_context>();
  crow::App app(io);
  bmcweb::PluginDb db("/tmp/");
  for (auto &plugin : db.getInterFaces<bmcweb::RouterPlugin>()) {
    BMCWEB_LOG_INFO("Registering plugin ");
    BMCWEB_LOG_INFO("{}", plugin->registerRoutes(app));
  }
  io->run();
  return 0;
}
