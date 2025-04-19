#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "YOUR-SSID";
const char* wifiPassword = "YOUR-WIFI-PASSWORD";
const char* minerIP = "http://YOURMINERIP/api/system/info";
const char* minerIP2 = "http://YOURMINERIP/api/system/info";// Second device
//const char* minerIP2 = "http://YOURMINERIP/api/system/info"; // another device and so on 
const String binID = "YOUR-BIN-ID"; // Your existing bin ID
const char* jsonBinURL = "https://api.jsonbin.io/v3/b/";
const char* apiKey = "YOUR JSONBIN.IO API KEY";  // Replace with your API Key

const char* minerID1 = "YOUR WORKER NAME 1";
const char* minerID2 = "YOUR WORKER NAME 2";
//const char* minerID2 = "YOUR WORKER NAME 3"; // another device and so on 

String obscureBTCAddress(const String& addr) {
  if (addr.length() > 8) {
    return addr.substring(0, 4) + "***" + addr.substring(addr.length() - 4);
  }
  return addr;
}

String obscureSSID(const String& ssid) {
  if (ssid.length() > 4) {
    return ssid.substring(0, 4) + "***";
  }
  return ssid;
}

String obscureMACaddr(String mac) {
  return mac.substring(0, 3) + ":**:**:**";
}

void sendToJsonBin(String payload) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient httpPut;
    String url = jsonBinURL + binID;
    httpPut.begin(url);
    httpPut.addHeader("Content-Type", "application/json");
    httpPut.addHeader("X-Master-Key", apiKey);
    
    int putResponse = httpPut.PUT(payload);
    Serial.print("JSONBin response: ");
    Serial.println(putResponse);
    httpPut.end();
  }
}

String fetchAndProcessData(const char* deviceIP) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(deviceIP);
    int httpCode = http.GET();
    if (httpCode > 0) {
      String payload = http.getString();
      StaticJsonDocument<2048> doc;
      DeserializationError error = deserializeJson(doc, payload);
      if (error) {
        Serial.println("JSON parsing error");
        http.end();
        return "{}";
      }
      JsonObject obj = doc.as<JsonObject>();
      for (JsonPair kv : obj) {
        const char* key = kv.key().c_str();
        if (String(key) == "stratumUser" || String(key) == "fallbackStratumUser") {
          kv.value().set(obscureBTCAddress(kv.value().as<String>()));
        } else if (String(key) == "ssid") {
          kv.value().set(obscureSSID(kv.value().as<String>()));
        } else if (String(key) == "macAddr") {
          kv.value().set(obscureMACaddr(kv.value().as<String>()));
        }
      }
      String jsonOutput;
      serializeJson(doc, jsonOutput);
      http.end();
      return jsonOutput;
    } else {
      Serial.println("Error: Device not responding");
      http.end();
      return "{}";
    }
  } else {
    Serial.println("Error: WiFi not connected");
    return "{}";
  }
}

void fetchMinerData() {
  String json1 = fetchAndProcessData(minerIP1);
  String json2 = fetchAndProcessData(minerIP2);
  
  StaticJsonDocument<4096> combinedDoc;
  combinedDoc["device1"]["Miner ID"] = minerID1;
  combinedDoc["device1"]["data"] = serialized(json1);
  combinedDoc["device2"]["Miner ID"] = minerID2;
  combinedDoc["device2"]["data"] = serialized(json2);
  //combinedDoc["device2"]["Miner ID"] = minerID3;
  //combinedDoc["device2"]["data"] = serialized(json3);
  
  String finalJson;
  serializeJson(combinedDoc, finalJson);
  sendToJsonBin(finalJson);
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, wifiPassword);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  fetchMinerData();
}

void loop() {
  delay(300000);
  fetchMinerData();
}

