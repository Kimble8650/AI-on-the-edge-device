#include "api_actions.h"

#include "api_response.h"
#include "MainFlowControl.h"

namespace
{

// How long a client should wait when the flow isn't ready, in seconds
constexpr char RETRY_AFTER_S[] = "10";

constexpr char STATUS_CONFLICT[] = "409 Conflict";

} // namespace

esp_err_t api_action_flow_start_handler(httpd_req_t *req)
{
    if (!getFlowIsInitialized())
    {
        httpd_resp_set_hdr(req, "Retry-After", RETRY_AFTER_S);
        return api_send_error(req, "503 Service Unavailable", "flow_not_ready", "The flow isn't initialized yet");
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
