#ifndef FIWARE_CONFIG_H
#define FIWARE_CONFIG_H

// Auth configuration
#define FIWARE_AUTH_URL "https://auth.iotplatform.telefonica.com:15001"
#define FIWARE_AUTH_FINAL_PATH "/v3/auth/tokens"
#define FIWARE_AUTH_USERNAME "FIWARE_AUTH_USERNAME"
#define FIWARE_AUTH_PASSWORD "FIWARE_AUTH_PASSWORD"
#define FIWARE_AUTH_DOMAIN_NAME "FIWARE_AUTH_DOMAIN_NAME"
#define FIWARE_AUTH_PROJECT_NAME "FIWARE_AUTH_PROJECT_NAME"

// Data configuration
#define FIWARE_DATA_URL "https://cb.iotplatform.telefonica.com:10027"
#define FIWARE_DATA_FINAL_PATH "/v2/entities/{entityId}/attrs"

#endif