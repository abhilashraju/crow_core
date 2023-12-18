#include "plugin-ifaces/router-plugin.h"

#include "App.hpp"
#include "shared_library.hpp"
int main()
{
    crow::App app;
    bmcweb::PluginDb db(
        "/Users/abhilashraju/work/cpp/crow_core/build/redfish_event/");
    for (auto& plugin : db.getInterFaces<bmcweb::RouterPlugin>())
    {
        BMCWEB_LOG_INFO("Registering plugin ");
        BMCWEB_LOG_INFO("{}", plugin->registerRoutes(app));
    }
    return 0;
}
