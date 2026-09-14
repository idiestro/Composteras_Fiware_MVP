/*
    ApiClient - HTTP client for REST APIs
    Created by Ignacio Diestro Gil - 2026

*/

#include "ApiClient.h"
#include <WiFi.h>
#include <memory>

// Traza condicional: si APICLIENT_DEBUG_ENABLED es 0 el compilador elimina todo.
#if APICLIENT_DEBUG_ENABLED
#define DBG(...)                            \
    do                                      \
    {                                       \
        if (_debug && _debugOut)            \
            _debugOut->printf(__VA_ARGS__); \
    } while (0)
#else
#define DBG(...) \
    do           \
    {            \
    } while (0)
#endif

// --------------------------------------------------------------------------
// ApiResponse
// --------------------------------------------------------------------------

String ApiResponse::header(const String &name) const
{
    for (const auto &h : headers)
    {
        if (h.first.equalsIgnoreCase(name))
            return h.second;
    }
    return String();
}

// --------------------------------------------------------------------------
// Construcción y configuración
// --------------------------------------------------------------------------

ApiClient::ApiClient(const String &baseUrl)
{
    setBaseUrl(baseUrl);
    // Por defecto asumimos JSON, típico en NGSIv2 / NGSI-LD.
    setHeader("Content-Type", "application/json");
    setHeader("Accept", "application/json");

    // Cabeceras de respuesta útiles por defecto:
    //  - Location: id/URL de la entidad recién creada en un POST /v2/entities
    //  - Fiware-Total-Count: total de resultados cuando se consulta con ?options=count
    collectResponseHeader("Content-Type");
    collectResponseHeader("Location");
    collectResponseHeader("Fiware-Total-Count");
}

void ApiClient::setBaseUrl(const String &baseUrl)
{
    _baseUrl = baseUrl;
    _baseUrl.trim();
    while (_baseUrl.endsWith("/"))
    {
        _baseUrl.remove(_baseUrl.length() - 1);
    }
}

void ApiClient::setHeader(const String &name, const String &value)
{
    for (auto &h : _headers)
    {
        if (h.first.equalsIgnoreCase(name))
        {
            h.second = value;
            return;
        }
    }
    _headers.push_back(std::make_pair(name, value));
}

void ApiClient::removeHeader(const String &name)
{
    for (size_t i = 0; i < _headers.size(); ++i)
    {
        if (_headers[i].first.equalsIgnoreCase(name))
        {
            _headers.erase(_headers.begin() + i);
            return;
        }
    }
}

void ApiClient::clearHeaders()
{
    _headers.clear();
}

void ApiClient::setBearerToken(const String &token)
{
    setHeader("Authorization", "Bearer " + token);
}

// --------------------------------------------------------------------------
// Cabeceras de respuesta
// --------------------------------------------------------------------------

void ApiClient::collectResponseHeader(const String &name)
{
    for (const auto &n : _collect)
    {
        if (n.equalsIgnoreCase(name))
            return; // ya declarada
    }
    _collect.push_back(name);
}

void ApiClient::clearCollectedHeaders()
{
    _collect.clear();
}

void ApiClient::applyCollectedHeaders(HTTPClient &http)
{
    if (_collect.empty())
        return;

    // HTTPClient copia las claves a String internamente, así que este array
    // temporal de punteros es seguro.
    std::vector<const char *> keys;
    keys.reserve(_collect.size());
    for (const auto &n : _collect)
        keys.push_back(n.c_str());

    http.collectHeaders(keys.data(), keys.size());
}

void ApiClient::readResponseHeaders(HTTPClient &http, ApiResponse &res)
{
    const int n = http.headers();
    res.headers.reserve(n);
    for (int i = 0; i < n; ++i)
    {
        const String value = http.header(i);
        if (value.length() == 0)
            continue; // no venía en la respuesta
        res.headers.push_back(std::make_pair(http.headerName(i), value));
    }
}

// --------------------------------------------------------------------------
// Debug por Serial
// --------------------------------------------------------------------------

void ApiClient::setDebug(bool enabled, Stream &out)
{
    _debug = enabled;
    _debugOut = &out;
}

bool ApiClient::isSensitiveHeader(const String &name)
{
    return name.equalsIgnoreCase("Authorization") ||
           name.equalsIgnoreCase("X-Auth-Token") ||
           name.equalsIgnoreCase("Cookie");
}

void ApiClient::logBody(const String &body)
{
#if APICLIENT_DEBUG_ENABLED
    if (!_debug || !_debugOut)
        return;
    DBG("[ApiClient]     body (%u B)", (unsigned)body.length());
    if (body.length() == 0)
    {
        DBG("\n");
        return;
    }
    if (_debugBodyLimit > 0 && body.length() > _debugBodyLimit)
    {
        DBG(": %s... [truncado]\n", body.substring(0, _debugBodyLimit).c_str());
    }
    else
    {
        DBG(": %s\n", body.c_str());
    }
#else
    (void)body;
#endif
}

void ApiClient::logRequest(const char *method, const String &url, const String &body)
{
#if APICLIENT_DEBUG_ENABLED
    if (!_debug || !_debugOut)
        return;
    DBG("[ApiClient] --> %s %s\n", method, url.c_str());
    for (const auto &h : _headers)
    {
        DBG("[ApiClient]     %s: %s\n", h.first.c_str(),
            isSensitiveHeader(h.first) ? "<oculto>" : h.second.c_str());
    }
    logBody(body);
#else
    (void)method;
    (void)url;
    (void)body;
#endif
}

void ApiClient::logResponse(const ApiResponse &res, unsigned long elapsedMs)
{
#if APICLIENT_DEBUG_ENABLED
    if (!_debug || !_debugOut)
        return;
    DBG("[ApiClient] <-- %d en %lu ms%s\n", res.status, elapsedMs,
        res.ok() ? "" : "  (!)");
    for (const auto &h : res.headers)
    {
        DBG("[ApiClient]     %s: %s\n", h.first.c_str(), h.second.c_str());
    }
    logBody(res.body);
#else
    (void)res;
    (void)elapsedMs;
#endif
}

// --------------------------------------------------------------------------
// Llamadas
// --------------------------------------------------------------------------

ApiResponse ApiClient::get(const String &path)
{
    return request("GET", path);
}

ApiResponse ApiClient::post(const String &path, const String &body)
{
    return request("POST", path, body);
}

ApiResponse ApiClient::put(const String &path, const String &body)
{
    return request("PUT", path, body);
}

ApiResponse ApiClient::patch(const String &path, const String &body)
{
    return request("PATCH", path, body);
}

ApiResponse ApiClient::del(const String &path)
{
    return request("DELETE", path);
}

// --------------------------------------------------------------------------
// Funciones Núcleo
// --------------------------------------------------------------------------

String ApiClient::buildUrl(const String &path) const
{
    // Permite pasar una URL absoluta y saltarse la base.
    if (path.startsWith("http://") || path.startsWith("https://"))
    {
        return path;
    }
    if (path.length() == 0)
        return _baseUrl;
    return path.startsWith("/") ? _baseUrl + path : _baseUrl + "/" + path;
}

ApiResponse ApiClient::request(const char *method, const String &path, const String &body)
{
    ApiResponse res;

    if (WiFi.status() != WL_CONNECTED)
    {
        res.status = ERR_NO_WIFI;
        res.body = "WiFi no conectada";
        DBG("[ApiClient] !!! %s abortado: sin WiFi\n", method);
        return res;
    }

    const String url = buildUrl(path);
    if (url.length() == 0)
    {
        res.status = ERR_NO_URL;
        res.body = "URL vacia (baseUrl sin configurar)";
        DBG("[ApiClient] !!! %s abortado: URL vacia\n", method);
        return res;
    }

    logRequest(method, url, body);

    // Elegimos el transporte según el esquema. unique_ptr para que se libere
    // el socket aunque salgamos por un return intermedio.
    std::unique_ptr<WiFiClient> client;
    if (url.startsWith("https://"))
    {
        auto *secure = new WiFiClientSecure();
        if (_caCert != nullptr)
        {
            secure->setCACert(_caCert);
        }
        else if (_insecure)
        {
            secure->setInsecure(); // sin validación de certificado
        }
        client.reset(secure);
    }
    else
    {
        client.reset(new WiFiClient());
    }

    HTTPClient http;
    http.setConnectTimeout(_timeoutMs);
    http.setTimeout(_timeoutMs);
    http.setReuse(false);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    if (!http.begin(*client, url))
    {
        res.status = ERR_BEGIN;
        res.body = "http.begin() fallo para " + url;
        DBG("[ApiClient] !!! %s\n", res.body.c_str());
        return res;
    }

    for (auto &h : _headers)
    {
        http.addHeader(h.first, h.second);
    }

    // Debe declararse antes de enviar la petición.
    applyCollectedHeaders(http);

    const unsigned long t0 = millis();

    // sendRequest acepta cualquier método, incluido PATCH (necesario en NGSIv2).
    int code;
    if (body.length() > 0)
    {
        code = http.sendRequest(method, (uint8_t *)body.c_str(), body.length());
    }
    else
    {
        code = http.sendRequest(method);
    }

    const unsigned long elapsed = millis() - t0;

    res.status = code;
    if (code > 0)
    {
        readResponseHeaders(http, res);
        res.body = http.getString();
    }
    else
    {
        res.body = HTTPClient::errorToString(code);
    }

    logResponse(res, elapsed);

    http.end();
    return res;
}