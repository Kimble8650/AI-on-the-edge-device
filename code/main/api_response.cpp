#include "api_response.h"

namespace
{

// Prints body with the given status line. Doesn't delete body.
esp_err_t send_body(httpd_req_t *req, const char *status, const cJSON *body)
{
    char *text = cJSON_PrintUnformatted(body);
    if (text == NULL)
    {
        // Memory ran out, so a JSON error body may not fit either
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Out of memory");
    }

    httpd_resp_set_status(req, status);
    httpd_resp_set_type(req, "application/json");
    esp_err_t result = httpd_resp_sendstr(req, text);
    cJSON_free(text);
    return result;
}

} // namespace

void api_add_string_or_null(cJSON *object, const char *key, const char *value)
{
    if (value[0] == '\0')
    {
        cJSON_AddNullToObject(object, key);
    }
    else
    {
        cJSON_AddStringToObject(object, key, value);
    }
}

esp_err_t api_send_json(httpd_req_t *req, cJSON *body)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*"); // Reads only, same as the old endpoints
    esp_err_t result = send_body(req, HTTPD_200, body);
    cJSON_Delete(body);
    return result;
}

esp_err_t api_send_accepted(httpd_req_t *req, cJSON *body)
{
    esp_err_t result = send_body(req, "202 Accepted", body);
    cJSON_Delete(body);
    return result;
}

esp_err_t api_send_flow_not_ready(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Retry-After", "10");
    return api_send_error(req, "503 Service Unavailable", "flow_not_ready", "The flow isn't initialized yet");
}

esp_err_t api_send_error(httpd_req_t *req, const char *status, const char *code, const char *message)
{
    cJSON *body = cJSON_CreateObject();
    cJSON *error = cJSON_AddObjectToObject(body, "error");
    cJSON_AddStringToObject(error, "code", code);
    cJSON_AddStringToObject(error, "message", message);

    esp_err_t result = send_body(req, status, body);
    cJSON_Delete(body);
    return result;
}
