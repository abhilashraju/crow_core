#include "boost/beast/core/flat_buffer.hpp"
#include "boost/beast/http/serializer.hpp"
#include "http/http_response.hpp"
#include "utility.hpp"

#include <filesystem>
#include <fstream>
#include <thread>

#include "gtest/gtest.h"

static void addHeaders(crow::Response& res)
{
    res.addHeader("myheader", "myvalue");
    res.keepAlive(true);
    res.result(boost::beast::http::status::ok);
}
static void verifyHeaders(crow::Response& res)
{
    EXPECT_EQ(res.getHeaderValue("myheader"), "myvalue");
    EXPECT_EQ(res.keepAlive(), true);
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
}
template <class Serializer>
struct Lambda
{
    Serializer& sr;
    mutable boost::beast::flat_buffer buffer;
    explicit Lambda(Serializer& s) : sr(s) {}

    template <class ConstBufferSequence>
    void operator()(boost::beast::error_code& ec,
                    const ConstBufferSequence& buffers) const
    {
        ec = {};
        buffer.commit(boost::asio::buffer_copy(
            buffer.prepare(boost::beast::buffer_bytes(buffers)), buffers));
        sr.consume(boost::beast::buffer_bytes(buffers));
    }
};
template <class Serializer>
Lambda(Serializer&) -> Lambda<Serializer>;

template <bool isRequest, class Body, class Fields>
auto writeMessage(boost::beast::http::serializer<isRequest, Body, Fields>& sr,
                  boost::beast::error_code& ec)
{
    sr.split(true);
    Lambda body(sr);
    Lambda header(sr);
    do
    {
        sr.next(ec, header);
    } while (!sr.is_header_done());
    if (!ec && !sr.is_done())
    {
        do
        {
            sr.next(ec, body);
        } while (!ec && !sr.is_done());
    }
    return body;
}

std::string makePath(std::thread::id threadID)
{
    std::stringstream stream;
    stream << "/tmp/" << threadID << ".txt";
    return stream.str();
}
void makeFile(const std::string& path, auto genrator)
{
    std::ofstream file;
    std::string_view s = genrator();
    file.open(path);
    file << s;
    file.close();
}
TEST(http_response, Defaults)
{
    crow::Response res;
    EXPECT_EQ(
        boost::variant2::holds_alternative<crow::Response::string_response>(
            res.response),
        true);
}
TEST(http_response, headers)
{
    crow::Response res;
    addHeaders(res);
    verifyHeaders(res);
}
TEST(http_response, stringbody)
{
    crow::Response res;
    addHeaders(res);
    std::string_view bodyvalue = "this is my new body";
    res.write({bodyvalue.data(), bodyvalue.length()});
    EXPECT_EQ(*res.body(), bodyvalue);
    verifyHeaders(res);
}
TEST(http_response, filebody)
{
    crow::Response res;
    addHeaders(res);
    std::string path = makePath(std::this_thread::get_id());
    makeFile(path, []() { return "sample text"; });
    res.openFile(path);

    verifyHeaders(res);
    std::filesystem::remove(path);
}
TEST(http_response, body_transitions)
{
    crow::Response res;
    addHeaders(res);
    std::string_view s = "sample text";
    std::string path = makePath(std::this_thread::get_id());
    makeFile(path, []() { return "sample text"; });
    res.openFile(path);

    EXPECT_EQ(boost::variant2::holds_alternative<crow::Response::file_response>(
                  res.response),
              true);

    verifyHeaders(res);
    res.write("body text");

    EXPECT_EQ(
        boost::variant2::holds_alternative<crow::Response::string_response>(
            res.response),
        true);

    verifyHeaders(res);
    std::filesystem::remove(path);
}
TEST(http_response, base64_body_transitions)
{
    crow::Response res;
    addHeaders(res);
    std::string_view s = "sample text";
    std::string path = makePath(std::this_thread::get_id());
    auto dataMaker = [&]() {
        int size{0};
        std::string str;
        while (size < 5000)
        {
            str += s;

            size += s.length();
        }
        return str;
    };
    makeFile(path, dataMaker);
    res.openFile<crow::Response::base64file_response>(path);

    EXPECT_EQ(
        boost::variant2::holds_alternative<crow::Response::base64file_response>(
            res.response),
        true);
    boost::beast::error_code ec{};
    crow::Response::base64file_response& bodyResp =
        boost::variant2::get<crow::Response::base64file_response>(res.response);
    boost::beast::http::response_serializer<bmcweb::Base64FileBody> sr{
        bodyResp};
    Lambda visit = writeMessage(sr, ec);

    EXPECT_EQ(ec.operator bool(), false);
    const auto b = boost::beast::buffers_front(visit.buffer.data());
    std::string_view s1{static_cast<char*>(b.data()), b.size()};
    std::string decoded;
    auto success = std::move(crow::utility::base64Decode(s1, decoded));
    EXPECT_EQ(success, true);
    EXPECT_EQ(decoded, dataMaker());

    verifyHeaders(res);
    res.write("body text");

    EXPECT_EQ(
        boost::variant2::holds_alternative<crow::Response::string_response>(
            res.response),
        true);

    verifyHeaders(res);
    std::filesystem::remove(path);
}
