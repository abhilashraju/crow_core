#include "http/http_client.hpp"

#include "http/web_client.hpp"
int main(int argc, char** argv)
{
    // Check command line arguments.
    if (argc != 4 && argc != 5)
    {
        std::cerr
            << "Usage: http-client-async-ssl <host> <port> <target> [<HTTP version: 1.0 or 1.1(default)>]\n"
            << "Example:\n"
            << "    http-client-async-ssl www.example.com 443 /\n"
            << "    http-client-async-ssl www.example.com 443 / 1.0\n";
        return EXIT_FAILURE;
    }
    const auto host = argv[1];
    const auto port = argv[2];
    const auto target = argv[3];
    int version = argc == 5 && !std::strcmp("1.0", argv[4]) ? 10 : 11;

    // The io_context is required for all I/O
    net::io_context ioc;

    // The SSL context is required, and holds certificates
    ssl::context ctx{ssl::context::tlsv12_client};

    // This holds the root certificate used for verification
    // load_root_certificates(ctx);

    // Verify the remote server's certificate
    ctx.set_verify_mode(ssl::verify_none);
    auto m = Mono<int>::just(10);
    m.subscribe([](auto v) { std::cout << v << std::endl; });
    auto m2 = Mono<std::string>::just("hello");
    m2.map([](auto&& v) { return v + " world"; })
        .map([](auto&& v) { return v + " example"; })
        .onFinish([]() { std::cout << "end of stream\n"; })
        .subscribe([](auto v) { std::cout << v << std::endl; });
    // Launch the asynchronous operation
    // The session is constructed with a strand to
    // ensure that handlers do not execute concurrently.
    auto ex = net::make_strand(ioc);
    // std::make_shared<HttpSession<ASyncSslStream>>(ex, ASyncSslStream(ex,
    // ctx))
    //     ->run(host, port, target, http::verb::get, version);
    auto session = HttpSession<ASyncSslStream>::create(ex,
                                                       ASyncSslStream(ex, ctx));
    auto m3 = Mono<std::string>::connect(session,
                                         "https://127.0.0.1:8443/machines");
    m3.subscribe([](auto v) { std::cout << v << std::endl; });

    // Run the I/O service. The call will return when
    // the get operation is complete.
    ioc.run();

    return EXIT_SUCCESS;
}
