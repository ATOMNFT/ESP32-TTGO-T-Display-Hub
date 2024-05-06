#include <WiFi.h>
#include <TFT_eSPI.h>

#define BUTTON_A_PIN 35 // Define pin for button A
#define BUTTON_B_PIN 0  // Define pin for button B

TFT_eSPI tft = TFT_eSPI();
int currentAPIndex = 0; // Index of the currently displayed access point
int numNetworks = 0;    // Number of networks found
bool scanDone = false;  // Flag to indicate if scanning is done

void setup() {
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(1); // Adjust rotation if needed

  // Initialize buttons
  pinMode(BUTTON_A_PIN, INPUT);
  pinMode(BUTTON_B_PIN, INPUT);

  // Scan for Wi-Fi networks
  scanNetworks();
}

void loop() {
  if (!scanDone) {
    // Display scanning message while scanning
    displayScanningMessage();
    delay(500);
    return;
  }

  // Check button presses to switch between access points
  if (digitalRead(BUTTON_A_PIN) == LOW) {
    currentAPIndex--;
    if (currentAPIndex < 0) {
      currentAPIndex = numNetworks - 1;
    }
    displayAccessPointInfo(currentAPIndex);
    delay(500); // Button debounce delay
  }

  if (digitalRead(BUTTON_B_PIN) == LOW) {
    currentAPIndex++;
    if (currentAPIndex >= numNetworks) {
      currentAPIndex = 0;
    }
    displayAccessPointInfo(currentAPIndex);
    delay(500); // Button debounce delay
  }

  delay(100); // Small delay to prevent button press detection issues
}

void scanNetworks() {
  numNetworks = WiFi.scanNetworks();
  if (numNetworks == 0) {
    Serial.println("No networks found");
    tft.setTextColor(TFT_RED);
    tft.setTextSize(2);
    tft.setCursor(10, 10);
    tft.println("No networks found");
    scanDone = true;
  } else {
    Serial.println("Scanning for networks...");
    displayAccessPointInfo(currentAPIndex);
    scanDone = true;
  }
}

void displayScanningMessage() {
  tft.fillScreen(TFT_BLACK); // Clear screen before displaying message
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("Scanning...");
}

void displayAccessPointInfo(int index) {
  tft.fillScreen(TFT_BLACK); // Clear screen before displaying info
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.print("SSID: ");
  tft.println(WiFi.SSID(index));
  tft.print("RSSI: ");
  tft.println(WiFi.RSSI(index));
  tft.print("Channel: ");
  tft.println(WiFi.channel(index));
  tft.print("Encryption: ");
  tft.println(getEncryptionType(WiFi.encryptionType(index)));
}

String getEncryptionType(int encryptionType) {
  switch (encryptionType) {
    case WIFI_AUTH_OPEN:
      return "Open";
    case WIFI_AUTH_WEP:
      return "WEP";
    case WIFI_AUTH_WPA_PSK:
      return "WPA-PSK";
    case WIFI_AUTH_WPA2_PSK:
      return "WPA2-PSK";
    case WIFI_AUTH_WPA_WPA2_PSK:
      return "WPA/WPA2-PSK";
    default:
      return "Unknown";
  }
}
