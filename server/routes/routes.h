#pragma once

#include "auth/auth_manager.h"
#include "storage/storage_manager.h"
#include <httplib.h>

namespace vault::server 
{
void setup_routes(httplib::Server& server,
                  AuthManager& auth,
                  StorageManager& storage);

}
