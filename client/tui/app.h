#pragma once

#include "network/api_client.h"
#include <memory>

namespace vault::client 
{
    class App 
    {
    public:
        explicit App(std::shared_ptr<ApiClient> api);
        void run();

    private:
        std::shared_ptr<ApiClient> api_;
    };

} 
