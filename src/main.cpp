#include <Arduino.h>
#include <U8g2lib.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

/* The following source file is not checked into github.  It contains
 * preprocessor definitions for my network like these:
#define MY_SSID "MyWirelessSSID"
#define MY_PSK  "Password for my wireleess network"
#define MY_HTTP_SERVER_IP "192.168.0.1"
#define MY_HTTP_SERVER_PORT 80
#define MY_HTTP_REQUEST "GET /rest/items/Temperature/state HTTP/1.1\r\n" \
                        "Host: servername\r\n"                           \
                        "Accept: text/plain\r\n"                         \
                        "Connection: close\r\n\r\n"
 */
#include "/etc/credentials/wireless/temperature_source.h"

#define DISPLAY_DATA_PIN         D7
#define DISPLAY_CLOCK_PIN        D5
#define DISPLAY_RESET_PIN        D6
#define DISPLAY_DATA_COMMAND_PIN D0
#define DISPLAY_CHIP_SELECT_PIN  D8
#define DISPLAY_NO_ROTATION U8G2_R0

U8G2_SSD1306_128X64_NONAME_1_4W_SW_SPI
  display(DISPLAY_NO_ROTATION,
          DISPLAY_CLOCK_PIN,
          DISPLAY_DATA_PIN,
          DISPLAY_CHIP_SELECT_PIN,
          DISPLAY_DATA_COMMAND_PIN,
          DISPLAY_RESET_PIN);

void setup_wifi() {
  // Set WiFi mode to station (client) and initiate connection
  WiFi.mode(WIFI_STA);
  WiFi.begin(MY_SSID, MY_PSK);
}

bool wifi_connected = false;
float temperature = -300;
int failed_attempts = 0;

void setup() {
  setup_wifi();
  Serial.begin(115200);
  display.begin();
}

float get_temp() {
  if (wifi_connected) {
    WiFiClient client;
    if (client.connect(MY_HTTP_SERVER_IP, MY_HTTP_SERVER_PORT)) {
      client.print(MY_HTTP_REQUEST);
      unsigned long request_time = millis();
      bool line_starts = true;
      bool last_line = false;
      String temp;
      while (millis() - request_time < 5000) {
        delay(1);
        while (client.available()) {
          char c = client.read();
          Serial.print(c);
          if (line_starts && (c == '-' || c == '.' || (c >= '0' && c <='9'))) {
            last_line = true;
          }
          line_starts = (c == '\r' || c == '\n');
          if (last_line)
            temp += String(c);
        }
        if (temp.length()) {
          Serial.println();
          return temp.toFloat();
        }
      }
    }
  }
  return -300.0f;
}

void update_display() {
  char str_connected[20] = {'\0'};
  String str_failed(failed_attempts);
  if (wifi_connected && failed_attempts == 0) {
    // Wireless connected and http working.
    // use sprintf to place an iso-8859-1 ß into the string Draußen that is
    // shown when we have no error.
    snprintf(str_connected, 20, "Drau%cen", 0xDF);
    str_failed = "";
  }
  else {
    // We have either a WLAN or TCP error.
    snprintf(str_connected, 20, "%s Error %d",
             wifi_connected ? "TCP" : "WLAN", failed_attempts);
  }
  char str_temperature[20] = {'\0'};

  // stringify the temperature with one place after decimal point.
  snprintf(str_temperature, 20, "%.1f%cC", temperature, 0xb0);

  // debug: print every info to serial.
  Serial.println(str_connected);
  Serial.println(str_temperature);
  Serial.println(str_failed);

  // print the same info to the display.
  display.firstPage();
  do {
    display.setFont(u8g2_font_helvB14_tf);//u8g2_font_inb16_mf);
    display.drawStr(0,15,str_connected);
    display.drawStr(100,64,str_failed.c_str());
    display.setFont(u8g2_font_logisoso32_tf);
    display.drawStr(0,52,str_temperature);
  } while ( display.nextPage() );
}

void loop() {
  // update the display
  update_display();

  // wait some time
  delay(5000);

  // update the wifi and temperature reading
  wifi_connected = WiFi.status() == WL_CONNECTED;
  float new_temperature = get_temp();

  // check if temperature is valid
  if (new_temperature > -280) {
    temperature = new_temperature;
    failed_attempts = 0;
  }
  else {
    failed_attempts++;
  }
}
