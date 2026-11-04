#include "api_actions.h"

#include "api_response.h"
#include "MainFlowControl.h"
#include "server_ota.h"

namespace
{

constexpr char STATUS_CONFLICT[] = "409 Conflict";

} // namespace

esp_err_t api_action_flow_start_handler(httpd_req_t *req)
{
    if (!getFlowIsInitialized())
    {
        return api_send_flow_not_ready(req);
    }

    // Has to be checked first: the flow task also waits inside a round, for example before taking the image,
    // and triggerFlowStart() would cut that wait short.
    if (getFlowIsRunning())
    {
        return api_send_error(req, STATUS_CONFLICT, "flow_busy", "A round is running");
    }

    const int next_round = getCountFlowRounds() + 1;

    if (!triggerFlowStart())
    {
        return api_send_error(req, STATUS_CONFLICT, "flow_not_active", "Automatic rounds aren't active");
    }

    cJSON *body = cJSON_CreateObject();
    cJSON_AddNumberToObject(body, "round", next_round);

    return api_send_accepted(req, body);
}

esp_err_t api_action_reboot_handler(httpd_req_t *req)
{
    // The answer has to go out first: doReboot() takes the network down a few seconds later
    esp_err_t result = api_send_accepted(req, cJSON_CreateObject());
    doReboot();
    return result;
}
