#pragma once
#include "plugin-defs.hpp"

#include <memory>
#include <string>
namespace bmcweb
{
class BmcWebPlugin : public std::enable_shared_from_this<BmcWebPlugin>
{
  public:
    virtual ~BmcWebPlugin() = 0; // Virtual destructor
    virtual bool hasInterface(const std::string& interfaceId) = 0;
    virtual std::shared_ptr<BmcWebPlugin>
        getInterface(const std::string& interfaceId) = 0;
    static const char* iid()
    {
        return "iid_bmcweb";
    }
    template <typename Iface>
    static inline std::shared_ptr<Iface> getInterface(BmcWebPlugin* baseIface)
    {
        if (baseIface->hasInterface(Iface::iid()))
        {
            return std::static_pointer_cast<Iface>(
                baseIface->getInterface(Iface::iid()));
        }
        return std::shared_ptr<Iface>();
    }
};
inline BmcWebPlugin::~BmcWebPlugin() = default;
inline std::shared_ptr<BmcWebPlugin>
    BmcWebPlugin::getInterface(const std::string& interfaceId)
{
    if (interfaceId == iid())
    {
        return this->shared_from_this();
    }
    return {};
}
inline bool BmcWebPlugin::hasInterface(const std::string& interfaceId)
{
    return interfaceId == iid();
}
} // namespace bmcweb
