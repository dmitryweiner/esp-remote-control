#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>

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

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  Serial.print("Configuring access point...");
  /* You can remove the password parameter if you want the AP to be open. */
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
  delay(10);
}

/** Handle root or redirect to captive portal */
void handleRoot() {
  String response = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>Remote controle main page</title><style>body,html{height:100%;margin:0}#wrapper{min-height:100%}#steering{position:absolute;bottom:0;width:90%;height:40px;margin-bottom:10px}#power{position:absolute;right:0;width:40px;height:90%;-webkit-appearance:slider-vertical;margin-right:10px}#controls{position:absolute;left:35px;top:35px}input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;border:1px solid #000;height:40px;width:40px;border-radius:3px;background:#fff;cursor:pointer;margin-top:-14px;box-shadow:1px 1px 1px #000,0 0 1px #0d0d0d}input[type=range]::-moz-range-thumb{box-shadow:1px 1px 1px #000,0 0 1px #0d0d0d;border:1px solid #000;height:40px;width:40px;border-radius:3px;background:#fff;cursor:pointer}.container{display:block;position:relative;padding-left:35px;margin-top:35px;cursor:pointer;font-size:22px;-webkit-user-select:none;-moz-user-select:none;-ms-user-select:none;user-select:none}.container input{position:absolute;opacity:0;cursor:pointer}.checkmark{position:absolute;top:0;left:0;height:25px;width:25px;background-color:#eee}.container:hover input~.checkmark{background-color:#ccc}.container input:checked~.checkmark{background-color:#2196f3}.checkmark:after{content:'';position:absolute;display:none}.container input:checked~.checkmark:after{display:block}.container .checkmark:after{left:9px;top:5px;width:5px;height:10px;border:solid #fff;border-width:0 3px 3px 0;-webkit-transform:rotate(45deg);-ms-transform:rotate(45deg);transform:rotate(45deg)}</style><script type='text/javascript'>document.addEventListener('DOMContentLoaded',function(){var t={steering:0,power:0,headLights:0,rearLights:0},e={url:'/control',delay:500};!function n(){var s,o,a,i;s=t,o=function(t){console.log(t),setTimeout(n,e.delay)},a=function(t){console.warn(t)},(i=new XMLHttpRequest).open('POST',e.url),i.setRequestHeader('Content-Type','application/json'),i.send(JSON.stringify(s)),i.onreadystatechange=function(){4==this.readyState&&(200==this.status?o(this.responseText):a(this.statusText))}}()},!1)</script></head><body><div id='wrapper'><div id='controls'><label class='container'>Head lights <input type='checkbox'> <span class='checkmark'></span></label><label class='container'>Rear lights <input type='checkbox'> <span class='checkmark'></span></label></div><div id='sensors'></div><input id='steering' type='range' min='0' max='100' step='1' value='50'> <input id='power' orient='vertical' type='range' min='0' max='100' step='1' value='50'></div></body></html>";
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(response.length());
  server.send(200, "text/html", response);
  Serial.println("Root page served");
}

/** Wifi config page handler */
void handleControl() {
  String response = "{\"result\": \"success\"}";
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(response.length());
  server.send(200, "text/html", response);
  server.sendContent(response);
  Serial.println("Control page served");
}

void handleNotFound() {
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


