#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h> // Install Arduino_JSON library via Library Manager
#include "display.h"

// WiFi credentials
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWRD";

// CoinAPI endpoint for fetching cryptocurrency data
const char* apiEndpoint = "https://rest.coinapi.io/v1/assets/DOGE"; 
// To change the asset visit the link and add your apikey 
// https://rest.coinapi.io/v1/assets/apikey-YOUR_API_KEY

// CoinAPI API key
const char* apiKey = "YOUR_API_KEY"; // https://www.coinapi.io/get-free-api-key?product_id=market-data-api

// Time interval for refreshing data (2 hours in milliseconds)
const unsigned long refreshInterval = 2 * 60 * 60 * 1000;

// Variables to keep track of the last refresh time
unsigned long lastRefreshTime = 0;

void setup() {
    // Initialize serial and TFT display
    Serial.begin(115200);
    initializeDisplay();

    // Connect to WiFi
    connectWiFi();

    // Fetch and display cryptocurrency data initially
    fetchAndDisplayCryptoData();
    lastRefreshTime = millis(); // Update the last refresh time
}

void loop() {
    // Check if it's time to refresh the data
    unsigned long currentTime = millis();
    if (currentTime - lastRefreshTime >= refreshInterval) {
        fetchAndDisplayCryptoData();
        lastRefreshTime = currentTime; // Update the last refresh time
    }
}

void connectWiFi() {
    // Connect to WiFi network
    Serial.print("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
}

void fetchAndDisplayCryptoData() {
    // Make HTTP GET request to CoinAPI
    HTTPClient http;
    http.begin(apiEndpoint);
    http.addHeader("X-CoinAPI-Key", apiKey);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
        String payload = http.getString();
        Serial.println(payload); // Print entire JSON response for debugging

        // Parse JSON response
        const size_t capacity = JSON_OBJECT_SIZE(12) + 256;
        StaticJsonDocument<capacity> doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            return;
        }

        // Extract price_usd and volume_1hrs_usd from JSON response
        float price_usd = doc[0]["price_usd"];
        float volume_1hrs_usd = doc[0]["volume_1hrs_usd"];
        
        Serial.print("Price USD received: ");
        Serial.println(price_usd); // Print price_usd to Serial Monitor for debugging
        Serial.print("Volume 1hr USD received: ");
        Serial.println(volume_1hrs_usd); // Print volume_1hrs_usd to Serial Monitor for debugging

        // Display crypto info on TFT
        displayCryptoInfo(price_usd, volume_1hrs_usd);

    } else {
        Serial.print("Error on HTTP request: ");
        Serial.println(httpResponseCode);
    }

    http.end(); // Free resources
}
