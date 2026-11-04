#include "api_numbers.h"

#include <stdlib.h>
#include <string>

#include "api_response.h"
#include "api_router.h"
#include "MainFlowControl.h"

namespace
{

constexpr size_t NUMBER_NAME_SEGMENT = 1; // In /numbers/{name}/prevalue

// UpdatePrevalue() takes the value as text, and a negative one means "use the latest raw reading".
constexpr char USE_RAW_VALUE[] = "-1";

} // namespace

esp_err_t api_number_prevalue_put_handler(httpd_req_t *req)
{
    if (!getFlowIsInitialized())
    {
        return api_send_flow_not_ready(req);
    }

    const std::string name = api_get_path_segment(req, NUMBER_NAME_SEGMENT);

    if (flowctrl.GetPrevalue(name).empty())
    {
        return api_send_error(req, "404 Not Found", "not_found", "Unknown number");
    }

    cJSON *request = api_receive_json(req);
    if (request == NULL)
    {
        return ESP_OK; // The error is already sent
    }

    const cJSON *value = cJSON_GetObjectItemCaseSensitive(request, "value");
    const bool use_raw = cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(request, "use_raw"));
    const bool value_is_valid = cJSON_IsNumber(value) && value->valuedouble >= 0;
    const std::string new_value = value_is_valid ? std::to_string(value->valuedouble) : USE_RAW_VALUE;
    const bool has_one_source = (value != NULL) != use_raw;
    cJSON_Delete(request);

    if (!has_one_source || (value != NULL && !value_is_valid))
    {
        return api_send_field_error(req, "value", "Must be a number of at least 0, unless use_raw is true");
    }

    if (!flowctrl.UpdatePrevalue(new_value, name, true))
    {
        return api_send_error(req, "409 Conflict", "no_valid_raw_value", "The latest raw reading can't be used");
    }

    cJSON *body = cJSON_CreateObject();
    cJSON_AddNumberToObject(body, "pre", strtod(flowctrl.GetPrevalue(name).c_str(), NULL));

    return api_send_changed(req, body);
}
