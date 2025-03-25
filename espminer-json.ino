#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "YOUR-SSID";
const char* wifiPassword = "YOUR-WIFI-PASSWORD";
const char* minerIP = "http://YOURMINERIP/api/system/info";
const String binID = "YOUR-BIN-ID"; // Your existing bin ID
const char* jsonBinURL = "https://api.jsonbin.io/v3/b/";  // URL for JSONBin.io API

const char* apiKey = "YOUR JSONBIN.IO API KEY";  // Replace with your API Key

// Function to obscure BTC address
String obscureBTCAddress(const String& addr) {
  if (addr.length() > 8) {
    return addr.substring(0, 4) + "***" + addr.substring(addr.length() - 4);
  }
  return addr;
}

// Function to obscure SSID
String obscureSSID(const String& ssid) {
  if (ssid.length() > 4) {
    return ssid.substring(0, 4) + "***";
  }
  return ssid;
}

// Function to obfuscate MAC address
String obscureMACaddr(String mac) {
  // You can apply any logic here to obfuscate the MAC address
  // For simplicity, let's just return a partial obfuscation as an example
  String obscuredMAC = mac.substring(0, 3) + ":**:**:**";  // Example: obfuscate the middle part of the MAC address
  return obscuredMAC;
}

// Function to send data to JSONBin
void sendToJsonBin(String payload) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient httpPut;
    String url = jsonBinURL + binID;  // Append the bin ID to the URL
    httpPut.begin(url);
    httpPut.addHeader("Content-Type", "application/json");
    httpPut.addHeader("X-Master-Key", apiKey);

    int putResponse = httpPut.PUT(payload);  // Use PUT request to update the existing bin
    Serial.print("JSONBin response: ");
    Serial.println(putResponse);
    httpPut.end();
  }
}

// Function to fetch miner data and send it to JSONBin
void fetchMinerData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(minerIP);
    int httpCode = http.GET();
    if (httpCode > 0) {
      String payload = http.getString();
      StaticJsonDocument<2048> doc;
      DeserializationError error = deserializeJson(doc, payload);
      if (error) {
        Serial.println("JSON parsing error");
        http.end();
        return;
      }

// Iterate through JSON data and apply obfuscation to specific fields
JsonObject obj = doc.as<JsonObject>();
for (JsonPair kv : obj) {
  const char* key = kv.key().c_str();
  
  // Obfuscate stratumUser, fallbackStratumUser, and ssid fields
  if (String(key) == "stratumUser") {
    kv.value().set(obscureBTCAddress(kv.value().as<String>()));  // Obfuscate stratumUser
  } else if (String(key) == "fallbackStratumUser") {
    kv.value().set(obscureBTCAddress(kv.value().as<String>()));  // Obfuscate fallbackStratumUser
  } else if (String(key) == "ssid") {
    kv.value().set(obscureSSID(kv.value().as<String>()));  // Obfuscate SSID
  } else if (String(key) == "macAddr") {
    kv.value().set(obscureMACaddr(kv.value().as<String>()));  // Obfuscate MAC address
  }
}


      // Serialize modified data for sending to JSONBin
      String jsonOutput;
      serializeJson(doc, jsonOutput);

      // Send the data to the remote JSONBin
      sendToJsonBin(jsonOutput);
    } else {
      Serial.println("Error: Miner not responding");
    }
    http.end();
  } else {
    Serial.println("Error: WiFi not connected");
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, wifiPassword);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");

  // Call function to get miner data
  fetchMinerData();
}

void loop() {
  // Repeat every 10 minutes (600,000 milliseconds)
  delay(600000);
  fetchMinerData();
}
