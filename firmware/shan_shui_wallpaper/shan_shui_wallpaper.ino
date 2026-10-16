// Standalone shan shui wallpaper for the Waveshare ESP32-S3-RLCD-4.2.
// Flashing this replaces whatever is already on the board.
//
// The board paints the landscape itself: a C++ port of shan-shui-inf runs on
// core 0 and fills a strip of columns in PSRAM ahead of the view; core 1
// pans the view across the strip. No computer is needed after flashing.
// Every power-on starts a new painting. USB serial carries plain-text logs.

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <esp_task_wdt.h>
#include <freertos/semphr.h>

#include "config.h"
#include "st7305.h"
#include "strip.h"
#include "src/shanshui/world.h"

// Columns painted per generator step, and samples per pixel for antialiasing.
static const int kTileCols = 80;
static const int kSubsample = 2;
// Keep this many painted columns ready right of the view.
static const uint32_t kAheadCols = 480;
// The painter works in float; far from the origin its coordinates coarsen.
// Start a new painting (new seed) after this many columns: about 5 hours at
// the fastest pan, a day at the default.
static const uint32_t kColsPerPainting = 400000;

static Strip strip;
static SemaphoreHandle_t strip_lock;
static uint8_t* fb = nullptr;
static bool have_frame = false;
static uint16_t speed = SS_SCROLL_DEFAULT;
static volatile uint32_t view_x = 0;
static uint32_t next_scroll = 0;
static uint32_t stalls = 0;
static const uint16_t kSpeeds[] = {2, 4, 6, 8, 12, 18, 24};
static const int kSpeedCount = sizeof(kSpeeds) / sizeof(kSpeeds[0]);
static int speed_i = 2;
static char seed_text[24];

struct SsButton {
  int pin;
  int stable;
  int read;
  uint32_t change;
};

static SsButton key_btn = {PIN_KEY, 1, 1, 0};
static SsButton boot_btn = {PIN_BOOT, 1, 1, 0};

static const char* reset_reason_name(esp_reset_reason_t r) {
  static const char* names[] = {"unknown", "power-on", "external", "software", "panic",
                                "int-watchdog", "task-watchdog", "watchdog", "deep-sleep",
                                "brownout", "sdio", "usb", "jtag", "efuse", "pwr-glitch",
                                "cpu-lockup"};
  int i = (int)r;
  return i >= 0 && i < (int)(sizeof names / sizeof names[0]) ? names[i] : "other";
}

// ------------------------------------------------------------ painter (core 0)

static void painter_task(void*) {
  static ss::World world;
  if (!world.init(kTileCols, PANEL_H, kSubsample)) {
    Serial.println("painter: out of memory for the raster");
    vTaskDelete(nullptr);
  }
  world.start(seed_text);
  world.set_clock([]() -> uint32_t { return (uint32_t)millis(); });
  uint8_t* packed = (uint8_t*)ps_malloc((size_t)kTileCols * SS_COL_STRIDE);
  uint32_t next_col = 0;
  uint32_t base_col = 0;
  uint32_t painted = 0;
  uint32_t busy_ms = 0;
  for (;;) {
    xSemaphoreTake(strip_lock, portMAX_DELAY);
    uint32_t filled = strip.start_x + strip.count;
    uint16_t room = strip_free(&strip);
    xSemaphoreGive(strip_lock);
    if (room < kTileCols || filled >= view_x + PANEL_W + kAheadCols) {
      vTaskDelay(pdMS_TO_TICKS(20));
      continue;
    }
    if (next_col - base_col >= kColsPerPainting) {
      snprintf(seed_text, sizeof seed_text, "%08lx", (unsigned long)esp_random());
      world.start(seed_text);
      base_col = next_col;
      Serial.printf("new painting at column %lu, seed %s\n", (unsigned long)next_col, seed_text);
    }
    uint32_t t0 = millis();
    world.render(next_col - base_col, packed);
    busy_ms += millis() - t0;
    xSemaphoreTake(strip_lock, portMAX_DELAY);
    strip_append(&strip, packed, kTileCols);
    xSemaphoreGive(strip_lock);
    next_col += kTileCols;
    painted += kTileCols;
    if (painted % 400 == 0) {
      Serial.printf("painted to column %lu, %.1f cols/s (gen %lu ms, raster %lu, dither %lu), heap %u KB, psram %u KB free\n",
                    (unsigned long)next_col, painted * 1000.0f / (busy_ms ? busy_ms : 1),
                    (unsigned long)world.t_gen, (unsigned long)world.t_raster, (unsigned long)world.t_post,
                    (unsigned)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024),
                    (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));
    }
  }
}

// ------------------------------------------------------------ view (core 1)

static uint32_t scroll_interval() {
  uint16_t sp = speed < 1 ? 1 : speed;
  return 1000UL / sp;
}

static void show_border() {
  if (!fb) return;
  st7305_fb_clear(fb);
  st7305_fb_rect(fb, 2, 2, PANEL_W - 3, PANEL_H - 3, false);
  st7305_fb_rect(fb, 6, 6, PANEL_W - 7, PANEL_H - 7, false);
  st7305_flush(fb);
}

static void show_view() {
  xSemaphoreTake(strip_lock, portMAX_DELAY);
  strip_blit(&strip, view_x, fb);
  xSemaphoreGive(strip_lock);
  st7305_flush(fb);
}

static void apply_speed_index(int index) {
  if (index < 0) index = 0;
  if (index >= kSpeedCount) index = kSpeedCount - 1;
  if (index == speed_i && speed == kSpeeds[index]) return;
  speed_i = index;
  speed = kSpeeds[index];
  next_scroll = millis();
  Serial.printf("speed %u\n", speed);
}

static void poll_button(SsButton* button, int dir) {
  int reading = digitalRead(button->pin);
  uint32_t now = millis();
  if (reading != button->read) {
    button->read = reading;
    button->change = now;
    return;
  }
  if ((uint32_t)(now - button->change) > 25 && reading != button->stable) {
    button->stable = reading;
    if (button->stable == 0) apply_speed_index(speed_i + dir);
  }
}

static void maybe_scroll(uint32_t now) {
  if (!have_frame) {
    xSemaphoreTake(strip_lock, portMAX_DELAY);
    bool ready = strip_covers(&strip, 0, PANEL_W);
    xSemaphoreGive(strip_lock);
    if (ready) {
      view_x = 0;
      show_view();
      have_frame = true;
      next_scroll = now + scroll_interval();
      Serial.println("first frame shown");
    }
    return;
  }
  if ((int32_t)(now - next_scroll) < 0) return;
  next_scroll += scroll_interval();
  if ((int32_t)(now - next_scroll) > 1000) next_scroll = now + scroll_interval();
  xSemaphoreTake(strip_lock, portMAX_DELAY);
  bool ready = strip_covers(&strip, view_x + 1, PANEL_W);
  if (ready) strip_drop_before(&strip, view_x + 1);
  xSemaphoreGive(strip_lock);
  if (!ready) {
    // The painter fell behind: hold the picture rather than jump.
    if ((++stalls % 50) == 1) Serial.printf("waiting for paint at column %lu\n", (unsigned long)view_x);
    return;
  }
  view_x = view_x + 1;
  show_view();
}

void setup() {
  esp_reset_reason_t reason = esp_reset_reason();
  Serial.begin(115200);
  pinMode(PIN_KEY, INPUT_PULLUP);
  pinMode(PIN_BOOT, INPUT_PULLUP);
  st7305_begin();
  strip_lock = xSemaphoreCreateMutex();
  fb = (uint8_t*)ps_malloc(SS_FB_BYTES);
  if (!fb) fb = (uint8_t*)malloc(SS_FB_BYTES);
  show_border();
  if (!psramFound() || !strip_init(&strip, SS_STRIP_COLUMNS, SS_COL_STRIDE)) {
    Serial.println("PSRAM missing; enable OPI PSRAM");
    return;
  }
  snprintf(seed_text, sizeof seed_text, "%08lx", (unsigned long)esp_random());
  Serial.printf("shan shui: reset reason %s, seed %s\n", reset_reason_name(reason), seed_text);
  // Painting one chunk of mountains can keep core 0 busy for seconds, which
  // would starve its idle task and trip the watchdog. Stop watching the idle
  // tasks. (disableCore0WDT() leaves core 0's idle hook checking in, which
  // logs "task not found" every time core 0 rests.)
  esp_task_wdt_config_t wdt = {};
  wdt.timeout_ms = 5000;
  wdt.idle_core_mask = 0;
  wdt.trigger_panic = true;
  esp_task_wdt_reconfigure(&wdt);
  xTaskCreatePinnedToCore(painter_task, "painter", 48 * 1024, nullptr, 1, nullptr, 0);
}

void loop() {
  poll_button(&key_btn, 1);
  poll_button(&boot_btn, -1);
  maybe_scroll(millis());
  delay(1);
}
