/*
 *  This sketch demonstrates how to set up a simple HTTP-like server.
 *  The server will set a GPIO pin depending on the request
 *    http://server_ip/gpio/0 will set the GPIO2 low,
 *    http://server_ip/gpio/1 will set the GPIO2 high
 *  server_ip is the IP address of the ESP8266 module, will be 
 *  printed to Serial when the module is connected.
 */

#include <ESP8266WiFi.h>

const char* ssid = "SHomeNet";
const char* password = "pass0passx";

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);
WiFiClient client;

enum States {INIT, READY};
States state = INIT;

String buildResponseText(String response) {
  String s = "HTTP/1.1 200 OK\r\n\r\n{\"result\": \"";
  s += response;
  s += "\"}\n";
  return s;
}

String buildErrorResponseText(String reason) {
  String s = "HTTP/1.1 500 OK\r\n\r\n{\"result\": \"";
  s += reason;
  s += "\"}\n";
  return s;
}


String processCommand(String command) {
  switch (state) {
    case INIT:
      return buildErrorResponseText("NOT_READY");
    case READY:
      Serial.println(command);
      String result = Serial.readStringUntil('\n');
      result.replace("\r", "");
      if (result.length() > 0) {
        return buildResponseText(result);
      } else {
        return buildErrorResponseText("CONTROLLER_RESPONSE_TIMEOUT");
      }
  }
}

void parseRequest(String &request) {
  request.replace("GET", "");
  request.replace("HTTP/1.1", "");
  request.replace("/", "");
  request.trim();
}

void serialFlush(){
  while(Serial.available() > 0) {
    char t = Serial.read();
  }
}

void sendToServer(String data) {
  if (client.connect("192.168.1.2", 10001)) {
      client.println("GET /" + data + " HTTP/1.1");
      client.println("Host: 192.168.1.2");
      client.println("Connection: close");
      client.println();
      Serial.println("ok");
  }
}

void setup() {
  Serial.begin(19200);
  Serial.setTimeout(100);
  delay(10);

  // prepare GPIO2
  pinMode(2, OUTPUT);
  digitalWrite(2, 0);
  
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());
}


void loop() {
  if (state == INIT) {
    Serial.println("ready");
    String response = Serial.readStringUntil('\n');
    response.replace("\r", "");
    if (response != NULL && response.equals("ok")) {
      state = READY;
      delay(1000);
      serialFlush();
    }
  }

  // Read serial data
  String ctrlrPushStr = Serial.readStringUntil('\n');
  ctrlrPushStr.replace("\r", "");
  if (ctrlrPushStr != NULL) {
    sendToServer(ctrlrPushStr);
  }
  serialFlush();

  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  
  // Wait until the client sends some data
  while(!client.available()){
    delay(1);
  }
  
  // Read the first line of the request
  String req = client.readStringUntil('\r');
  parseRequest(req);
  String result = processCommand(req);
  client.flush();

  client.print(result);
  delay(1);
}

