#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <Servo.h>

#define MOTOR_PWM D1
#define MOTOR_DIR D3

#define DIR_BACK HIGH
#define DIR_FWD LOW

#define HEAD_LIGHTS D7
#define REAR_LIGHTS D8

#define MAX_DISCONNECT_TIME 5000 // 5 sec

/*
 * Now the ESP8266 is in your network. You can reach it through http://192.168.x.x/ (the IP you took note of) or maybe at http://esp8266.local too.
 * 
 * This is a captive portal because through the softAP it will redirect any http request to http://192.168.4.1/
 */

/* Set these to your desired softAP credentials. They are not configurable at runtime */
const char *softAP_ssid = "ESP_ap";

/* hostname for mDNS. Should work at least on windows. Try http://esp8266.local */
const char *myHostname = "esp8266";

// DNS server
const byte DNS_PORT = 53;
DNSServer dnsServer;

// Web server
ESP8266WebServer server(80);

/* Soft AP network parameters */
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

Servo servo;

int steering = 0;
int power = 0;
int token = 0;
int lastConnectTime = 0;

void setup() {
  lastConnectTime = millis();
  delay(1000);

  // set up hardware

  // light
  pinMode(HEAD_LIGHTS, OUTPUT);
  pinMode(REAR_LIGHTS, OUTPUT);
  digitalWrite(HEAD_LIGHTS, LOW);
  digitalWrite(REAR_LIGHTS, LOW);

  // motor
  pinMode(MOTOR_PWM, OUTPUT); // Motor pwm pin
  pinMode(MOTOR_DIR, OUTPUT); // Motor dir pin

  // steering
  servo.attach(2); // GPIO 2 = 4 on the board

  Serial.begin(115200);
  Serial.println();
  Serial.print("Configuring access point...");
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(softAP_ssid);
  delay(500); // Without delay I've seen the IP address blank
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  /* Setup the DNS server redirecting all the domains to the apIP */  
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);

  /* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
  server.on("/", handleRoot);
  server.on("/control", handleControl);
  server.on("/token", handleToken);
  server.onNotFound ( handleNotFound );
  server.begin(); // Web server start
  Serial.println("HTTP server started");
}

void loop() {
  // Do work:
  //DNS
  dnsServer.processNextRequest();
  //HTTP
  server.handleClient();

  // turn off motor when disconnected
  if (millis() - lastConnectTime > MAX_DISCONNECT_TIME) {
    power = 0;
  }
}

/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
boolean captivePortal() {
  if (!isIp(server.hostHeader()) && server.hostHeader() != (String(myHostname)+".local")) {
    Serial.print("Request redirected to captive portal");
    server.sendHeader("Location", String("http://") + toStringIp(server.client().localIP()), true);
    server.send ( 302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
    server.client().stop(); // Stop is needed because we sent no content length
    return true;
  }
  return false;
}

/** Handle root or redirect to captive portal */
void handleRoot() {
  if (captivePortal()) { // If caprive portal redirect instead of displaying the page.
    return;
  }
  String response = "INDEX"; // this string will be replaced
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(response.length());
  server.send(200, "text/html", response);
  Serial.println("Root page served");
}

void handleControl() {
  StaticJsonBuffer<200> newBuffer;
  int power;

  lastConnectTime = millis();

  String response = "{\"result\": \"success\"}";
  int responseCode = 200;
  JsonObject& json = newBuffer.parseObject(server.arg("plain"));

  json.printTo(Serial);

  if (json["token"] == NULL || json["token"] != token) {
      response = "{\"result\": \"token error\"}";
      responseCode = 401;
  } else {

    // control light
    if (json["headLights"] == 1) {
      digitalWrite(HEAD_LIGHTS, HIGH);
    } else {
      digitalWrite(HEAD_LIGHTS, LOW);
    }

    if (json["rearLights"] == 1) {
      digitalWrite(REAR_LIGHTS, HIGH);
    } else {
      digitalWrite(REAR_LIGHTS, LOW);
    }

    // steering
    if (steering != json["steering"]) {
      steering = json["steering"];
      servo.write(steering);
    }

    // control motor
    power = json["power"];
    if (power < 0) {
      digitalWrite(MOTOR_DIR, DIR_BACK);
      analogWrite(MOTOR_PWM, abs(power));
    } else {
      digitalWrite(MOTOR_DIR, DIR_FWD);
      analogWrite(MOTOR_PWM, power);
    }
  }

  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(response.length());
  server.send(responseCode, "text/html", response);
  server.sendContent(response);
  Serial.println("Control page served");
}

void handleToken() {
  StaticJsonBuffer<200> newBuffer;
  JsonObject& json = newBuffer.parseObject(server.arg("plain"));
  json.printTo(Serial);

  if (json["token"]) {
    token = json["token"];
  }

  String response = "";
  server.setContentLength(response.length());
  server.send(200, "text/html", response);
  server.sendContent(response);
  Serial.print("Token set to: ");
  Serial.println(token);
}

void handleNotFound() {
  if (captivePortal()) { // If caprive portal redirect instead of displaying the page.
    return;
  }
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send ( 404, "text/plain", message );
  Serial.println("404 page served");
}

/** Is this an IP? */
boolean isIp(String str) {
  for (int i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

/** IP to String? */
String toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}
