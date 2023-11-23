#include "boost/beast/core/flat_buffer.hpp"
#include "boost/beast/http/serializer.hpp"
#include "http/http_connection.hpp"
#include "http/http_response.hpp"
#include "simple_client.hpp"
#include "testapp.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/beast/_experimental/test/stream.hpp>

#include <filesystem>
#include <fstream>
#include <thread>

#include "gtest/gtest.h"

namespace
{

inline auto stringSplitter(char c)
{
    return std::views::split(c) | std::views::transform([](auto&& sub) {
               return std::string(sub.begin(), sub.end());
           });
}
inline auto split(std::string_view input, char c)
{
    auto vw = input | stringSplitter(c);
    return std::vector(vw.begin(), vw.end());
}

struct FakeFileHanadler
{
    std::string root = "/tmp/";
    explicit FakeFileHanadler(boost::asio::io_context& ioIn) : io(ioIn) {}
    static void
        handleUpgrade(crow::Request& /*req*/,
                      const std::shared_ptr<bmcweb::AsyncResp>& /*asyncResp*/,
                      boost::beast::test::stream&& /*adaptor*/)
    {
        // Handle Upgrade should never be called
        EXPECT_FALSE(true);
    }
    void handle(crow::Request& req,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
    {
        asyncResp->res.addHeader("myheader", "myvalue");
        if (req.target() == "/hello")
        {
            asyncResp->res.write("Hello World");
            return;
        }
        auto paths = split(req.target(), '/');

        boost::beast::http::file_body::value_type body;

        auto filetofetch = std::filesystem::path(root).c_str() + paths.back();
        asyncResp->res.openFile(filetofetch);
    }
    boost::asio::io_context& io;
    bool called = false;
};

class ConnectionTest : public ::testing::Test
{
    std::string_view path = "/tmp/temp.txt";

  protected:
    void SetUp() override
    {
        std::string_view s = "sample text from file";
        std::ofstream file;
        file.open(path.data());
        file << s;
        file.close();
    }

    void TearDown() override
    {
        std::filesystem::remove(path);
    }
};
void executeRequest(std::string_view target, boost::beast::test::stream& out,
                    boost::beast::test::stream& stream,
                    boost::asio::io_context& io)
{
    std::string url = "GET ";
    url.append(target);
    url.append(
        " HTTP/1.1\r\nHost: openbmc_project.xyz\r\nConnection: close\r\n\r\n");

    out.write_some(boost::asio::buffer(url.data(), url.length()));
    FakeFileHanadler handler(io);
    boost::asio::steady_timer timer(io);
    std::function<std::string()> date = []() { return "data"; };
    std::shared_ptr<
        crow::Connection<boost::beast::test::stream, FakeFileHanadler>>
        conn = std::make_shared<
            crow::Connection<boost::beast::test::stream, FakeFileHanadler>>(
            &handler, std::move(timer), date, std::move(stream));
    conn->start();
    io.run();
}
TEST_F(ConnectionTest, GetRootStringBody)
{
    boost::asio::io_context io;
    boost::beast::test::stream stream(io);
    boost::beast::test::stream out(io);
    stream.connect(out);
    executeRequest("/hello", out, stream, io);
    beast::error_code ec;
    beast::flat_buffer fb;
    http::response_parser<http::string_body> p;

    http::read_header(out, fb, p, ec);
    const auto bytes_transferred = http::read(out, fb, p, ec);
    boost::ignore_unused(bytes_transferred);

    EXPECT_EQ(p.get().body(), "Hello World");
    EXPECT_EQ(p.get()["myheader"], "myvalue");
    char* ptr = nullptr;
    EXPECT_EQ(strtol(p.get()["Content-Length"].data(), &ptr, 10),
              std::string_view("Hello World").length());
}

TEST_F(ConnectionTest, GetRootFileBody)
{
    boost::asio::io_context io;
    boost::beast::test::stream stream(io);
    boost::beast::test::stream out(io);
    stream.connect(out);
    executeRequest("/temp.txt", out, stream, io);
    beast::error_code ec;
    beast::flat_buffer fb;
    http::response_parser<http::string_body> p;

    http::read_header(out, fb, p, ec);
    const auto bytes_transferred = http::read(out, fb, p, ec);
    boost::ignore_unused(bytes_transferred);

    EXPECT_EQ(p.get().body(), "sample text from file");
    EXPECT_EQ(p.get()["myheader"], "myvalue");
    char* ptr = nullptr;
    EXPECT_EQ(strtol(p.get()["Content-Length"].data(), &ptr, 10),
              std::string_view("sample text from file").length());
}

} // Namespace
