#pragma once

#ifndef API_STATUS_H
#define API_STATUS_H

#include <esp_http_server.h>

// GET /api/v1/status: the latest readings and the state of the flow, for polling.
esp_err_t api_status_get_handler(httpd_req_t *req);

#endif // API_STATUS_H
