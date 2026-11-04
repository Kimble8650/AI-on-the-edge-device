#pragma once

#ifndef API_ROUTER_H
#define API_ROUTER_H

#include <string>

#include <esp_http_server.h>

// Reported in GET /api/v1/info. Within v1, changes only add things.
constexpr int API_VERSION = 1;

// Returns one segment of the path below /api/v1, decoded and counted from 0: in /numbers/main/prevalue, index 1 is "main".
// Only for handlers, whose route has already matched, so the segment exists.
std::string api_get_path_segment(const httpd_req_t *req, size_t index);

// Registers the single handler that serves everything below /api/v1/.
// Must be called before the "/*" catch-all is registered, because handlers match in registration order.
void register_api_uri(httpd_handle_t server);

#endif // API_ROUTER_H
