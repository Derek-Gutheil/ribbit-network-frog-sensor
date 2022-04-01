// This sketch provide the functionality of OTA Firmware Upgrade
#include "WiFi.h"
#include "HttpsOTAUpdate.h"
#include <time.h>
// This sketch shows how to implement HTTPS firmware update Over The Air.
// Please provide your WiFi credentials, https URL to the firmware image and the server certificate.

static const char *ssid     = "GutHause";  // your network SSID (name of wifi network)
static const char *password = "*****"; // your network password

static const char *url = "https://raw.githubusercontent.com/Derek-Gutheil/ribbit-network-frog-sensor/feature/OTA/firmware/OTA/test_bin.ino.bin"; //state url of your firmware image

static const char *server_certificate = "-----BEGIN CERTIFICATE-----\n" \
     "MIIEvjCCA6agAwIBAgIQBtjZBNVYQ0b2ii+nVCJ+xDANBgkqhkiG9w0BAQsFADBh\n" \
     "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
     "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n" \
     "QTAeFw0yMTA0MTQwMDAwMDBaFw0zMTA0MTMyMzU5NTlaME8xCzAJBgNVBAYTAlVT\n" \
     "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxKTAnBgNVBAMTIERpZ2lDZXJ0IFRMUyBS\n" \
     "U0EgU0hBMjU2IDIwMjAgQ0ExMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKC\n" \
     "AQEAwUuzZUdwvN1PWNvsnO3DZuUfMRNUrUpmRh8sCuxkB+Uu3Ny5CiDt3+PE0J6a\n" \
     "qXodgojlEVbbHp9YwlHnLDQNLtKS4VbL8Xlfs7uHyiUDe5pSQWYQYE9XE0nw6Ddn\n" \
     "g9/n00tnTCJRpt8OmRDtV1F0JuJ9x8piLhMbfyOIJVNvwTRYAIuE//i+p1hJInuW\n" \
     "raKImxW8oHzf6VGo1bDtN+I2tIJLYrVJmuzHZ9bjPvXj1hJeRPG/cUJ9WIQDgLGB\n" \
     "Afr5yjK7tI4nhyfFK3TUqNaX3sNk+crOU6JWvHgXjkkDKa77SU+kFbnO8lwZV21r\n" \
     "eacroicgE7XQPUDTITAHk+qZ9QIDAQABo4IBgjCCAX4wEgYDVR0TAQH/BAgwBgEB\n" \
     "/wIBADAdBgNVHQ4EFgQUt2ui6qiqhIx56rTaD5iyxZV2ufQwHwYDVR0jBBgwFoAU\n" \
     "A95QNVbRTLtm8KPiGxvDl7I90VUwDgYDVR0PAQH/BAQDAgGGMB0GA1UdJQQWMBQG\n" \
     "CCsGAQUFBwMBBggrBgEFBQcDAjB2BggrBgEFBQcBAQRqMGgwJAYIKwYBBQUHMAGG\n" \
     "GGh0dHA6Ly9vY3NwLmRpZ2ljZXJ0LmNvbTBABggrBgEFBQcwAoY0aHR0cDovL2Nh\n" \
     "Y2VydHMuZGlnaWNlcnQuY29tL0RpZ2lDZXJ0R2xvYmFsUm9vdENBLmNydDBCBgNV\n" \
     "HR8EOzA5MDegNaAzhjFodHRwOi8vY3JsMy5kaWdpY2VydC5jb20vRGlnaUNlcnRH\n" \
     "bG9iYWxSb290Q0EuY3JsMD0GA1UdIAQ2MDQwCwYJYIZIAYb9bAIBMAcGBWeBDAEB\n" \
     "MAgGBmeBDAECATAIBgZngQwBAgIwCAYGZ4EMAQIDMA0GCSqGSIb3DQEBCwUAA4IB\n" \
     "AQCAMs5eC91uWg0Kr+HWhMvAjvqFcO3aXbMM9yt1QP6FCvrzMXi3cEsaiVi6gL3z\n" \
     "ax3pfs8LulicWdSQ0/1s/dCYbbdxglvPbQtaCdB73sRD2Cqk3p5BJl+7j5nL3a7h\n" \
     "qG+fh/50tx8bIKuxT8b1Z11dmzzp/2n3YWzW2fP9NsarA4h20ksudYbj/NhVfSbC\n" \
     "EXffPgK2fPOre3qGNm+499iTcc+G33Mw+nur7SpZyEKEOxEXGlLzyQ4UfaJbcme6\n" \
     "ce1XR2bFuAJKZTRei9AqPCCcUZlM51Ke92sRKw2Sfh3oius2FkOH6ipjv3U/697E\n" \
     "A7sKPPcw7+uvTPyLNhBzPvOk\n" \
     "-----END CERTIFICATE-----";

static HttpsOTAStatus_t otastatus;


// Set time via NTP, as required for x.509 validation
void setClock() {
  /*
    Note: Bundled Arduino lwip supports only ONE ntp server, 2nd and 3rd options are silently ignored
    see CONFIG_LWIP_DHCP_MAX_NTP_SERVERS define in ./tools/sdk/esp32/sdkconfig
  */
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");  // UTC

  Serial.print(F("Waiting for NTP time sync: "));
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    yield();
    delay(500);
    Serial.print(F("."));
    now = time(nullptr);
  }

  Serial.println(F(""));
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print(F("Current time: "));
  Serial.print(asctime(&timeinfo));
}

void HttpEvent(HttpEvent_t *event)
{
    switch(event->event_id) {
        case HTTP_EVENT_ERROR:
            Serial.println("Http Event Error");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            Serial.println("Http Event On Connected");
            break;
        case HTTP_EVENT_HEADER_SENT:
            Serial.println("Http Event Header Sent");
            break;
        case HTTP_EVENT_ON_HEADER:
            Serial.printf("Http Event On Header, key=%s, value=%s\n", event->header_key, event->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            break;
        case HTTP_EVENT_ON_FINISH:
            Serial.println("Http Event On Finish");
            break;
        case HTTP_EVENT_DISCONNECTED:
            Serial.println("Http Event Disconnected");
            break;
    }
}

void setup(){

    Serial.begin(115200);
    Serial.print("Attempting to connect to SSID: ");
    WiFi.begin(ssid, password);

    // attempt to connect to Wifi network:
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
    }

    Serial.print("Connected to ");
    Serial.println(ssid);
    setClock();
    
    HttpsOTA.onHttpEvent(HttpEvent);
    Serial.println("Starting OTA");
    HttpsOTA.begin(url, server_certificate); 

    Serial.println("Please Wait it takes some time ...");
}

void loop(){
    otastatus = HttpsOTA.status();
    if(otastatus == HTTPS_OTA_SUCCESS) { 
        Serial.println("Firmware written successfully. To reboot device, call API ESP.restart() or PUSH restart button on device");
    } else if(otastatus == HTTPS_OTA_FAIL) { 
        Serial.println("Firmware Upgrade Fail");
    }
    delay(1000);
}
