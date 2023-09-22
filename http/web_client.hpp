#pragma once
#include "http_client.hpp"

#include <boost/url/url.hpp>
#include <boost/url/url_view.hpp>

#include <numeric>
#include <ranges>
template <typename T>
struct FluxBase
{
    struct SourceHandler
    {
        virtual void next(std::function<void(T)> consumer) = 0;
        virtual bool hasNext() const = 0;
    };
    using CompletionToken = std::function<void()>;

    using Handler = std::function<void(const T&)>;
    using HandlerWithCompletionToken =
        std::function<void(const T&, CompletionToken&&)>;

  protected:
    explicit FluxBase(SourceHandler* srcHandler) : mSource(srcHandler) {}
    std::unique_ptr<SourceHandler> mSource{};
    std::function<void()> onFinishHandler{};
    std::vector<std::function<T(T)>> mapHandlers{};
    std::function<void(T, std::function<void()>)> subscriber;

  public:
    void subscribe(Handler handler)
    {
        if (mSource->hasNext())
        {
            mSource->next([handler = std::move(handler), this](T v) {
                auto r = std::accumulate(begin(mapHandlers), end(mapHandlers),
                                         v, [](auto sofar, auto& func) {
                                             return func(std::move(sofar));
                                         });
                handler(r);
                subscribe(std::move(handler));
            });
            return;
        }
        if (onFinishHandler)
        {
            onFinishHandler();
        }
    }
    void subscribe(auto handler)
    {
        subscriber = std::move(handler);
        if (mSource->hasNext())
        {
            mSource->next([handler = std::move(handler), this](T v) {
                auto r = std::accumulate(begin(mapHandlers), end(mapHandlers),
                                         v, [](auto sofar, auto& func) {
                                             return func(std::move(sofar));
                                         });
                subscriber(r, [this] { subscribe(std::move(subscriber)); });
            });
            return;
        }
        if (onFinishHandler)
        {
            onFinishHandler();
        }
    }
    FluxBase& onFinish(std::function<void()> finishH)
    {
        onFinishHandler = std::move(finishH);
        return *this;
    }
    FluxBase& map(std::function<T(T)> mapFun)
    {
        mapHandlers.push_back(std::move(mapFun));
        return *this;
    }
};

template <typename T, typename Session>
struct HttpSource : FluxBase<T>::SourceHandler
{
    int count{1};
    std::shared_ptr<Session> session;
    std::string url;
    http::verb verb;
    explicit HttpSource(std::shared_ptr<Session> aSession, int shots) :
        session(std::move(aSession)), count(shots)
    {}
    void setUrl(std::string u)
    {
        url = std::move(u);
    }
    void setVerb(http::verb v)
    {
        verb = v;
    }

    void next(std::function<void(T)> consumer) override
    {
        count--;
        session->setResponseHandler(
            [consumer = std::move(consumer)](
                const http::response<http::string_body>& res) {
            consumer(res.body());
        });
        boost::urls::url_view urlvw(url);
        std::string h = urlvw.host();
        std::string p = urlvw.port();
        std::string path = urlvw.path();
        session->setOptions(Host{h}, Port{p}, Target{path}, Version{11},
                            Verb{verb}, KeepAlive{true});
        session->run();
    }
    bool hasNext() const override
    {
        return count > 0;
    }
};

template <typename T, typename Session>
struct HttpSink
{
    std::shared_ptr<Session> session;
    std::string url;
    http::verb verb;
    std::string data;
    explicit HttpSink(std::shared_ptr<Session> aSession) :
        session(std::move(aSession))
    {}
    void setUrl(std::string u)
    {
        url = std::move(u);
    }
    void setVerb(http::verb v)
    {
        verb = v;
    }
    void setData(std::string d)
    {
        data = std::move(d);
    }

    void operator()(T&& data, auto&& token)
    {
        session->setResponseHandler(
            [token = std::move(token)](
                const http::response<http::string_body>& res) {
            std::cout << res;
            token();
        });
        boost::urls::url_view urlvw(url);
        std::string h = urlvw.host();
        std::string p = urlvw.port();
        std::string path = urlvw.path();
        http::string_body::value_type body(data);

        session->setOptions(Host{h}, Port{p}, Target{path}, Version{11},
                            Verb{http::verb::post}, KeepAlive{true}, body,
                            ContentType{"plain/text"});
        session->run();
    }
};
// template <typename T, typename Session>
// HttpSource(std::shared_ptr<Session>, int) -> HttpSource<T, Session>;
template <typename T>
struct Mono : FluxBase<T>
{
    using Base = FluxBase<T>;
    struct Just : Base::SourceHandler
    {
        T value{};
        bool mHasNext{true};
        explicit Just(T v) : value(std::move(v)) {}
        void next(std::function<void(T)> consumer) override
        {
            mHasNext = false;
            consumer(std::move(value));
        }
        bool hasNext() const override
        {
            return mHasNext;
        }
    };

    explicit Mono(Base::SourceHandler* srcHandler) : Base(srcHandler) {}

    static Mono just(T v)
    {
        return Mono{new Just(std::move(v))};
    }
    template <typename Stream>
    static Mono
        connect(std::shared_ptr<
                    HttpSession<Stream, http::empty_body, http::string_body>>
                    session,
                const std::string& url)
    {
        auto src = new HttpSource<
            T, HttpSession<Stream, http::empty_body, http::string_body>>(
            session, 1);
        src->setUrl(url);
        src->setVerb(http::verb::get);
        auto m = Mono{src};
        return m;
    }
};

struct WebClient
{};
