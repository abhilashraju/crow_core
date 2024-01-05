#include "plugin-ifaces/router-plugin.h"

#include "http/app.hpp"
#include "shared_library.hpp"

#include <boost/asio.hpp>
std::shared_ptr<boost::asio::io_context> getIoContext() {
  static std::shared_ptr<boost::asio::io_context> io =
      std::make_shared<boost::asio::io_context>();
  return io;
}
int main() {
  try {
    std::shared_ptr<boost::asio::io_context> io = getIoContext();
    boost::asio::any_io_executor executor = io->get_executor();
    crow::App app(io);
    bmcweb::PluginDb db("/tmp");
    auto interfaces = db.getInterFaces<bmcweb::RouterPlugin>();
    for (auto &plugin : interfaces) {
      BMCWEB_LOG_INFO("Registering plugin ");
      BMCWEB_LOG_INFO("{}", plugin->registerRoutes(app));
    }
    io->post([&app]() { 
      boost::beast::http::request<boost::beast::http::string_body> req;
      req.method(boost::beast::http::verb::post);
      req.target("/redfish/v1/MyRouterPlugin/");
      std::error_code ec;
      crow::Request cReq(req,ec);
      auto resp =std::make_shared<bmcweb::AsyncResp>();
      app.handle(cReq,resp);
      BMCWEB_LOG_INFO("{}", resp->res.jsonValue.dump(2));
     });
    io->run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
  }

  return 0;
}
