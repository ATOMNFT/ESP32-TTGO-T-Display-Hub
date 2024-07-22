#ifndef DISPLAY_H
#define DISPLAY_H

#include <TFT_eSPI.h>

// TFT Display
extern TFT_eSPI tft;

// Function prototypes
void initializeDisplay();
void displayCryptoInfo(float price_usd, float volume_1hrs_usd);

#endif // DISPLAY_H
