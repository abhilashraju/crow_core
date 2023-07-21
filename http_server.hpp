#pragma once

#include "http_connection.hpp"
#include "logging.hpp"

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
#ifdef BMCWEB_ENABLE_SSL
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <ssl_key_handler.hpp>
#endif

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <future>
#include <memory>
#include <utility>
#include <vector>
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
#include <persistent_data.hpp>
#endif
namespace crow
{

template <typename Handler, typename Adaptor = boost::asio::ip::tcp::socket>
class Server
{
  public:
    Server(Handler* handlerIn,
           std::unique_ptr<boost::asio::ip::tcp::acceptor>&& acceptorIn,
#ifdef BMCWEB_ENABLE_SSL
           std::shared_ptr<boost::asio::ssl::context> adaptorCtxIn,
#endif
           std::shared_ptr<boost::asio::io_context> io) :
        ioService(std::move(io)),
        acceptor(std::move(acceptorIn)),
        signals(*ioService, SIGINT, SIGTERM, SIGHUP), handler(handlerIn)
#ifdef BMCWEB_ENABLE_SSL
        ,
        adaptorCtx(std::move(adaptorCtxIn))
#endif

    {
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
        signals.add(SIGUSR1);
#endif
    }

    Server(Handler* handlerIn, const std::string& bindaddr, uint16_t port,
#ifdef BMCWEB_ENABLE_SSL
           const std::shared_ptr<boost::asio::ssl::context>& adaptorCtxIn,
#endif
           const std::shared_ptr<boost::asio::io_context>& io) :
        Server(handlerIn,
               std::make_unique<boost::asio::ip::tcp::acceptor>(
                   *io, boost::asio::ip::tcp::endpoint(
                            boost::asio::ip::tcp::v4(), port)),
#ifdef BMCWEB_ENABLE_SSL
               adaptorCtxIn,
#endif
               io)
    {}

    Server(Handler* handlerIn, int existingSocket,
#ifdef BMCWEB_ENABLE_SSL
           const std::shared_ptr<boost::asio::ssl::context>& adaptorCtxIn,
#endif
           const std::shared_ptr<boost::asio::io_context>& io) :
        Server(handlerIn,
               std::make_unique<boost::asio::ip::tcp::acceptor>(
                   *io, boost::asio::ip::tcp::v6(), existingSocket),
#ifdef BMCWEB_ENABLE_SSL
               adaptorCtxIn,
#endif
               io)
    {}

    void updateDateStr()
    {
        time_t lastTimeT = time(nullptr);
        tm myTm{};

        gmtime_r(&lastTimeT, &myTm);

        dateStr.resize(100);
        size_t dateStrSz = strftime(&dateStr[0], 99,
                                    "%a, %d %b %Y %H:%M:%S GMT", &myTm);
        dateStr.resize(dateStrSz);
    }

    void run()
    {
        loadCertificate();
        updateDateStr();

        getCachedDateStr = [this]() -> std::string {
            static std::chrono::time_point<std::chrono::steady_clock>
                lastDateUpdate = std::chrono::steady_clock::now();
            if (std::chrono::steady_clock::now() - lastDateUpdate >=
                std::chrono::seconds(10))
            {
                lastDateUpdate = std::chrono::steady_clock::now();
                updateDateStr();
            }
            return this->dateStr;
        };

        BMCWEB_LOG_INFO << "bmcweb server is running, local endpoint "
                        << acceptor->local_endpoint().address().to_string();
        // startAsyncWaitForSignal();
        doAccept();
    }

    void loadCertificate()
    {
#ifdef BMCWEB_ENABLE_SSL
        namespace fs = std::filesystem;
        // Cleanup older certificate file existing in the system
        fs::path oldCert = "/home/root/server.pem";
        if (fs::exists(oldCert))
        {
            fs::remove("/home/root/server.pem");
        }
        fs::path certPath = "/etc/ssl/certs/https/";
        // if path does not exist create the path so that
        // self signed certificate can be created in the
        // path
        if (!fs::exists(certPath))
        {
            fs::create_directories(certPath);
        }
        fs::path certFile = certPath / "server.pem";
        BMCWEB_LOG_INFO << "Building SSL Context file=" << certFile.string();
        std::string sslPemFile(certFile);
        ensuressl::ensureOpensslKeyPresentAndValid(sslPemFile);
        std::shared_ptr<boost::asio::ssl::context> sslContext =
            ensuressl::getSslContext(sslPemFile);
        adaptorCtx = sslContext;
        handler->ssl(std::move(sslContext));
#endif
    }

    void startAsyncWaitForSignal()
    {
        signals.async_wait(
            [this](const boost::system::error_code& ec, int signalNo) {
            if (ec)
            {
                BMCWEB_LOG_INFO << "Error in signal handler" << ec.message();
            }
            else
            {
                if (signalNo == SIGHUP)
                {
                    BMCWEB_LOG_INFO << "Receivied reload signal";
                    loadCertificate();
                    boost::system::error_code ec2;
                    acceptor->cancel(ec2);
                    if (ec2)
                    {
                        BMCWEB_LOG_ERROR
                            << "Error while canceling async operations:"
                            << ec2.message();
                    }
                    this->startAsyncWaitForSignal();
                }
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
                if (signalNo == SIGUSR1)
                {
                    BMCWEB_LOG_CRITICAL
                        << "INFO: Receivied USR1 signal to dump latest session "
                           "data for bmc dump";
                    persistent_data::getConfig().writeCurrentSessionData();
                    this->startAsyncWaitForSignal();
                }
#endif
                else
                {
                    stop();
                }
            }
        });
    }

    void stop()
    {
        ioService->stop();
    }

    void doAccept()
    {
        boost::asio::steady_timer timer(*ioService);
        std::shared_ptr<Connection<Adaptor, Handler>> connection;
#ifdef BMCWEB_ENABLE_SSL
        connection = std::make_shared<Connection<Adaptor, Handler>>(
            handler, std::move(timer), getCachedDateStr,
            Adaptor(*ioService, *adaptorCtx));
#else
        connection = std::make_shared<Connection<Adaptor, Handler>>(
            handler, std::move(timer), getCachedDateStr, Adaptor(*ioService));
#endif
        acceptor->async_accept(
            boost::beast::get_lowest_layer(connection->socket()),
            [this, connection](boost::system::error_code ec) {
            if (!ec)
            {
                boost::asio::post(*this->ioService,
                                  [connection] { connection->start(); });
            }
            doAccept();
            });
        // boost::system::error_code ec{};
        // acceptor->accept(boost::beast::get_lowest_layer(connection->socket()),
        //                  ec);
        // if (ec)
        // {
        //     BMCWEB_LOG_CRITICAL << ec.message();
        //     return;
        // }
        // connection->start();
        // doAccept();
    }

  private:
    std::shared_ptr<boost::asio::io_context> ioService;
    std::function<std::string()> getCachedDateStr;
    std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor;
    boost::asio::signal_set signals;

    std::string dateStr;

    Handler* handler;
#ifdef BMCWEB_ENABLE_SSL
    std::shared_ptr<boost::asio::ssl::context> adaptorCtx;
#endif
};
} // namespace crow