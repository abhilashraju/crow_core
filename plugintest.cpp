#include "plugin-ifaces/router-plugin.h"

#include "http/app.hpp"
#include "shared_library.hpp"

#include <boost/asio.hpp>
std::shared_ptr<boost::asio::io_context> getIoContext()
{
    static std::shared_ptr<boost::asio::io_context> io =
        std::make_shared<boost::asio::io_context>();
    return io;
}
int main()
{
    try
    {
        std::shared_ptr<boost::asio::io_context> io = getIoContext();
        boost::asio::any_io_executor executor = io->get_executor();
        crow::App app(io);
        bmcweb::PluginDb db(
            "/Users/abhilashraju/work/cpp/crow_core/build/redfish_event");
        auto interfaces = db.getInterFaces<bmcweb::RouterPlugin>();
        for (auto& plugin : interfaces)
        {
            BMCWEB_LOG_INFO("Registering plugin ");
            BMCWEB_LOG_INFO("{}", plugin->registerRoutes(app));
        }
        io->run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    return 0;
}
