#include <WiFi.h>
#include "secrets.h"



const char* ssid = "Merida";
const char* password = "Am6569853";

IPAddress local_IP(192, 168, 10, 1);
IPAddress gateway(192, 168, 10, 1);
IPAddress subnet(255, 255, 255, 0);

WiFiServer server(80);

const int ledPin = 2;


void setup() {
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  WiFi.softAPConfig(local_IP, gateway, subnet);

  WiFi.softAP(ssid, password);

  Serial.println("Access Point Started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  server.begin();
}

void loop() {
  WiFiClient client = server.available();

  if (client) {
    Serial.println("Client Connected");
    String request = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;
        if (c == '\n') break;
      }
    }

    Serial.println(request);

    if (request.indexOf("GET /led/on") != -1) {
      digitalWrite(ledPin, HIGH);
    } else if (request.indexOf("GET /led/off") != -1) {
      digitalWrite(ledPin, LOW);
    }

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println();
    client.println("<!DOCTYPE html>");
    client.println("<html>");
    client.println("<head><meta charset='UTF-8'><title>LED Control</title></head>");
    client.println("<body style='text-align:center; font-family:sans-serif;'>");
    client.println("<h2> Control Led with ESP32</h2>");
    client.println("<p><a href=\"/led/on\"><button style='padding:15px;'> Turn on LED</button></a></p>");
    client.println("<p><a href=\"/led/off\"><button style='padding:15px;'> Turn Off LED</button></a></p>");
    client.println("</body></html>");

    client.stop();
    Serial.println("Client Disconnected");
  }
}
