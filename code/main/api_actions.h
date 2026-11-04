#pragma once

#ifndef API_ACTIONS_H
#define API_ACTIONS_H

#include <esp_http_server.h>

// POST /api/v1/actions/flow-start: starts the next round now instead of after the interval.
esp_err_t api_action_flow_start_handler(httpd_req_t *req);

// POST /api/v1/actions/reboot: restarts the device a few seconds after answering.
esp_err_t api_action_reboot_handler(httpd_req_t *req);

#endif // API_ACTIONS_H
