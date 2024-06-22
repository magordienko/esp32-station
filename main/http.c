#include "http.h"

/* Используемые макросы */
#define SERVER_PORT 80
#define IS_FILE_EXT(filename, ext) (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)

static const char *TAG = "http";
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename)
{
    if (IS_FILE_EXT(filename, ".pdf"))
    {
        return httpd_resp_set_type(req, "application/pdf");
    }
    else if (IS_FILE_EXT(filename, ".html"))
    {
        return httpd_resp_set_type(req, "text/html");
    }
    else if (IS_FILE_EXT(filename, ".jpeg"))
    {
        return httpd_resp_set_type(req, "image/jpeg");
    }
    else if (IS_FILE_EXT(filename, ".ico"))
    {
        return httpd_resp_set_type(req, "image/x-icon");
    }
    return httpd_resp_set_type(req, "text/plain");
}

static const char *get_path_from_uri(char *dest, const char *base_path, const char *uri, size_t destsize)
{
    const size_t base_pathlen = strlen(base_path);
    size_t pathlen = strlen(uri);
    const char *quest = strchr(uri, '?');
    if (quest)
    {
        pathlen = MIN(pathlen, quest - uri);
    }
    const char *hash = strchr(uri, '#');
    if (hash)
    {
        pathlen = MIN(pathlen, hash - uri);
    }
    if (base_pathlen + pathlen + 1 > destsize)
    {
        return NULL;
    }
    strcpy(dest, base_path);
    strlcpy(dest + base_pathlen, uri, pathlen + 1);
    return dest + base_pathlen;
}
//-------------------------------------------------------------
static esp_err_t download_get_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];
    FILE *fd = NULL;
    char *buf;
    size_t buf_len;
    struct stat file_stat;

    const char *filename = get_path_from_uri(filepath, ((struct file_server_data *)req->user_ctx)->base_path,
                                             req->uri, sizeof(filepath));
    if (!filename)
    {
        ESP_LOGE(TAG, "Filename is too long");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Filename too long");
        return ESP_FAIL;
    }
    if (strcmp(filename, "/") == 0)
    {
        strcat(filepath, "index.html");
    }
    stat(filepath, &file_stat);
    fd = fopen(filepath, "r");
    if (!fd)
    {
        ESP_LOGE(TAG, "Failed to read existing file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Sending file : %s (%ld bytes)...", filename, file_stat.st_size);
    set_content_type_from_file(req, filename);

    buf_len = httpd_req_get_url_query_len(req) + 1;
    printf("buf_len: %d\n", buf_len);

    if (buf_len > 1)
    {
        buf = malloc(buf_len);

        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
        {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            char param[32];
            if (httpd_query_key_value(buf, "red", param, sizeof(param)) == ESP_OK)
            {
                ESP_LOGI(TAG, "Found URL query parameter => red:%s", param);
                if (!strcmp(param, "RED+ON"))
                    gpio_set_level(GPIO_LED, 0);
                else if (!strcmp(param, "RED+OFF"))
                    gpio_set_level(GPIO_LED, 1);
            }
        }
        free(buf);
    }

    char *chunk = ((struct file_server_data *)req->user_ctx)->scratch;
    size_t chunksize;
    do
    {
        chunksize = fread(chunk, 1, SCRATCH_BUFSIZE, fd);
        if (chunksize > 0)
        {
            if (httpd_resp_send_chunk(req, chunk, chunksize) != ESP_OK)
            {
                fclose(fd);
                ESP_LOGE(TAG, "File sending failed!");
                httpd_resp_sendstr_chunk(req, NULL);
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (chunksize != 0);
    fclose(fd);
    ESP_LOGI(TAG, "File sending complete");
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t download_post_handler(httpd_req_t *req)
{
    char *resp_str = NULL;
    char *buf;
    size_t buf_len;

    buf_len = httpd_req_get_url_query_len(req) + 1;
    printf("buf_len: %d\n", buf_len);

    if (buf_len > 1)
    {
        buf = malloc(buf_len);

        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
        {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            char param[32];
            if (httpd_query_key_value(buf, "red", param, sizeof(param)) == ESP_OK)
            {
                ESP_LOGI(TAG, "Found URL query parameter => red:%s", param);
                if (!strcmp(param, "1"))
                    gpio_set_level(GPIO_LED, 1);
                else if (!strcmp(param, "0"))
                    gpio_set_level(GPIO_LED, 0);
            }

            resp_str = (char *)req->user_ctx;
            resp_str = malloc(200);

            strcpy(resp_str, "<table>\
        <tr><th>RED</th><th>");
            if (gpio_get_level(GPIO_LED))
                strcat(resp_str, "ON");
            else
                strcat(resp_str, "OFF");
            strcat(resp_str, "</td><td>");
            strcat(resp_str, "</td></tr></table>");

            httpd_resp_send(req, resp_str, strlen(resp_str));

            free(resp_str);
        }
        free(buf);
    }

    return ESP_OK;
}
//-------------------------------------------------------------
httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    static struct file_server_data *server_data = NULL;
    if (server_data)
    {
        ESP_LOGE(TAG, "File server already started");
        return ESP_ERR_INVALID_STATE;
    }
    server_data = calloc(1, sizeof(struct file_server_data));
    if (!server_data)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for server data");
        return ESP_ERR_NO_MEM;
    }
    strlcpy(server_data->base_path, "/spiffs",
            sizeof(server_data->base_path));
    config.uri_match_fn = httpd_uri_match_wildcard;

    config.lru_purge_enable = true;

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);

    if (httpd_start(&server, &config) != ESP_OK)
    {
        ESP_LOGI(TAG, "Error starting server!");
        return NULL;
    }

    ESP_LOGI(TAG, "Registering URI handlers");

    httpd_uri_t file_download = {
        .uri = "/*", // Match all URIs of type /path/to/file
        .method = HTTP_GET,
        .handler = download_get_handler,
        .user_ctx = server_data // Pass server data as context
    };

    httpd_uri_t file_download_post = {
        .uri = "/*",
        .method = HTTP_POST,
        .handler = download_post_handler,
        .user_ctx = NULL};

    httpd_register_uri_handler(server, &file_download);
    httpd_register_uri_handler(server, &file_download_post);
    return server;
}
//-------------------------------------------------------------
void stop_webserver(httpd_handle_t server)
{
    httpd_stop(server);
}