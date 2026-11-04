#include "api_router.h"

#include <string>
#include <strings.h>

#include "api_actions.h"
#include "api_info.h"
#include "api_response.h"
#include "api_status.h"
#include "basic_auth.h"

namespace
{

// One endpoint: path relative to /api/v1 and the method it answers to.
// The same path may appear several times with different methods.
struct ApiRoute
{
    httpd_method_t method;
    const char *path;
    esp_err_t (*handler)(httpd_req_t *req);
};

const ApiRoute api_routes[] = {
    {HTTP_GET, "/info", api_info_get_handler},
    {HTTP_GET, "/status", api_status_get_handler},
    {HTTP_POST, "/actions/flow-start", api_action_flow_start_handler},
    {HTTP_POST, "/actions/reboot", api_action_reboot_handler},
};

constexpr char API_PATH_PREFIX[] = "/api/v1";

// Returns the request path relative to /api/v1, without the query string.
std::string get_route_path(const httpd_req_t *req)
{
    std::string path(req->uri);
    path = path.substr(0, path.find('?'));
    return path.substr(sizeof(API_PATH_PREFIX) - 1);
}

// True for application/json, with or without parameters such as charset.
bool has_json_content_type(httpd_req_t *req)
{
    constexpr char JSON_TYPE[] = "application/json";
    constexpr size_t JSON_TYPE_LENGTH = sizeof(JSON_TYPE) - 1;

    char content_type[32] = ""; // A longer value is cut off, which is enough to see its start
    httpd_req_get_hdr_value_str(req, "Content-Type", content_type, sizeof(content_type));

    const char next = content_type[JSON_TYPE_LENGTH];
    return strncasecmp(content_type, JSON_TYPE, JSON_TYPE_LENGTH) == 0 && (next == '\0' || next == ';' || next == ' ');
}

// Finds the route for the request and calls it. All of /api/v1/ comes through here,
// so it takes up one URI handler slot however many routes the table has.
esp_err_t api_router_handler(httpd_req_t *req)
{
    const std::string path = get_route_path(req);
    std::string allowed_methods; // Methods that exist for this path, for the Allow header of a 405

    for (const ApiRoute &route : api_routes)
    {
        if (path != route.path)
        {
            continue;
        }

        if (route.method == req->method)
        {
            // A plain web form can't send this type, and a script on another site needs a CORS preflight for it.
            // The preflight gets a 405, so other sites can't change anything on the device.
            if (route.method != HTTP_GET && !has_json_content_type(req))
            {
                return api_send_error(req, "415 Unsupported Media Type", "unsupported_media_type", "Content-Type must be application/json");
            }

            return route.handler(req);
        }

        allowed_methods += (allowed_methods.empty() ? "" : ", ");
        allowed_methods += http_method_str(route.method);
    }

    if (allowed_methods.empty())
    {
        return api_send_error(req, "404 Not Found", "not_found", "Unknown endpoint");
    }

    // httpd keeps the pointer until the response is sent, and allowed_methods outlives that
    httpd_resp_set_hdr(req, "Allow", allowed_methods.c_str());
    return api_send_error(req, "405 Method Not Allowed", "method_not_allowed", "Method not supported on this endpoint");
}

// Runs the router behind basic auth, with the 401 in the same JSON format as the other errors.
esp_err_t api_authorized_router_handler(httpd_req_t *req)
{
    return basic_auth_request_filter(req, api_router_handler, [](httpd_req_t *unauthorized_req) {
        httpd_resp_set_hdr(unauthorized_req, "WWW-Authenticate", BASIC_AUTH_CHALLENGE);
        return api_send_error(unauthorized_req, "401 Unauthorized", "unauthorized", "Valid credentials are required");
    });
}

} // namespace

void register_api_uri(httpd_handle_t server)
{
    httpd_uri_t api_handle = {
        .uri      = "/api/v1/*",
        .method   = static_cast<httpd_method_t>(HTTP_ANY), // The router checks the method, so wrong ones get a JSON error
        .handler  = api_authorized_router_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &api_handle);
}
