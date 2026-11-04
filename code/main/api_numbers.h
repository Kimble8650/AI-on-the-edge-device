#pragma once

#ifndef API_NUMBERS_H
#define API_NUMBERS_H

#include <esp_http_server.h>

// PUT /api/v1/numbers/{name}/prevalue: sets the last valid reading of a number.
esp_err_t api_number_prevalue_put_handler(httpd_req_t *req);

#endif // API_NUMBERS_H
