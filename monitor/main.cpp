#include <iostream>
#include <thread>
#include <httplib.h>
#include <chrono>
#include "config.h"
#define nl "\n"

template <typename T>
using Result = std::expected<T, std::string>;

using Err = std::unexpected<std::string>;

struct NewtorkMonitor
{
    std::string switch_hostname{config::SWITCH_HOST};
    std::string app_hostname{config::APP_HOST};

    int failureCount{0};

    NetworkMonitor() = default;

    Result<void> pingApp()
    {

        try
        {
            httplib::Client client{app_hostname};
            auto res = client.Get("/");

            if (!res)
                return Err{"bad connection"};

            if (res->status >= 400)
            {
                return Err{res->body};
            }
        }
        catch (...)
        {
            return Err{"error making request"};
        }

        return {};
    }

    void triggerSwitch()
    {
        std::cout << "triggering deadman" << nl;

        httplib::Client client{switch_hostname};

        client.Get("/trigger");
        std::this_thread::sleep_for(std::chrono::seconds(15));
        client.Get("/trigger");
    }

    void run()
    {

        for (;;)
        {

            int failures{};
            for (;;)
            {
                if (failures > 5)
                    break;
                auto res = pingApp();
                if (!res)
                {
                    ++failures;
                }
                else
                {
                    failures = 0;
                }
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
            // send  restart
            triggerSwitch();

            // wait 2 min for boot
            std::cout << "waiting 2 min for boot sequence" << nl;
            std::this_thread::sleep_for(std::chrono::seconds(120));
        }
    }
};

int main()
{

    auto last = std::chrono::steady_clock::now();

    std::cout << "starting monitor" << nl;
}