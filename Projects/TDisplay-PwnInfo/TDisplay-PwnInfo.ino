#include <TFT_eSPI.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <Button2.h>

const char *ssid = "YOUR_SIDD"; // Enter your WiFi network SSID here
const char *password = "YOUR_PASSWORD"; // Enter your WiFi network password here

// Define TFT display object
TFT_eSPI tft;

Button2 refreshButton(35); // GPIO pin for the refresh button
Button2 scrollButton(0);   // GPIO pin for the scroll button

int startIndex = 0; // Index for the first entry to display
const int totalEntries = 8; // Total number of entries
DynamicJsonDocument doc(2048); // JSON document to store the fetched data

void buttonScrollClickHandler(Button2 &btn) {
  startIndex = (startIndex + 4) % totalEntries; // Cycle through pages
  displayData();
}

void setup() {
  Serial.begin(115200);

  // Initialize TFT display
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);

  // Display connecting message
  tft.setCursor(10, 10);
  tft.println("Connecting to WiFi...");

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Display connected message with IP address
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(10, 10);
  tft.print("Connected to WiFi!");
  tft.setCursor(10, 40);
  tft.print("IP: ");
  tft.println(WiFi.localIP());

  refreshButton.setLongClickHandler([](Button2 &btn) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_RED);
    tft.setCursor(10, 10);
    tft.println("Refreshing...");
    delay(500); // Delay for visibility
    fetchData();
  });

  // Configure scrollButton
  scrollButton.setClickHandler(buttonScrollClickHandler); // Assign click handler
  scrollButton.setLongClickHandler([](Button2 &btn) {});  // Disable long click handler
  scrollButton.setDoubleClickHandler([](Button2 &btn) {}); // Disable double click handler

  fetchData(); // Initial data fetch and display
}

void loop() {
  refreshButton.loop();
  scrollButton.loop();
}

void fetchData() {
  HTTPClient http;

  // Make HTTP GET request
  http.begin("https://api.pwnagotchi.ai/api/v1/units/by_country"); // API for fetching pwny units by country

  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println(payload);

    // Parse JSON data
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      // Ensure we only use the first 8 results
      while (doc.size() > totalEntries) {
        doc.remove(doc.size() - 1);
      }

      startIndex = 0; // Reset startIndex when fetching new data
      displayData(); // Display the first set of results
    } else {
      Serial.println("Failed to parse JSON");
    }
  } else {
    Serial.println("Error in HTTP request");
  }

  http.end();
}

void displayData() {
  // Display data on TFT screen
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(10, 10);
  tft.println("Pwnies by Country");

  int y = 40; // Initial y position for the list
  for (int i = startIndex; i < startIndex + 4 && i < totalEntries; i++) {
    String country = doc[i]["country"];
    int units = doc[i]["units"];
    tft.setCursor(10, y);
    tft.print(i + 1);
    tft.print(". ");
    tft.print(country);
    tft.print(" - ");
    tft.println(units);
    y += 25; // Increment y position for next entry
  }
}