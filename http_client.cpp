#include "http/http_client.hpp"

#include "http/web_client.hpp"

#include <map>
struct Sink
{
    void operator()(auto&& data, auto&& token) const
    {
        std::cout << data;
        token();
    }
};
int main(int argc, char** argv)
{
    // The io_context is required for all I/O
    net::io_context ioc;

    // The SSL context is required, and holds certificates
    ssl::context ctx{ssl::context::tlsv12_client};

    // This holds the root certificate used for verification
    // load_root_certificates(ctx);

    // Verify the remote server's certificate
    ctx.set_verify_mode(ssl::verify_none);
    auto m = Mono<int>::just(10);

    m.subscribe([](auto v, auto&& requestNext) {
        std::cout << v << std::endl;
        requestNext(true);
    });

    auto m2 = Flux<std::pair<std::string, std::string>>::range(
        std::map{std::make_pair(std::string("name"), std::string("hello")),
                 std::make_pair(std::string("greetings"), std::string("hi"))});
    m2.map([](auto&& v) {
          v.second += " world";
          return v;
      })
        .map([](auto&& v) {
            v.second += " example";
            return v;
        })
        .onFinish([]() { std::cout << "end of stream\n"; })
        .subscribe([](auto v) {
            std::cout << v.first << "," << v.second << std::endl;
        });

    auto ex = net::make_strand(ioc);

    using SourceSession = HttpSession<AsyncSslStream>;
    std::shared_ptr<SourceSession> session =
        SourceSession::create(ex, AsyncSslStream(ex, ctx));
    auto m3 = Flux<std::string>::connect(session,
                                         "https://127.0.0.1:8443/machines");
    m3.map([](auto v) {
        std::string_view view(v);
        return std::string(view.substr(0, 10));
    });
    using SinkSession = HttpSession<AsyncSslStream, http::string_body>;
    auto sinksession = SinkSession::create(ex, AsyncSslStream(ex, ctx));
    HttpSink<std::string, SinkSession> sink(sinksession);
    sink.setUrl("https://127.0.0.1:8443/test")
        .onData(
            [](auto& res, bool& needNext) { std::cout << res << std::endl; });

    m3.subscribe(std::move(sink));

    std::cout << session->inUse() << std::endl;
    ioc.run();

    return EXIT_SUCCESS;
}
