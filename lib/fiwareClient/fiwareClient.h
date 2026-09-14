#pragma once

#include "ApiClient.h"

class fiwareClient
{
public:
    fiwareClient();
    String getToken();
    ApiResponse sendData(const String &entityId, const String &payload);

private:
    void setTokenHeaders();
    void setDataHeaders();
    bool isTokenAlive(int dataStatusCode);
    String setTokenPayload();
    String setDataUrl(String entitiId);

    String _token;
    ApiResponse _tokenResponse;
    ApiResponse _dataResponse;
};