/*
    ApiClient - HTTP client for REST APIs with token authentication
    Created by Ignacio Diestro Gil - 2026

    Uso mínimo:
      ApiClient api("http://mi-orion.local:1026");
      api.setHeader("fiware-service", "smartcity");
      api.setHeader("fiware-servicepath", "/sensores");
      api.setDebug(true);                        // traza por Serial
      api.collectResponseHeader("Location");     // cabecera que queremos leer
      ApiResponse r = api.get("/v2/entities/Sensor:001");
      if (r.ok()) Serial.println(r.body);
      else        Serial.printf("Error %d: %s\n", r.status, r.body.c_str());

    El debug se puede eliminar del binario en compilación con:
      build_flags = -DAPICLIENT_DEBUG_ENABLED=0
 */

#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <utility>
#include <vector>

// Permite compilar sin el código de traza (ahorra flash y evita fugas de datos).
#ifndef APICLIENT_DEBUG_ENABLED
#define APICLIENT_DEBUG_ENABLED 1
#endif

/** Resultado de una petición. */
struct ApiResponse
{
    int status = 0; // >0 código HTTP; <0 error de transporte/cliente
    String body;    // cuerpo de la respuesta o descripción del error

    /** Cabeceras de respuesta capturadas (solo las declaradas con collectResponseHeader). */
    std::vector<std::pair<String, String>> headers;

    bool ok() const { return status >= 200 && status < 300; }

    /** Valor de una cabecera de respuesta, o "" si no se capturó / no venía. */
    String header(const String &name) const;
    bool hasHeader(const String &name) const { return header(name).length() > 0; }
};

class ApiClient
{
public:
    // Errores propios (fuera del rango de HTTPClient, que usa -1..-11)
    static constexpr int ERR_NO_WIFI = -100;
    static constexpr int ERR_BEGIN = -101;
    static constexpr int ERR_NO_URL = -102;

    explicit ApiClient(const String &baseUrl = "");

    // --- Configuración ---
    void setBaseUrl(const String &baseUrl);
    String baseUrl() const { return _baseUrl; }

    /** Añade o sustituye una cabecera de petición (sin distinguir mayúsculas). */
    void setHeader(const String &name, const String &value);
    void removeHeader(const String &name);
    void clearHeaders();

    /** Atajo para APIs protegidas (Keyrock/Wilma en el stack FIWARE). */
    void setBearerToken(const String &token);

    void setTimeout(uint16_t ms) { _timeoutMs = ms; }

    /** HTTPS sin validar el certificado. Cómodo para pruebas, NO para producción. */
    void setInsecure(bool insecure = true) { _insecure = insecure; }

    /** Certificado raíz en PEM. El puntero debe seguir vivo (usa un const char* global). */
    void setCACert(const char *pemCert) { _caCert = pemCert; }

    // --- Cabeceras de respuesta (punto 8) ---
    /**
     * Declara una cabecera de respuesta a capturar. HTTPClient no permite leerlas
     * todas: hay que decir de antemano cuáles interesan. Por defecto se capturan
     * Content-Type, Location y Fiware-Total-Count.
     */
    void collectResponseHeader(const String &name);
    void clearCollectedHeaders();

    // --- Debug por Serial (punto 12) ---
    /** Activa la traza de método, URL, cabeceras, cuerpo, código y tiempo. */
    void setDebug(bool enabled, Stream &out = Serial);
    bool debug() const { return _debug; }

    /** Máximo de caracteres de cuerpo que se imprimen (0 = sin límite). */
    void setDebugBodyLimit(size_t chars) { _debugBodyLimit = chars; }

    // --- Verbos ---
    ApiResponse get(const String &path);
    ApiResponse post(const String &path, const String &body = "");
    ApiResponse put(const String &path, const String &body = "");
    ApiResponse patch(const String &path, const String &body = ""); // NGSIv2 usa PATCH para atributos
    ApiResponse del(const String &path);

    /** Petición genérica: cualquier método HTTP. */
    ApiResponse request(const char *method, const String &path, const String &body = "");

private:
    String buildUrl(const String &path) const;
    void applyCollectedHeaders(HTTPClient &http);
    void readResponseHeaders(HTTPClient &http, ApiResponse &res);

    // Helpers de traza (vacíos si APICLIENT_DEBUG_ENABLED == 0)
    void logRequest(const char *method, const String &url, const String &body);
    void logResponse(const ApiResponse &res, unsigned long elapsedMs);
    void logBody(const String &body);
    static bool isSensitiveHeader(const String &name);

    String _baseUrl;
    std::vector<std::pair<String, String>> _headers;
    std::vector<String> _collect;
    uint16_t _timeoutMs = 8000;
    bool _insecure = true;
    const char *_caCert = nullptr;

    bool _debug = false;
    Stream *_debugOut = &Serial;
    size_t _debugBodyLimit = 512;
};