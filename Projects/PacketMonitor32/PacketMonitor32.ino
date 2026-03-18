/*
  PacketMonitor32 - TFT_eSPI version
  Original Version by https://github.com/spacehuhn/PacketMonitor32
  ----------------------------------------------------------------
  Converted for TFT_eSPI display output while keeping the original
  Wi-Fi packet monitor / PCAP logging behavior.

  Tuned for LilyGO TTGO T-Display style boards.

  Controls
  --------
  - GPIO 35 short press  -> next Wi-Fi channel
  - GPIO 35 long hold    -> deep sleep
  - GPIO 0 short press   -> toggle SD logging on/off
  - GPIO 0 double tap    -> toggle Graph page / Stats page

  Notes
  -----
  - This version uses an external SPI SD module, not SD_MMC.
  - SD wiring used here:
      CS   -> GPIO 33
      MOSI -> GPIO 26
      MISO -> GPIO 36
      SCK  -> GPIO 25
*/

#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <string>
#include <cstddef>
#include <Wire.h>
#include <Preferences.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <SD.h>
#include "FS.h"
#include "Buffer.h"

using namespace std;

/* ===== compile settings ===== */
#define MAX_CH 13
#define SNAP_LEN 2324

// Main control button: short press changes channel, long hold sleeps device.
#define BUTTON_CH_PIN 35

// Secondary control button: short press toggles SD logging, double tap swaps pages.
#define BUTTON_SD_PIN 0

// Display settings for the TTGO T-Display style panel.
#define TFT_ROTATION 1
#define TFT_BL 4

// External SPI SD module wiring.
#define SD_CS    33
#define SD_MOSI  26
#define SD_MISO  36
#define SD_SCK   25

// Button timing values.
#define CH_HOLD_MS          2000
#define SD_DOUBLE_TAP_MS     350
#define SLEEP_RELEASE_MS     150

#if CONFIG_FREERTOS_UNICORE
#define RUNNING_CORE 0
#else
#define RUNNING_CORE 1
#endif

/* ===== forward declarations ===== */
esp_err_t event_handler(void* ctx, system_event_t* event);
void wifi_promiscuous(void* buf, wifi_promiscuous_pkt_type_t type);
void coreTask(void* p);
double getMultiplicator();
void setChannel(int newChannel);
bool setupSD();
void draw();
void drawGraphPage();
void drawStatsPage();
void drawStatRow(int y, const char* label, const String& value);
void enterDeepSleep();
void toggleSDLogging();

/* ===== globals ===== */
Buffer sdBuffer;
TFT_eSPI tft = TFT_eSPI();
SPIClass sdSPI(HSPI);
Preferences preferences;

// SD state.
bool useSD = false;
bool sdReady = false;

// GPIO 35 button state tracking for short press vs long hold.
bool chButtonDown = false;
bool chActionTaken = false;
uint32_t chPressStart = 0;

// GPIO 0 button state tracking for single tap vs double tap.
bool sdButtonLast = HIGH;
bool sdTapPending = false;
uint32_t sdFirstTapTime = 0;

// Live counters and UI state.
uint32_t lastDrawTime = 0;
uint32_t tmpPacketCounter = 0;
uint32_t deauths = 0;
uint32_t totalPackets = 0;
uint32_t totalDeauths = 0;
uint32_t packetsWritten = 0;
uint32_t sessionStartMs = 0;
unsigned int ch = 1;
int rssiSum = 0;
int lastRssi = 0;
bool showStatsPage = false;

// Screen layout values. These are refreshed after TFT init.
int SCREEN_W = 135;
int SCREEN_H = 240;
int HEADER_H = 18;
int GRAPH_TOP = 20;
int GRAPH_H = 219;
int GRAPH_W = 135;

// Packet history buffer used by the scrolling graph.
uint32_t pkts[240] = {0};

esp_err_t event_handler(void* ctx, system_event_t* event) {
  return ESP_OK;
}

/* ===== helpers ===== */

// Finds a scale factor so the graph fits the current screen height.
double getMultiplicator() {
  uint32_t maxVal = 1;
  for (int i = 0; i < GRAPH_W; i++) {
    if (pkts[i] > maxVal) maxVal = pkts[i];
  }

  if (maxVal > (uint32_t)GRAPH_H) return (double)GRAPH_H / (double)maxVal;
  return 1.0;
}

// Changes channel, saves it to preferences, and safely reapplies promiscuous mode.
void setChannel(int newChannel) {
  ch = newChannel;
  if (ch > MAX_CH || ch < 1) ch = 1;

  preferences.begin("packetmonitor32", false);
  preferences.putUInt("channel", ch);
  preferences.end();

  esp_wifi_set_promiscuous(false);
  esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous_rx_cb(&wifi_promiscuous);
  esp_wifi_set_promiscuous(true);
}

// Initializes the external SPI SD card module.
bool setupSD() {
  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, sdSPI)) {
    Serial.println("Card Mount Failed");
    sdReady = false;
    return false;
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  sdReady = true;
  return true;
}

// Toggles packet logging to SD on or off.
void toggleSDLogging() {
  if (!useSD) {
    if (setupSD()) {
      sdBuffer.open(&SD);
      useSD = true;
      packetsWritten = 0;
      Serial.println("SD logging ON");
    } else {
      useSD = false;
      Serial.println("SD mount failed");
    }
  } else {
    sdBuffer.close(&SD);
    useSD = false;
    Serial.println("SD logging OFF");
  }

  draw();
}

// Handles the full sleep sequence so the board sleeps cleanly.
void enterDeepSleep() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(20, SCREEN_H / 2 - 8);
  tft.println("Sleeping...");
  delay(250);

  // Close any active capture file before sleeping.
  if (useSD) {
    sdBuffer.close(&SD);
    useSD = false;
  }

  // Stop packet sniffing before sleep.
  esp_wifi_set_promiscuous(false);

  // Wait until the button is released so the board does not instantly wake/reboot.
  uint32_t releaseStart = millis();
  while (digitalRead(BUTTON_CH_PIN) == LOW && (millis() - releaseStart < 3000)) {
    delay(10);
  }
  delay(SLEEP_RELEASE_MS);

  // Turn off the backlight right before entering deep sleep.
  digitalWrite(TFT_BL, LOW);

  // Wake when GPIO 35 is pulled LOW again.
  esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_CH_PIN, 0);
  esp_deep_sleep_start();
}

// Promiscuous callback: counts packets, deauths, RSSI, and optionally logs packets.
void wifi_promiscuous(void* buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  wifi_pkt_rx_ctrl_t ctrl = pkt->rx_ctrl;

  if (type == WIFI_PKT_MGMT && (pkt->payload[0] == 0xA0 || pkt->payload[0] == 0xC0)) {
    deauths++;
    totalDeauths++;
  }

  if (type == WIFI_PKT_MISC) return;
  if (ctrl.sig_len > SNAP_LEN) return;

  uint32_t packetLength = ctrl.sig_len;
  if (type == WIFI_PKT_MGMT && packetLength >= 4) packetLength -= 4;

  tmpPacketCounter++;
  totalPackets++;
  rssiSum += ctrl.rssi;

  if (useSD) {
    sdBuffer.addPacket(pkt->payload, packetLength);
    packetsWritten++;
  }
}

// Draws the main live graph page.
void drawGraphPage() {
  double multiplicator = getMultiplicator();
  int rssi = 0;

  if (pkts[GRAPH_W - 1] > 0) rssi = rssiSum / (int)pkts[GRAPH_W - 1];
  else rssi = lastRssi;

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.printf("CH:%u RSSI:%d PKT:%lu D:%lu %s",
             ch,
             rssi,
             (unsigned long)tmpPacketCounter,
             (unsigned long)deauths,
             useSD ? "SD" : "");

  tft.drawFastHLine(0, HEADER_H, SCREEN_W, TFT_DARKGREY);

  // Draw each packet bar and shift older samples left for the scrolling effect.
  for (int i = 0; i < GRAPH_W; i++) {
    int len = (int)(pkts[i] * multiplicator);
    if (len > GRAPH_H) len = GRAPH_H;

    int y1 = SCREEN_H - 1 - len;
    tft.drawFastVLine(i, y1, len > 0 ? len : 1, TFT_GREEN);

    if (i < GRAPH_W - 1) pkts[i] = pkts[i + 1];
  }
}

// Helper used by the stats page to draw boxed rows with a label on the left and value on the right.
void drawStatRow(int y, const char* label, const String& value) {
  const int rowX = 4;
  const int rowW = SCREEN_W - 8;
  const int rowH = 14;              // slightly shorter so more rows fit
  const int labelX = rowX + 5;
  const int textY = y + 3;
  const int valueRightX = rowX + rowW - 5;

  // Draw the row box.
  tft.drawRoundRect(rowX, y, rowW, rowH, 3, TFT_DARKGREY);

  // Draw the label on the left.
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(labelX, textY);
  tft.print(label);

  // Right-align the value text inside the box.
  int valueW = tft.textWidth(value);
  int valueX = valueRightX - valueW;

  // Prevent overlap into the label area.
  if (valueX < rowX + 72) valueX = rowX + 72;

  tft.setCursor(valueX, textY);
  tft.print(value);
}

// Draws the secondary stats page with boxed info rows.
void drawStatsPage() {
  tft.fillScreen(TFT_BLACK);

  // Header
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(4, 4);
  tft.println("PacketMonitor32 Stats");

  // Build derived stats from values already tracked in the sketch.
  uint32_t uptimeSec = millis() / 1000;
  uint32_t avgPps = uptimeSec > 0 ? totalPackets / uptimeSec : totalPackets;

  // Start rows below the header and keep spacing tight.
  int y = 20;
  drawStatRow(y, "Channel", String(ch));               y += 16;
  drawStatRow(y, "Packets", String(tmpPacketCounter)); y += 16;
  drawStatRow(y, "Total", String(totalPackets));       y += 16;
  drawStatRow(y, "Deauth", String(totalDeauths));      y += 16;
  drawStatRow(y, "Avg PPS", String(avgPps));           y += 16;
  drawStatRow(y, "RSSI", String(lastRssi));            y += 16;
  drawStatRow(y, "Written", String(packetsWritten));   y += 16;

  // Footer hint
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setCursor(10, 220);
  tft.println("B0 dbl = page");
}

// Draws whichever page is currently active.
void draw() {
  if (showStatsPage) drawStatsPage();
  else drawGraphPage();
}

/* ===== main program ===== */
void setup() {
  Serial.begin(115200);
  Serial.println("BOOTED");

  // Used for uptime on the stats page.
  sessionStartMs = millis();

  // Restore the last saved channel.
  preferences.begin("packetmonitor32", false);
  ch = preferences.getUInt("channel", 1);
  preferences.end();

  // Initialize Wi-Fi in NULL mode so promiscuous sniffing can be enabled.
  nvs_flash_init();
  tcpip_adapter_init();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
  ESP_ERROR_CHECK(esp_wifi_start());
  esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);

  // Start with SD logging disabled until the user toggles it on.
  sdBuffer = Buffer();
  useSD = false;
  sdReady = false;

  // GPIO 35 is input-only, so use INPUT. GPIO 0 can use the internal pull-up.
  pinMode(BUTTON_CH_PIN, INPUT);
  pinMode(BUTTON_SD_PIN, INPUT_PULLUP);

  // Turn on the TFT backlight before drawing.
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // Initialize the TFT and derive the actual screen layout from the active setup.
  tft.init();
  tft.setRotation(TFT_ROTATION);
  SCREEN_W = tft.width();
  SCREEN_H = tft.height();
  HEADER_H = 18;
  GRAPH_TOP = HEADER_H + 2;
  GRAPH_H = SCREEN_H - GRAPH_TOP;
  GRAPH_W = SCREEN_W;
  if (GRAPH_W > 240) GRAPH_W = 240;

  // Small boot splash.
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(8, 10);
  tft.println("PacketMonitor32");
  tft.setCursor(8, 28);
  tft.println("Graph + Stats");
  delay(800);

  // Run the main input / draw task on the selected core.
  xTaskCreatePinnedToCore(
    coreTask,
    "coreTask",
    4096,
    NULL,
    0,
    NULL,
    RUNNING_CORE
  );

  // Start packet sniffing.
  esp_wifi_set_promiscuous_rx_cb(&wifi_promiscuous);
  esp_wifi_set_promiscuous(true);
}

void loop() {
  // Nothing is done here because the worker task handles input and drawing.
  vTaskDelay(portMAX_DELAY);
}

void coreTask(void* p) {
  while (true) {
    uint32_t currentTime = millis();

    // GPIO 35: short press = next channel, long hold = sleep.
    int chState = digitalRead(BUTTON_CH_PIN);
    if (chState == LOW) {
      if (!chButtonDown) {
        chButtonDown = true;
        chActionTaken = false;
        chPressStart = currentTime;
      } else if (!chActionTaken && (currentTime - chPressStart >= CH_HOLD_MS)) {
        chActionTaken = true;
        enterDeepSleep();
      }
    } else {
      if (chButtonDown && !chActionTaken) {
        setChannel(ch + 1);
        draw();
      }
      chButtonDown = false;
      chActionTaken = false;
    }

    // GPIO 0: single tap = SD toggle, double tap = page swap.
    int sdState = digitalRead(BUTTON_SD_PIN);
    if (sdButtonLast == LOW && sdState == HIGH) {
      // Release event: decide whether this is the first tap or the second tap.
      if (sdTapPending && (currentTime - sdFirstTapTime <= SD_DOUBLE_TAP_MS)) {
        showStatsPage = !showStatsPage;
        sdTapPending = false;
        draw();
        Serial.println(showStatsPage ? "Stats page ON" : "Graph page ON");
      } else {
        sdTapPending = true;
        sdFirstTapTime = currentTime;
      }
    }
    sdButtonLast = sdState;

    // If a second tap never came in time, treat it as a normal single tap.
    if (sdTapPending && (currentTime - sdFirstTapTime > SD_DOUBLE_TAP_MS)) {
      sdTapPending = false;
      toggleSDLogging();
    }

    // Periodically flush buffered packet data while logging is active.
    if (useSD) sdBuffer.save(&SD);

    // Refresh the display once per second and push the newest packet count into the graph.
    if (currentTime - lastDrawTime > 1000) {
      lastDrawTime = currentTime;
      pkts[GRAPH_W - 1] = tmpPacketCounter;
      lastRssi = (tmpPacketCounter > 0) ? (rssiSum / (int)tmpPacketCounter) : lastRssi;
      draw();
      Serial.println((String)pkts[GRAPH_W - 1]);
      tmpPacketCounter = 0;
      deauths = 0;
      rssiSum = 0;
    }

    // Optional serial control: type a channel number in Serial Monitor to jump channels.
    if (Serial.available()) {
      ch = Serial.readString().toInt();
      if (ch < 1 || ch > 14) ch = 1;
      setChannel(ch);
      draw();
    }

    delay(1);
  }
}
