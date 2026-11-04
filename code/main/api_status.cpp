#include "api_status.h"

#include <stdlib.h>
#include <string>

#include "api_response.h"
#include "MainFlowControl.h"

namespace
{

// How long a client should wait when the flow isn't ready, in seconds
constexpr char RETRY_AFTER_S[] = "10";

// The flow keeps its results as text, which is empty or not a number when there's no valid value.
void add_number_or_null(cJSON *object, const char *key, const std::string &text)
{
    char *end = NULL;
    const double number = strtod(text.c_str(), &end);

    if (text.empty() || *end != '\0')
    {
        cJSON_AddNullToObject(object, key);
    }
    else
    {
        cJSON_AddNumberToObject(object, key, number);
    }
}

// The flow writes the offset as +0200, but ISO 8601 needs +02:00 next to the dashes and colons of the rest.
std::string to_iso_8601(std::string timestamp)
{
    if (!timestamp.empty())
    {
        timestamp.insert(timestamp.length() - 2, ":");
    }
    return timestamp;
}

void add_flow(cJSON *root)
{
    cJSON *flow = cJSON_AddObjectToObject(root, "flow");
    cJSON_AddNumberToObject(flow, "round", getCountFlowRounds());
    cJSON_AddBoolToObject(flow, "running", getFlowIsRunning());
    cJSON_AddStringToObject(flow, "status", flowctrl.getActStatus().c_str());
}

void add_numbers(cJSON *root)
{
    cJSON *numbers = cJSON_AddObjectToObject(root, "numbers");

    for (const NumberResult &result : flowctrl.getNumberResults())
    {
        cJSON *number = cJSON_AddObjectToObject(numbers, result.name.c_str());
        add_number_or_null(number, "value", result.value);
        api_add_string_or_null(number, "raw", result.raw.c_str());
        add_number_or_null(number, "pre", result.pre);
        api_add_string_or_null(number, "error", result.error == "no error" ? "" : result.error.c_str());
        add_number_or_null(number, "rate", result.rate);
        api_add_string_or_null(number, "timestamp", to_iso_8601(result.timestamp).c_str());
    }
}

} // namespace

esp_err_t api_status_get_handler(httpd_req_t *req)
{
    if (!getFlowIsInitialized())
    {
        httpd_resp_set_hdr(req, "Retry-After", RETRY_AFTER_S);
        return api_send_error(req, "503 Service Unavailable", "flow_not_ready", "The flow isn't initialized yet");
    }

    cJSON *root = cJSON_CreateObject();
    add_flow(root);
    add_numbers(root);

    return api_send_json(req, root);
}
