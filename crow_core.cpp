#include "app.hpp"
#include "command_line_parser.hpp"
#include "utility.hpp"

#include <boost/asio/io_context.hpp>
using namespace crow;
std::string root;
struct Router {
  void validate() {}
  void handleUpgrade(
      const Request &req, const std::shared_ptr<bmcweb::AsyncResp> &asyncResp,
      boost::beast::ssl_stream<boost::asio::ip::tcp::socket> &&adaptor) {}
  void handle(Request &req,
              const std::shared_ptr<bmcweb::AsyncResp> &asyncResp) {
    auto paths = crow::utility::split(req.target(), '/');

    boost::beast::http::file_body::value_type body;
    boost::beast::error_code ec{};
    auto filetofetch = std::filesystem::path(root).c_str() + paths.back();
    body.open(filetofetch.data(), boost::beast::file_mode::scan, ec);
    if (ec == boost::beast::errc::no_such_file_or_directory) {
      asyncResp->res.result(boost::beast::http::status::not_found);
      asyncResp->res.jsonValue = R"({"hi":"your file not found"})"_json;
      return;
    }

    boost::beast::http::response<boost::beast::http::file_body> res{
        std::piecewise_construct, std::make_tuple(std::move(body)),
        std::make_tuple(boost::beast::http::status::ok, req.version())};
    asyncResp->res.genericResponse.emplace(std::move(res));
  }
};
int main(int argc, const char *argv[]) {
  // Create a server endpoint
  auto [port, r] =
      chai::getArgs(chai::parseCommandline(argc, argv), "-p", "-r");
  if (port.empty()) {
    std::cout << "Invalid arguments\n";
    std::cout << "eg: fileserver -p port\n";
    return 0;
  }
  root = r.data();
  using App = crow::App<Router>;
  auto io = std::make_shared<boost::asio::io_context>();
  // boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
  //     work_guard(io->get_executor());
  App app(io);
  app.port(atoi(port.data()));

  app.run();
  io->run();

  return 0;
}
