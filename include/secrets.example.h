#pragma once

// Copy this file to secrets.h and fill in local development values.
// Never commit secrets.h.
inline constexpr char WIFI_SSID[] = "your-wifi-name";
inline constexpr char WIFI_PASSWORD[] = "your-wifi-password";
inline constexpr char OYNK_API_URL[] = "https://api.example.com/health";

// PEM-encoded root CA certificate for the API host. Keep certificate
// validation enabled; do not replace this with an insecure TLS mode.
inline constexpr char OYNK_API_ROOT_CA[] = R"CERT(
-----BEGIN CERTIFICATE-----
replace-with-api-root-ca
-----END CERTIFICATE-----
)CERT";
