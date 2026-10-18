/*
  DIY Yoto firmware - ESP32-S3-WROOM-1
  ------------------------------------
  Fully offline: Wi-Fi and Bluetooth are never started. Everything lives on the SD card.

  How cards work
  - ANY 13.56 MHz tag the RC522 can read works (MIFARE Classic, Ultralight, NTAG213/215/216,
    DESFire...). The firmware only reads the tag's unique ID (UID), so nothing is ever
    written to the card and no special formatting is needed.
  - Each card plays the audio files in   /cards/<UID>/   in alphabetical order.
  - Tap an unknown card and the firmware creates that folder for you and logs the UID to
    /new_cards.txt. Put the SD card in a computer, drop audio files into the new folder, done.

  Controls
  - Volume knob: turn = volume, press = pause / resume
  - Track knob:  turn = next / previous track, press = restart current track
  - Lift the card off = pause. Put the same card back = resume where it left off.
  - New card = starts that card (from the track it was last on).
  - Idle for IDLE_SLEEP_MS = deep sleep. Press either knob to wake.

  Libraries (Arduino Library Manager / GitHub)
  - MFRC522 by GithubCommunity
  - ESP32-audioI2S by schreibfaul1 (needs PSRAM enabled, see README)
*/

#include <Arduino.h>
#include <SPI.h>
#include <FS.h>
#include <SD_MMC.h>
#include <MFRC522.h>
#include <Preferences.h>
#include <vector>
#include <algorithm>
#include "Audio.h"
#include "driver/rtc_io.h"
#include "soc/gpio_reg.h"

// ===================== Pins (match the schematic) =====================
constexpr int PIN_I2S_BCLK  = 5;
constexpr int PIN_I2S_LRC   = 6;
constexpr int PIN_I2S_DOUT  = 7;
constexpr int PIN_AMP_SD    = 8;    // MAX98357A SD_MODE: HIGH = on, LOW = shutdown

constexpr int PIN_RFID_RST  = 9;
constexpr int PIN_RFID_SS   = 10;
constexpr int PIN_RFID_MOSI = 11;
constexpr int PIN_RFID_SCK  = 12;
constexpr int PIN_RFID_MISO = 13;

constexpr int PIN_SD_CMD    = 38;
constexpr int PIN_SD_CLK    = 39;
constexpr int PIN_SD_D0     = 40;
constexpr int PIN_SD_D1     = 41;
constexpr int PIN_SD_D2     = 42;
constexpr int PIN_SD_D3     = 21;

constexpr int PIN_VOL_A     = 15;
constexpr int PIN_VOL_B     = 16;
constexpr int PIN_BTN_VOL   = 17;
constexpr int PIN_TRK_A     = 18;
constexpr int PIN_TRK_B     = 4;
constexpr int PIN_BTN_TRK   = 2;

constexpr int PIN_BAT_SENSE = 1;    // 100k/100k divider from VBAT

// ===================== Settings you may want to tweak =====================
constexpr int      START_VOLUME      = 8;      // 0..21
constexpr int      MAX_VOLUME        = 16;     // cap for little ears (library max is 21)
constexpr int      STEPS_PER_DETENT  = 4;      // set to 2 if one click skips two steps
constexpr bool     REVERSE_VOL_KNOB  = false;  // flip if turning right lowers volume
constexpr bool     REVERSE_TRK_KNOB  = false;  // flip if turning right goes back
constexpr uint32_t CARD_POLL_MS      = 200;    // how often to look for the card
constexpr int      CARD_REMOVE_MISSES = 3;     // misses in a row before "card removed"
constexpr uint32_t IDLE_SLEEP_MS     = 5UL * 60UL * 1000UL;  // 5 minutes
constexpr uint32_t DEBOUNCE_MS       = 30;
constexpr int      BAT_LOW_MV        = 3450;   // play a warning once
constexpr int      BAT_CRITICAL_MV   = 3300;   // warn, then sleep to protect the cell
constexpr uint32_t BAT_CHECK_MS      = 10000;

const char* CARDS_DIR  = "/cards";
const char* SYSTEM_DIR = "/system";   // optional: unknown.mp3, lowbatt.mp3

// ===================== Globals =====================
Audio       audio;
MFRC522     rfid(PIN_RFID_SS, PIN_RFID_RST);
Preferences prefs;

enum class State { Idle, Playing, Paused, CardRemoved, SystemSound };
State state = State::Idle;

std::vector<String> tracks;
String   currentUid;          // card whose tracks are loaded
String   presentUid;          // card currently on the reader ("" if none)
bool     cardPresent = false;
int      missCount   = 0;
int      trackIdx    = 0;
int      failedTracks = 0;
int      volume      = START_VOLUME;

uint32_t trackStartedAt = 0;
uint32_t lastActivity   = 0;
uint32_t lastCardPoll   = 0;
uint32_t lastBatCheck   = 0;
uint32_t volChangedAt   = 0;
bool     volDirty       = false;
bool     lowBatWarned   = false;
int      criticalCount  = 0;

// ===================== Rotary encoders (interrupt driven) =====================
struct Encoder {
  uint8_t pinA, pinB;
  volatile int32_t count;
  volatile uint8_t last;
};
Encoder encVol{PIN_VOL_A, PIN_VOL_B, 0, 0};
Encoder encTrk{PIN_TRK_A, PIN_TRK_B, 0, 0};
portMUX_TYPE encMux = portMUX_INITIALIZER_UNLOCKED;
static const int8_t QDEC[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};

void IRAM_ATTR encoderISR(void* arg) {
  Encoder* e = static_cast<Encoder*>(arg);
  uint32_t in = REG_READ(GPIO_IN_REG);  // all encoder pins are < 32
  uint8_t s = (((in >> e->pinA) & 1) << 1) | ((in >> e->pinB) & 1);
  portENTER_CRITICAL_ISR(&encMux);
  e->count += QDEC[(e->last << 2) | s];
  e->last = s;
  portEXIT_CRITICAL_ISR(&encMux);
}

void setupEncoder(Encoder& e) {
  pinMode(e.pinA, INPUT_PULLUP);
  pinMode(e.pinB, INPUT_PULLUP);
  e.last = (digitalRead(e.pinA) << 1) | digitalRead(e.pinB);
  attachInterruptArg(e.pinA, encoderISR, &e, CHANGE);
  attachInterruptArg(e.pinB, encoderISR, &e, CHANGE);
}

int takeDetents(Encoder& e) {
  portENTER_CRITICAL(&encMux);
  int d = e.count / STEPS_PER_DETENT;
  e.count -= d * STEPS_PER_DETENT;
  portEXIT_CRITICAL(&encMux);
  return d;
}

// ===================== Push buttons =====================
struct Button {
  uint8_t pin;
  bool stable;
  bool lastRead;
  uint32_t changedAt;
};
Button btnVol{PIN_BTN_VOL, true, true, 0};
Button btnTrk{PIN_BTN_TRK, true, true, 0};

bool wasPressed(Button& b) {
  bool r = digitalRead(b.pin);
  uint32_t now = millis();
  if (r != b.lastRead) { b.lastRead = r; b.changedAt = now; }
  if (now - b.changedAt > DEBOUNCE_MS && r != b.stable) {
    b.stable = r;
    if (!r) return true;  // pressed = pulled LOW
  }
  return false;
}

// ===================== Helpers =====================
void ampOn()  { digitalWrite(PIN_AMP_SD, HIGH); }
void ampOff() { digitalWrite(PIN_AMP_SD, LOW); }

String trackKey(const String& uid) {
  // Preferences keys max 15 chars
  String tail = uid.length() > 14 ? uid.substring(uid.length() - 14) : uid;
  return "t" + tail;
}

bool isAudioFile(String name) {
  name.toLowerCase();
  if (name.startsWith(".")) return false;  // skip macOS "._" junk files
  return name.endsWith(".mp3") || name.endsWith(".m4a") || name.endsWith(".aac") ||
         name.endsWith(".wav") || name.endsWith(".flac");
}

void logNewCard(const String& uid) {
  File f = SD_MMC.open("/new_cards.txt", FILE_APPEND);
  if (f) { f.println(uid); f.close(); }
}

// Load all audio files for a card. Returns false if the card has no folder / no files.
bool loadTracks(const String& uid) {
  tracks.clear();
  String dir = String(CARDS_DIR) + "/" + uid;
  File d = SD_MMC.open(dir);
  if (!d || !d.isDirectory()) {
    SD_MMC.mkdir(CARDS_DIR);
    SD_MMC.mkdir(dir);
    logNewCard(uid);
    Serial.printf("New card %s: created %s - add audio files there.\n", uid.c_str(), dir.c_str());
    return false;
  }
  File f;
  while ((f = d.openNextFile())) {
    if (!f.isDirectory() && isAudioFile(f.name())) tracks.push_back(dir + "/" + f.name());
    f.close();
  }
  d.close();
  std::sort(tracks.begin(), tracks.end());
  Serial.printf("Card %s: %u track(s)\n", uid.c_str(), (unsigned)tracks.size());
  return !tracks.empty();
}

// ===================== Playback =====================
void playTrack(int idx) {
  if (tracks.empty()) return;
  if (idx < 0) idx = 0;
  if (idx >= (int)tracks.size()) idx = tracks.size() - 1;
  trackIdx = idx;
  audio.stopSong();
  ampOn();
  Serial.printf("Playing %d/%u: %s\n", trackIdx + 1, (unsigned)tracks.size(), tracks[trackIdx].c_str());
  if (audio.connecttoFS(SD_MMC, tracks[trackIdx].c_str())) failedTracks = 0;
  else failedTracks++;  // the end-of-track check below skips to the next one
  state = State::Playing;
  trackStartedAt = millis();
  lastActivity = millis();
  prefs.putInt(trackKey(currentUid).c_str(), trackIdx);
}

void pausePlayback(State newState) {
  if (audio.isRunning()) audio.pauseResume();
  ampOff();
  state = newState;
  lastActivity = millis();
}

void resumePlayback() {
  ampOn();
  if (!audio.isRunning()) audio.pauseResume();
  state = State::Playing;
  trackStartedAt = millis();
  lastActivity = millis();
}

void stopPlayback() {
  audio.stopSong();
  ampOff();
  state = State::Idle;
  lastActivity = millis();
}

bool playSystemSound(const char* name) {
  String path = String(SYSTEM_DIR) + "/" + name;
  if (!SD_MMC.exists(path)) return false;
  audio.stopSong();
  ampOn();
  if (!audio.connecttoFS(SD_MMC, path.c_str())) { ampOff(); return false; }
  state = State::SystemSound;
  trackStartedAt = millis();
  return true;
}

// ===================== RFID =====================
// Returns true and the UID (hex) if any card is on the reader right now.
// WakeupA also wakes cards we halted, so a card left on the reader keeps being "seen".
bool pollCard(String& uidOut) {
  byte atqa[2];
  byte len = sizeof(atqa);
  MFRC522::StatusCode st = rfid.PICC_WakeupA(atqa, &len);
  if (st != MFRC522::STATUS_OK && st != MFRC522::STATUS_COLLISION) return false;
  if (!rfid.PICC_ReadCardSerial()) return false;
  uidOut = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    char h[3];
    snprintf(h, sizeof(h), "%02X", rfid.uid.uidByte[i]);
    uidOut += h;
  }
  rfid.PICC_HaltA();
  return true;
}

void onCardInserted(const String& uid) {
  lastActivity = millis();
  Serial.printf("Card on: %s\n", uid.c_str());

  // Same card back on the reader: carry on where we left off
  if (uid == currentUid && state == State::CardRemoved) { resumePlayback(); return; }

  if (!loadTracks(uid)) {
    currentUid = "";
    stopPlayback();
    playSystemSound("unknown.mp3");
    return;
  }
  currentUid = uid;
  int saved = prefs.getInt(trackKey(uid).c_str(), 0);
  playTrack(saved < (int)tracks.size() ? saved : 0);
}

void onCardRemoved() {
  Serial.println("Card off");
  if (state == State::Playing || state == State::Paused) pausePlayback(State::CardRemoved);
  else if (state == State::SystemSound) stopPlayback();
}

void checkCard() {
  if (millis() - lastCardPoll < CARD_POLL_MS) return;
  lastCardPoll = millis();
  String uid;
  if (pollCard(uid)) {
    missCount = 0;
    if (!cardPresent || uid != presentUid) {
      cardPresent = true;
      presentUid = uid;
      onCardInserted(uid);
    }
  } else if (cardPresent && ++missCount >= CARD_REMOVE_MISSES) {
    cardPresent = false;
    presentUid = "";
    onCardRemoved();
  }
}

// ===================== Controls =====================
void checkControls() {
  int dv = takeDetents(encVol);
  if (REVERSE_VOL_KNOB) dv = -dv;
  if (dv != 0) {
    volume = constrain(volume + dv, 0, MAX_VOLUME);
    audio.setVolume(volume);
    volDirty = true;
    volChangedAt = millis();
    lastActivity = millis();
    Serial.printf("Volume %d\n", volume);
  }
  if (volDirty && millis() - volChangedAt > 3000) {  // save once the knob settles
    prefs.putInt("vol", volume);
    volDirty = false;
  }

  int dt = takeDetents(encTrk);
  if (REVERSE_TRK_KNOB) dt = -dt;
  if (dt != 0 && cardPresent && !tracks.empty() && presentUid == currentUid) {
    playTrack(constrain(trackIdx + dt, 0, (int)tracks.size() - 1));
  }

  if (wasPressed(btnVol)) {
    lastActivity = millis();
    if (state == State::Playing) pausePlayback(State::Paused);
    else if (state == State::Paused) resumePlayback();
    else if (state == State::Idle && cardPresent && !tracks.empty()) playTrack(trackIdx);
  }

  if (wasPressed(btnTrk)) {
    lastActivity = millis();
    if (cardPresent && !tracks.empty()) playTrack(trackIdx);
  }
}

void checkTrackEnd() {
  if (millis() - trackStartedAt < 800) return;  // give the decoder time to start
  if (state == State::Playing && !audio.isRunning()) {
    if (failedTracks >= (int)tracks.size()) {   // every file failed: give up
      Serial.println("No playable files on this card.");
      stopPlayback();
    } else if (trackIdx + 1 < (int)tracks.size()) {
      playTrack(trackIdx + 1);
    } else {
      Serial.println("End of card.");
      prefs.putInt(trackKey(currentUid).c_str(), 0);  // start over next time
      trackIdx = 0;
      stopPlayback();
    }
  } else if (state == State::SystemSound && !audio.isRunning()) {
    ampOff();
    state = State::Idle;
  }
}

// ===================== Battery & sleep =====================
int batteryMv() { return analogReadMilliVolts(PIN_BAT_SENSE) * 2; }

void goToSleep() {
  Serial.println("Going to sleep. Press a knob to wake.");
  audio.stopSong();
  ampOff();
  digitalWrite(PIN_RFID_RST, LOW);  // RC522 hard power-down
  gpio_hold_en((gpio_num_t)PIN_RFID_RST);
  gpio_hold_en((gpio_num_t)PIN_AMP_SD);
  gpio_deep_sleep_hold_en();

  rtc_gpio_pullup_en((gpio_num_t)PIN_BTN_VOL);
  rtc_gpio_pulldown_dis((gpio_num_t)PIN_BTN_VOL);
  rtc_gpio_pullup_en((gpio_num_t)PIN_BTN_TRK);
  rtc_gpio_pulldown_dis((gpio_num_t)PIN_BTN_TRK);
  uint64_t mask = (1ULL << PIN_BTN_VOL) | (1ULL << PIN_BTN_TRK);
  esp_sleep_enable_ext1_wakeup(mask, ESP_EXT1_WAKEUP_ANY_LOW);
  delay(50);
  esp_deep_sleep_start();
}

void checkBattery() {
  if (millis() - lastBatCheck < BAT_CHECK_MS) return;
  lastBatCheck = millis();
  int mv = batteryMv();
  if (mv < 1000) return;  // divider not fitted / reading nonsense

  if (mv < BAT_CRITICAL_MV) {
    if (++criticalCount >= 3) {
      Serial.printf("Battery critical (%d mV)\n", mv);
      if (playSystemSound("lowbatt.mp3")) {
        uint32_t t = millis();
        while (audio.isRunning() && millis() - t < 8000) { audio.loop(); delay(1); }
      }
      goToSleep();
    }
  } else {
    criticalCount = 0;
  }

  if (mv < BAT_LOW_MV && !lowBatWarned && state == State::Idle) {
    lowBatWarned = true;
    Serial.printf("Battery low (%d mV)\n", mv);
    playSystemSound("lowbatt.mp3");
  }
}

void checkIdleSleep() {
  bool idle = (state == State::Idle || state == State::Paused || state == State::CardRemoved);
  if (idle && millis() - lastActivity > IDLE_SLEEP_MS) goToSleep();
}

// ===================== Setup & loop =====================
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\nDIY Yoto starting (offline mode)");

  // Undo anything left over from deep sleep
  gpio_hold_dis((gpio_num_t)PIN_RFID_RST);
  gpio_hold_dis((gpio_num_t)PIN_AMP_SD);
  gpio_deep_sleep_hold_dis();
  rtc_gpio_deinit((gpio_num_t)PIN_BTN_VOL);
  rtc_gpio_deinit((gpio_num_t)PIN_BTN_TRK);

  pinMode(PIN_AMP_SD, OUTPUT);
  ampOff();
  pinMode(PIN_BTN_VOL, INPUT_PULLUP);
  pinMode(PIN_BTN_TRK, INPUT_PULLUP);
  setupEncoder(encVol);
  setupEncoder(encTrk);

  prefs.begin("yoto", false);
  volume = constrain(prefs.getInt("vol", START_VOLUME), 0, MAX_VOLUME);

  // SD card in 4-bit SDMMC mode
  SD_MMC.setPins(PIN_SD_CLK, PIN_SD_CMD, PIN_SD_D0, PIN_SD_D1, PIN_SD_D2, PIN_SD_D3);
  while (!SD_MMC.begin("/sdcard", false)) {
    Serial.println("SD card not found - insert a FAT32 card. Retrying...");
    delay(2000);
  }
  SD_MMC.mkdir(CARDS_DIR);
  Serial.printf("SD card OK (%llu MB)\n", SD_MMC.cardSize() / (1024ULL * 1024ULL));

  // RFID
  SPI.begin(PIN_RFID_SCK, PIN_RFID_MISO, PIN_RFID_MOSI, PIN_RFID_SS);
  rfid.PCD_Init();
  rfid.PCD_SetAntennaGain(rfid.RxGain_max);
  byte ver = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  Serial.printf("RC522 version: 0x%02X %s\n", ver,
                (ver == 0x00 || ver == 0xFF) ? "(not responding - check wiring!)" : "");

  // Audio
  audio.setPinout(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT);
  audio.forceMono(true);  // single speaker: mix left + right
  audio.setVolume(volume);

  lastActivity = millis();
  Serial.println("Ready. Put a card on the reader.");
}

void loop() {
  audio.loop();
  checkCard();
  checkControls();
  checkTrackEnd();
  checkBattery();
  checkIdleSleep();
  delay(1);
}
