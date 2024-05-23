// ###################
// Created by ATOMNFT
// 
/* FLASH SETTINGS
Board: LOLIN D32
Flash Frequency: 80MHz
Partition Scheme: Minimal SPIFFS
*/
//
// ###################

#include <WiFi.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <Button2.h>

// Define the TFT display object
TFT_eSPI tft = TFT_eSPI();

// Define buttons connected to GPIO 0 and GPIO 35
Button2 buttonScroll(0);
Button2 buttonRescan(35);

// Timer variables
unsigned long previousMillis = 0; 
const long interval = 40000; // 40 seconds
unsigned long startupMillis = 0; 
const long startupDisplayTime = 2000; // 2 seconds

// WiFi scan results
int numNetworks = 0;
String ssids[50];
int rssis[50];
int currentPage = 0;
const int networksPerPage = 2;

void setup() {
  // Initialize the display
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  
  // Display startup message
  tft.setCursor(0, tft.height() / 2 - 10);
  tft.setTextColor(TFT_VIOLET, TFT_BLACK);
  tft.println("WiFi-Scanner v0.0.1");
  startupMillis = millis();
  
  // Initialize WiFi
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  // Initialize buttons
  buttonScroll.setClickHandler(buttonScrollClickHandler);
  buttonRescan.setClickHandler(buttonRescanClickHandler);
  
  // Set debounce time for buttons (in milliseconds)
  buttonScroll.setDebounceTime(50); // Adjust debounce time as needed
  buttonRescan.setDebounceTime(50);
}

void loop() {
  buttonScroll.loop();
  buttonRescan.loop();
  unsigned long currentMillis = millis();
  
  // Check if startup display time has passed
  if (currentMillis - startupMillis >= startupDisplayTime) {
    // Perform the initial scan only once
    if (previousMillis == 0) {
      performScan();
      previousMillis = currentMillis; // Initialize previousMillis after the first scan
    }
    
    // Rescan every interval
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      performScan();
    }
  }
}

void performScan() {
  // Display "Scanning..." message
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, tft.height() / 2 - 10);
  tft.setTextSize(2);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.println("Scanning...");
  
  // Perform WiFi scan
  numNetworks = WiFi.scanNetworks();
  
  // Store the scan results
  for (int i = 0; i < numNetworks; i++) {
    ssids[i] = WiFi.SSID(i);
    rssis[i] = WiFi.RSSI(i);
  }
  
  currentPage = 0; // Reset to the first page
  displayPage();
}

void displayPage() {
  // Clear the display
  tft.fillScreen(TFT_BLACK);
  
  if (numNetworks == 0) {
    tft.setCursor(0, 0);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("No WiFi networks found");
  } else {
    tft.setCursor(0, 0);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.printf("%d network(s) found:\n", numNetworks);
    
    // Display information for networks on the current page
    for (int i = 0; i < networksPerPage; ++i) {
      int index = currentPage * networksPerPage + i;
      if (index >= numNetworks) {
        break;
      }
      
      int ypos = (i + 1) * 40; // Space out the results by 40 pixels
      
      // Alternate colors for better readability
      if (index % 2 == 0) {
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      } else {
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
      }
      
      tft.setCursor(0, ypos);
      wrapText(String(index + 1) + ": " + ssids[index], 0, ypos, 240);
      
      tft.setCursor(0, ypos + 20);
      tft.printf("   RSSI: %d dBm\n", rssis[index]);
    }
  }
}

void wrapText(const String& text, int x, int y, int maxWidth) {
  int len = text.length();
  int width = 0;
  int start = 0;
  
  tft.setCursor(x, y);
  
  for (int i = 0; i < len; i++) {
    width += tft.textWidth(text.substring(i, i + 1));
    
    if (width > maxWidth) {
      tft.print(text.substring(start, i));
      start = i;
      width = tft.textWidth(text.substring(i, i + 1));
      y += 20; // Move to the next line
      tft.setCursor(x, y);
    }
  }
  tft.print(text.substring(start, len));
}

void buttonScrollClickHandler(Button2& btn) {
  if (numNetworks > networksPerPage) {
    currentPage++;
    if (currentPage * networksPerPage >= numNetworks) {
      currentPage = 0; // Wrap around to the first page
    }
    displayPage();
  }
}

void buttonRescanClickHandler(Button2& btn) {
  performScan();
}
