# ESP remote control

Controlling a vehicle with ESP8266 chip via WiFi.

This chip provides a [captive portal](https://www.hackster.io/rayburne/esp8266-captive-portal-5798ff) demonstrating basic controls.

## Prerequisites

* Arduino IDE
* Node.js
* Node.js package manager (npm)

## Before you start

Install node dependencies:

```bash
npm install
```

## Compiling the project

To compile the arduino project, run:

```bash
node ./node_modules/gulp/bin/gulp.js
```

The resulting project will be in  `dist/server/server.ino`.
