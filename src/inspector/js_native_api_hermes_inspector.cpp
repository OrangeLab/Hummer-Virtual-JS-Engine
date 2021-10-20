//
// Created by didi on 2021/10/19.
//

#include <hermes/inspector/chrome/Connection.h>
#include <jsinspector/InspectorInterfaces.h>

#include "js_native_api_hermes_inspector.h"

namespace orangelab
{
namespace hermes
{
namespace inspector
{
namespace chrome
{

using ::facebook::hermes::inspector::chrome::Connection;
using ::facebook::react::IInspector;
using ::facebook::react::ILocalConnection;
using ::facebook::react::IRemoteConnection;

namespace
{

class LocalConnection : public ILocalConnection
{
  public:
    LocalConnection(std::shared_ptr<Connection> conn,
                    std::shared_ptr<std::unordered_set<std::string>> inspectedContexts);
    ~LocalConnection();

    void sendMessage(std::string message) override;
    void disconnect() override;

  private:
    std::shared_ptr<Connection> conn_;
    std::shared_ptr<std::unordered_set<std::string>> inspectedContexts_;
};

LocalConnection::LocalConnection(std::shared_ptr<Connection> conn,
                                 std::shared_ptr<std::unordered_set<std::string>> inspectedContexts)
    : conn_(conn), inspectedContexts_(inspectedContexts)
{
    inspectedContexts_->insert(conn->getTitle());
}

LocalConnection::~LocalConnection() = default;

void LocalConnection::sendMessage(std::string str)
{
    conn_->sendMessage(std::move(str));
}

void LocalConnection::disconnect()
{
    inspectedContexts_->erase(conn_->getTitle());
    conn_->disconnect();
}
} // namespace

class ConnectionDemux
{
  public:
    explicit ConnectionDemux(facebook::react::IInspector &inspector);
    ~ConnectionDemux();

    ConnectionDemux(const ConnectionDemux &) = delete;
    ConnectionDemux &operator=(const ConnectionDemux &) = delete;

    int enableDebugging(std::unique_ptr<facebook::hermes::inspector::RuntimeAdapter> adapter, const std::string &title,
                        bool waitForDebugger);

    void disableDebugging(facebook::hermes::HermesRuntime &runtime);

  private:
    int addPage(std::shared_ptr<facebook::hermes::inspector::chrome::Connection> conn);
    void removePage(int pageId);

    facebook::react::IInspector &globalInspector_;

    std::mutex mutex_;
    std::unordered_map<int, std::shared_ptr<facebook::hermes::inspector::chrome::Connection>> conns_;
    std::shared_ptr<std::unordered_set<std::string>> inspectedContexts_;
};

ConnectionDemux::ConnectionDemux(facebook::react::IInspector &inspector)
    : globalInspector_(inspector), inspectedContexts_(std::make_shared<std::unordered_set<std::string>>())
{
}

ConnectionDemux::~ConnectionDemux() = default;

int ConnectionDemux::enableDebugging(std::unique_ptr<facebook::hermes::inspector::RuntimeAdapter> adapter,
                                     const std::string &title, bool waitForDebugger)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // TODO(#22976087): workaround for ComponentScript contexts never being
    // destroyed.
    //
    // After a reload, the old ComponentScript VM instance stays alive. When we
    // register the new CS VM instance, check for any previous CS VM (via strcmp
    // of title) and remove them.
    std::vector<int> pagesToDelete;
    for (auto it = conns_.begin(); it != conns_.end(); ++it)
    {
        if (it->second->getTitle() == title)
        {
            pagesToDelete.push_back(it->first);
        }
    }

    for (auto pageId : pagesToDelete)
    {
        removePage(pageId);
    }
    return addPage(
        std::make_shared<facebook::hermes::inspector::chrome::Connection>(std::move(adapter), title, waitForDebugger));
}

void ConnectionDemux::disableDebugging(facebook::hermes::HermesRuntime &runtime)
{
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto &it : conns_)
    {
        int pageId = it.first;
        auto &conn = it.second;

        if (&(conn->getRuntime()) == &runtime)
        {
            removePage(pageId);

            // must break here. removePage mutates conns_, so range-for iterator is
            // now invalid.
            break;
        }
    }
}

int ConnectionDemux::addPage(std::shared_ptr<facebook::hermes::inspector::chrome::Connection> conn)
{
    auto connectFunc = [conn, this](std::unique_ptr<facebook::react::IRemoteConnection> remoteConn)
        -> std::unique_ptr<facebook::react::ILocalConnection> {
        if (!conn->connect(std::move(remoteConn)))
        {
            return nullptr;
        }

        return std::make_unique<LocalConnection>(conn, inspectedContexts_);
    };

    int pageId = globalInspector_.addPage(conn->getTitle(), "Hermes", std::move(connectFunc));
    conns_[pageId] = std::move(conn);

    return pageId;
}

void ConnectionDemux::removePage(int pageId)
{
    globalInspector_.removePage(pageId);

    auto conn = conns_.at(pageId);
    conn->disconnect();
    conns_.erase(pageId);
}

namespace
{
ConnectionDemux &demux()
{
    static ConnectionDemux instance{facebook::react::getInspectorInstance()};
    return instance;
}
} // namespace

void enableDebugging(
    std::unique_ptr<facebook::hermes::inspector::RuntimeAdapter> adapter,
    const std::string &title,
    bool waitForDebugger)
{
    demux().enableDebugging(std::move(adapter), title, waitForDebugger);
}

void disableDebugging(facebook::hermes::HermesRuntime &runtime)
{
    demux().disableDebugging(runtime);
}

} // namespace chrome
} // namespace inspector
} // namespace hermes
} // namespace orangelab