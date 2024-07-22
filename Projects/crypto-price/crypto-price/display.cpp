#include <TFT_eSPI.h>
#include "display.h"

// TFT Display
TFT_eSPI tft = TFT_eSPI(); // Initialize TFT library

void initializeDisplay() {
    tft.init();
    tft.setRotation(1); // Adjust rotation if needed
}

void displayCryptoInfo(float price_usd, float volume_1hrs_usd) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_YELLOW);
    tft.setTextSize(2);
    tft.setCursor(6, 10);
    tft.print("DOGE Price: ");
    tft.println(price_usd);
    tft.setCursor(6, 40);
    tft.println("1hr Vol USD: ");
    tft.setCursor(6, 60);
    tft.println(volume_1hrs_usd);
}
