#include "fiwareClient.h"
#include "ApiClient.h"
#include "config/fiwareConfig.h"

const char *tokenTemplate =
    "{\"auth\":{"
    "\"identity\":{"
    "\"methods\":[\"password\"],"
    "\"password\":{\"user\":{"
    "\"domain\":{\"name\":\"{domainName}\"},"
    "\"name\":\"{username}\","
    "\"password\":\"{password}\""
    "}}"
    "},"
    "\"scope\":{\"project\":{"
    "\"domain\":{\"name\":\"{domainName}\"},"
    "\"name\":\"{projectName}\""
    "}}"
    "}}";

ApiClient authClient(FIWARE_AUTH_URL);
ApiClient dataClient(FIWARE_DATA_URL);

fiwareClient::fiwareClient()
{
    // Set response auth header
    authClient.collectResponseHeader("X-Subject-Token");
    // Set debug options
    authClient.setDebug(true);
    dataClient.setDebug(true);
}

String fiwareClient::getToken()
{
    // Set token headers
    setTokenHeaders();
    // Set token paylaod
    String payload = setTokenPayload();

    // Get Fiware Token
    _tokenResponse = authClient.post(FIWARE_AUTH_FINAL_PATH, payload);
    String token = "";

    if (_tokenResponse.status == 201)
    {
        token = _tokenResponse.header("X-Subject-Token");
        _token = token;
        Serial.println("[FiwareClient] Token got successfully");
    }
    else
    {
        Serial.println("[FiwareClient] Error during token retry: " + String(_tokenResponse.status) + " - " + _tokenResponse.body);
    }
    return token;
}

ApiResponse fiwareClient::sendData(const String &entityId, const String &payload)
{
    // Set data headers
    setDataHeaders();

    // Send data to Fiware
    String finalPath = setDataUrl(entityId);
    ApiResponse _dataResponse = dataClient.post(finalPath, payload);

    // Check if token is alive and retry request if needed
    if (!isTokenAlive(_dataResponse.status))
    {
        ApiResponse _dataResponse = dataClient.post(finalPath, payload);
    }

    if (_dataResponse.status == 201)
    {
        Serial.println("[FiwareClient] Fiware data sent successfully");
    }
    else
    {
        Serial.println("[FiwareClient] Error during Fiware data sending: " + String(_dataResponse.status) + " - " + _dataResponse.body);
    }
    return _dataResponse;
}

bool fiwareClient::isTokenAlive(int dataStatusCode)
{
    // Temporal method until esp32 working will be defined
    if (dataStatusCode != 201)
    {
        Serial.println("[FiwareClient] Token has expired, getting a new one...");
        getToken();
        return false;
    }
    else
    {
        Serial.println("[FiwareClient] Token is still alive");
        return true;
    }
}

void fiwareClient::setTokenHeaders()
{
    authClient.setHeader("Content-Type", "application/json");
    authClient.setHeader("Accept", "application/json");
}

void fiwareClient::setDataHeaders()
{
    dataClient.setHeader("Content-Type", "application/json");
    dataClient.setHeader("Fiware-Service", FIWARE_AUTH_DOMAIN_NAME);
    dataClient.setHeader("Fiware-ServicePath", FIWARE_AUTH_PROJECT_NAME);
    dataClient.setHeader("X-Auth-Token", _token);
}

String fiwareClient::setTokenPayload()
{
    String payload = tokenTemplate;
    payload.replace("{domainName}", FIWARE_AUTH_DOMAIN_NAME);
    payload.replace("{username}", FIWARE_AUTH_USERNAME);
    payload.replace("{password}", FIWARE_AUTH_PASSWORD);
    payload.replace("{projectName}", FIWARE_AUTH_PROJECT_NAME);
    return payload;
}

String fiwareClient::setDataUrl(String entityId)
{
    String finalPath = FIWARE_DATA_URL;
    finalPath.replace("{entityId}", entityId);
}