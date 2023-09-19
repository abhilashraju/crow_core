
#pragma once
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>

#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

namespace beast = boost::beast;   // from <boost/beast.hpp>
namespace http = beast::http;     // from <boost/beast/http.hpp>
namespace net = boost::asio;      // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl; // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------

// Report a failure
void fail(beast::error_code ec, const char* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}
template <typename Stream>
struct SyncStream : public std::enable_shared_from_this<SyncStream<Stream>>
{
  private:
    Stream mStream;

  protected:
    SyncStream(Stream&& strm) : mStream(std::move(strm)) {}
    auto& lowestLayer()
    {
        return beast::get_lowest_layer(mStream);
    }
    Stream& stream()
    {
        return mStream;
    }
    virtual void
        on_resolve(std::function<void(beast::error_code)> connectionHandler,
                   tcp::resolver::results_type results) = 0;

  public:
    void resolve(tcp::resolver& resolver, const char* host, const char* port,
                 std::function<void(beast::error_code)> handler)
    {
        beast::error_code ec{};
        tcp::resolver::results_type result = resolver.resolve(host, port, ec);
        if (ec)
        {
            return fail(ec, "resolve");
        }
        on_resolve(std::move(handler), result);
    }
    virtual void shutDown() = 0;
    template <typename Body>
    void write(
        http::request<Body>& req,
        std::function<void(beast::error_code, std::size_t)> onWriteHandler)
    {
        beast::error_code ec{};
        // Send the HTTP request to the remote host
        auto bytes_transferred = http::write(mStream, req, ec);
        if (ec)
        {
            return fail(ec, "write");
        }
        onWriteHandler(ec, bytes_transferred);
    }
    void read(auto& buffer, auto& res,
              std::function<void(beast::error_code, std::size_t)> onReadHandler)
    {
        beast::error_code ec{};
        // Receive the HTTP response
        auto bytes_transferred = http::read(mStream, buffer, res, ec);
        if (ec)
        {
            return fail(ec, "read");
        }
        onReadHandler(ec, bytes_transferred);
    }
};
struct TcpStream : public SyncStream<beast::tcp_stream>
{
  protected:
    void on_resolve(std::function<void(beast::error_code)> connectionHandler,
                    tcp::resolver::results_type results) override
    {
        // Set a timeout on the operation
        lowestLayer().expires_after(std::chrono::seconds(30));
        beast::error_code ec{};
        // Make the connection on the IP address we get from a lookup
        auto epType = lowestLayer().connect(results, ec);
        if (ec)
        {
            return fail(ec, "connect");
        }
        connectionHandler(ec);
    }

  public:
    TcpStream(net::any_io_executor ex) : SyncStream(beast::tcp_stream(ex)) {}

    void shutDown() override
    {
        lowestLayer().close();
    }
};
struct SSLStream : public SyncStream<beast::ssl_stream<beast::tcp_stream>>
{
  private:
    void on_resolve(std::function<void(beast::error_code)> connectionHandler,
                    tcp::resolver::results_type results) override
    {
        // Set a timeout on the operation
        lowestLayer().expires_after(std::chrono::seconds(30));
        beast::error_code ec{};
        // Make the connection on the IP address we get from a lookup
        auto epType = lowestLayer().connect(results, ec);
        if (ec)
        {
            return fail(ec, "connect");
        }
        handShake(std::move(connectionHandler));
    }
    void handShake(std::function<void(beast::error_code)>&& connectionHandler)
    {
        beast::error_code ec{};
        // Perform the SSL handshake
        stream().handshake(ssl::stream_base::client, ec);
        if (ec)
        {
            return fail(ec, "handshake");
        }
        connectionHandler(ec);
    }

  public:
    SSLStream(net::any_io_executor ex, ssl::context& ctx) :
        SyncStream(beast::ssl_stream<beast::tcp_stream>(ex, ctx))
    {}
    void shutDown() override
    {
        lowestLayer().expires_after(std::chrono::seconds(30));
        beast::error_code ec{};
        // Gracefully close the stream
        stream().shutdown(ec);
        if (ec == net::error::eof)
        {
            ec = {};
        }
        if (ec)
            return fail(ec, "shutdown");
        // If we get here then the connection is closed
        // gracefully
    }
};

template <typename Stream>
struct ASyncStream : public std::enable_shared_from_this<ASyncStream<Stream>>
{
    using Base = std::enable_shared_from_this<ASyncStream<Stream>>;

  private:
    Stream mStream;

  protected:
    ASyncStream(Stream&& strm) : mStream(std::move(strm)) {}
    auto& lowestLayer()
    {
        return beast::get_lowest_layer(mStream);
    }
    Stream& stream()
    {
        return mStream;
    }
    void on_resolve(std::function<void(beast::error_code)> connectionHandler,
                    beast::error_code ec, tcp::resolver::results_type results)
    {
        if (ec)
            return fail(ec, "resolve");

        // Set a timeout on the operation
        lowestLayer().expires_after(std::chrono::seconds(30));

        // Make the connection on the IP address we get from a
        // lookup
        lowestLayer().async_connect(
            results, beast::bind_front_handler(&ASyncStream::on_connect,
                                               Base::shared_from_this(),
                                               std::move(connectionHandler)));
    }
    virtual void
        on_connect(std::function<void(beast::error_code)> connectionHandler,
                   beast::error_code ec,
                   tcp::resolver::results_type::endpoint_type) = 0;
    void on_write(
        std::function<void(beast::error_code, std::size_t)> onWriteHandler,
        beast::error_code ec, std::size_t bytes_transferred)
    {
        if (ec)
            return fail(ec, "write");
        onWriteHandler(ec, bytes_transferred);
    }

    void on_read(
        std::function<void(beast::error_code, std::size_t)> onReadHandler,
        beast::error_code ec, std::size_t bytes_transferred)
    {
        if (ec)
            return fail(ec, "read");
        onReadHandler(ec, bytes_transferred);
    }

  public:
    void resolve(tcp::resolver& resolver, const char* host, const char* port,
                 std::function<void(beast::error_code)> handler)
    {
        resolver.async_resolve(
            host, port,
            beast::bind_front_handler(&ASyncStream::on_resolve,
                                      Base::shared_from_this(),
                                      std::move(handler)));
    }
    virtual void shutDown() = 0;
    template <typename Body>
    void write(
        http::request<Body>& req,
        std::function<void(beast::error_code, std::size_t)> onWriteHandler)
    {
        // Send the HTTP request to the remote host
        http::async_write(stream(), req,
                          std::bind_front(&ASyncStream::on_write,
                                          Base::shared_from_this(),
                                          std::move(onWriteHandler)));
    }
    void read(auto& buffer, auto& res,
              std::function<void(beast::error_code, std::size_t)> onReadHandler)
    {
        // Receive the HTTP response
        http::async_read(stream(), buffer, res,
                         std::bind_front(&ASyncStream::on_read,
                                         Base::shared_from_this(),
                                         std::move(onReadHandler)));
    }
};
struct ASyncTcpStream : public ASyncStream<beast::tcp_stream>
{
  private:
    void on_connect(std::function<void(beast::error_code)> connectionHandler,
                    beast::error_code ec,
                    tcp::resolver::results_type::endpoint_type) override
    {
        if (ec)
            return fail(ec, "connect");
        connectionHandler(ec);
    }

  public:
    ASyncTcpStream(net::any_io_executor ex) : ASyncStream(beast::tcp_stream(ex))
    {}
    void shutDown()
    {
        stream().close();
    }
};
struct ASyncSslStream : public ASyncStream<beast::ssl_stream<beast::tcp_stream>>
{
  private:
    void on_connect(std::function<void(beast::error_code)> connectionHandler,
                    beast::error_code ec,
                    tcp::resolver::results_type::endpoint_type) override
    {
        if (ec)
            return fail(ec, "connect");

        // Perform the SSL handshake
        stream().async_handshake(
            ssl::stream_base::client,
            [thisp = Base::shared_from_this(),
             connHandler = std::move(connectionHandler)](beast::error_code ec) {
            static_cast<ASyncSslStream*>(thisp.get())
                ->on_handshake(std::move(connHandler), ec);
            });
    }
    void on_shutdown(beast::error_code ec)
    {
        if (ec == net::error::eof)
        {
            ec = {};
        }
        if (ec)
            return fail(ec, "shutdown");

        // If we get here then the connection is closed
        // gracefully
    }
    void on_handshake(std::function<void(beast::error_code)> connectionHandler,
                      beast::error_code ec)
    {
        if (ec)
            return fail(ec, "handshake");
        connectionHandler(ec);
    }

  public:
    ASyncSslStream(net::any_io_executor ex, ssl::context& ctx) :
        ASyncStream(beast::ssl_stream<beast::tcp_stream>(ex, ctx))
    {}

    void shutDown()
    {
        // Set a timeout on the operation
        beast::get_lowest_layer(stream()).expires_after(
            std::chrono::seconds(30));

        // // Gracefully close the stream
        stream().async_shutdown(
            [thisp = Base::shared_from_this()](beast::error_code ec) {
            static_cast<ASyncSslStream*>(thisp.get())->on_shutdown(ec);
        });
    }
};
// Performs an HTTP GET and prints the response
template <typename Stream>
class session : public std::enable_shared_from_this<session<Stream>>
{
    using Base = std::enable_shared_from_this<session<Stream>>;
    tcp::resolver resolver_;
    std::shared_ptr<Stream> stream;
    beast::flat_buffer buffer_; // (Must persist between reads)
    http::request<http::empty_body> req_;
    http::response<http::string_body> res_;

  public:
    explicit session(net::any_io_executor ex, Stream&& astream) :
        resolver_(ex), stream(std::make_shared<Stream>(std::move(astream)))
    {}

    // Start the asynchronous operation
    void run(const char* host, const char* port, const char* target,
             int version)
    {
        // Set SNI Hostname (many hosts need this to handshake
        // successfully)

        // Set up an HTTP GET request message
        req_.version(version);
        req_.method(http::verb::get);
        req_.target(target);
        req_.set(http::field::host, host);
        req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        stream->resolve(
            resolver_, host, port,
            std::bind_front(&session::on_connect, Base::shared_from_this()));
    }
    void on_connect(beast::error_code ec)
    {
        stream->write(req_, std::bind_front(&session::on_write,
                                            Base::shared_from_this()));
    }
    void on_write(beast::error_code ec, std::size_t bytes_transferred)
    {
        stream->read(
            buffer_, res_,
            std::bind_front(&session::on_read, Base::shared_from_this()));
    }
    void on_read(beast::error_code ec, std::size_t bytes_transferred)
    {
        // Write the message to standard out
        std::cout << res_ << std::endl;
        stream->shutDown();
    }
};
