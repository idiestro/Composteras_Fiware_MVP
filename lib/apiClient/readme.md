# ApiClient

Cliente REST genérico para **Seeed XIAO ESP32-S3** (Arduino framework). Envuelve `HTTPClient` con una API más corta, soporte de cabeceras persistentes, lectura de cabeceras de respuesta y traza por puerto serie.

Pensado para APIs **FIWARE** (Orion / Orion-LD, IoT Agent, QuantumLeap), pero sirve para cualquier API REST JSON.

## Requisitos

- Arduino core para ESP32 (v2.x o v3.x)
- Librerías del core: `WiFi`, `HTTPClient`, `WiFiClientSecure`
- Sin dependencias externas

## Instalación

Copia `ApiClient.h` y `ApiClient.cpp` en `src/` (PlatformIO) o en la carpeta del sketch (Arduino IDE).

```ini
; platformio.ini
[env:seeed_xiao_esp32s3]
platform = espressif32
board = seeed_xiao_esp32s3
framework = arduino
monitor_speed = 115200
; opcional: elimina la traza del binario
; build_flags = -DAPICLIENT_DEBUG_ENABLED=0
```

## Uso básico

```cpp
#include <WiFi.h>
#include "ApiClient.h"

ApiClient api("http://orion.local:1026");

void setup() {
  Serial.begin(115200);
  WiFi.begin("SSID", "password");
  while (WiFi.status() != WL_CONNECTED) delay(200);

  api.setHeader("fiware-service", "smartcity");
  api.setHeader("fiware-servicepath", "/sensores");
  api.setDebug(true);

  ApiResponse r = api.get("/v2/entities/Sensor:001");
  if (r.ok()) Serial.println(r.body);
  else        Serial.printf("Error %d: %s\n", r.status, r.body.c_str());
}

void loop() {}
```

## API

### Configuración

| Método | Descripción |
|---|---|
| `ApiClient(baseUrl)` / `setBaseUrl(url)` | URL base. Se le quitan las `/` finales. |
| `setHeader(name, value)` | Añade o sustituye una cabecera de petición (se envía en todas). |
| `removeHeader(name)` / `clearHeaders()` | Quita cabeceras. |
| `setBearerToken(token)` | Atajo para `Authorization: Bearer ...`. |
| `setTimeout(ms)` | Timeout de conexión y lectura. Por defecto 8000. |
| `setInsecure(bool)` | HTTPS sin validar certificado. **Solo para pruebas.** |
| `setCACert(pem)` | Certificado raíz. El puntero debe seguir vivo. |

El constructor deja puestas `Content-Type: application/json` y `Accept: application/json`.

### Peticiones

```cpp
ApiResponse get(path);
ApiResponse post(path, body = "");
ApiResponse put(path, body = "");
ApiResponse patch(path, body = "");   // NGSIv2 usa PATCH para atributos
ApiResponse del(path);
ApiResponse request(method, path, body = "");   // cualquier petición
```

`path` puede ser relativa (`/v2/entities`) o una URL absoluta, que ignora la base. El transporte (HTTP o HTTPS) se elige según el esquema de la URL.

### Respuesta

```cpp
struct ApiResponse {
  int    status;    // >0 código HTTP, <0 error
  String body;      // cuerpo, o descripción del error si status < 0
  bool   ok() const;                   // 2xx
  String header(name) const;           // "" si no está
  bool   hasHeader(name) const;
};
```

Códigos negativos: `-100` sin WiFi, `-101` fallo al abrir la conexión, `-102` URL vacía. Entre `-1` y `-11` son errores de `HTTPClient` (`errorToString` ya viene en `body`).

### Cabeceras de respuesta

`HTTPClient` obliga a declarar de antemano qué cabeceras capturar. Por defecto se recogen `Content-Type`, `Location` y `Fiware-Total-Count`.

```cpp
api.collectResponseHeader("Fiware-Correlator");

ApiResponse r = api.post("/v2/entities", payload);
Serial.println(r.header("Location"));   // /v2/entities/Sensor:001?type=Sensor
```

### Debug por serie

```cpp
api.setDebug(true);              // o setDebug(true, Serial1)
api.setDebugBodyLimit(256);      // 0 = sin truncar (por defecto 512)
```

```
[ApiClient] --> PATCH http://orion:1026/v2/entities/Sensor:001/attrs
[ApiClient]     Content-Type: application/json
[ApiClient]     Authorization: <oculto>
[ApiClient]     body (46 B): {"temperature":{"type":"Number","value":23.4}}
[ApiClient] <-- 204 en 132 ms
```

`Authorization`, `X-Auth-Token` y `Cookie` se enmascaran. Con `-DAPICLIENT_DEBUG_ENABLED=0` el preprocesador elimina toda la traza.

## Ejemplos con Orion (NGSIv2)

```cpp
// Crear entidad
api.post("/v2/entities",
  "{\"id\":\"Sensor:001\",\"type\":\"Sensor\","
  "\"temperature\":{\"type\":\"Number\",\"value\":21.0}}");

// Actualizar atributos -> 204
api.patch("/v2/entities/Sensor:001/attrs",
  "{\"temperature\":{\"type\":\"Number\",\"value\":23.4}}");

// Consultar por tipo
ApiResponse r = api.get("/v2/entities?type=Sensor&options=keyValues");

// Borrar
api.del("/v2/entities/Sensor:001");
```

## Limitaciones

- `request()` es **bloqueante**: no lo llames desde una ISR y ten en cuenta el watchdog en peticiones lentas.
- `getString()` carga todo el cuerpo en RAM. Cuidado con consultas de muchas entidades.
- No reutiliza la conexión (`setReuse(false)`), así que cada petición HTTPS repite el handshake TLS.
- No construye query params ni hace URL-encoding: van escritos a mano en `path`.