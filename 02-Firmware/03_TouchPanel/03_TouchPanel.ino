#include <Arduino.h>
#include <math.h>
#include <esp_display_panel.hpp>
#include <lvgl.h>
#include "lvgl_v8_port.h"
#include "ui.h"
#include "BLEDevice.h"
#include "BLEUtils.h"
#include "BLEServer.h"
#include "BLE2902.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

#define BLE_DEVICE_NAME "Kangfu-RS485"
#define BLE_SERVICE_UUID "5b12fdaf-479a-4722-a0f3-b993592377b2"
#define BLE_RX_CHARACTERISTIC_UUID "768a019c-a143-40c8-834d-d515c433e006"
#define BLE_TX_CHARACTERISTIC_UUID "768a019c-a143-40c8-834d-d515c433e007"

#ifndef RS485_RX_PIN
#define RS485_RX_PIN 15
#endif
#ifndef RS485_TX_PIN
#define RS485_TX_PIN 16
#endif
#ifndef RS485_DE_RE_PIN
#define RS485_DE_RE_PIN -1
#endif
#ifndef RS485_BAUD
#define RS485_BAUD 115200
#endif
#ifndef RS485_SERIAL
#define RS485_SERIAL Serial1
#endif

enum TouchCommand : uint8_t {
  TOUCH_MOTOR1_START = 0x01,
  TOUCH_MOTOR2_SEND_MIT = 0x02,
  TOUCH_MOTOR2_RESET_ZERO = 0x03,
  TOUCH_MOTOR2_START = 0x04,
  TOUCH_MOTOR1_ANGLE = 0x10,
  TOUCH_MOTOR1_VELOCITY = 0x11,
  TOUCH_MOTOR1_CURRENT = 0x12,
  TOUCH_MOTOR1_KP = 0x13,
  TOUCH_MOTOR1_KI = 0x14,
  TOUCH_MODE_SELECT = 0x20,
  TOUCH_MODE_ORIGINAL = 0x21,
  TOUCH_MODE_PASSIVE = 0x22,
  TOUCH_MODE_ACTIVE = 0x23,
  TOUCH_MODE_COMPOSITE = 0x24,
};

struct MotorParams {
  float angle;
  float velocity;
  float current;
  float kp;
  float ki;
};

static Board *displayBoard = nullptr;
static bool motor1Running = false;
static bool motor2Running = false;
static BLEServer *pServer = nullptr;
static BLECharacteristic *pRxCharacteristic = nullptr;
static BLECharacteristic *pTxCharacteristic = nullptr;
static bool deviceConnected = false;
static SemaphoreHandle_t rs485Mutex = nullptr;

static constexpr size_t QT_FRAME_LENGTH = 14;
static constexpr uint8_t FORCE_SENSOR_CMD_GROUP = 0x21;
static constexpr uint8_t FORCE_SENSOR_ZERO_CMD = 0x01;

static float textareaFloat(lv_obj_t *textarea) {
  const char *text = lv_textarea_get_text(textarea);
  return String(text).toFloat();
}

static MotorParams readAngleParams() {
  MotorParams params;
  params.angle = textareaFloat(ui_TextArea5);
  params.velocity = textareaFloat(ui_TextArea4);
  params.current = textareaFloat(ui_TextArea3);
  params.kp = textareaFloat(ui_TextArea2);
  params.ki = textareaFloat(ui_TextArea1);
  return params;
}

static MotorParams readHorizonParams() {
  MotorParams params;
  params.angle = textareaFloat(ui_TextArea10);
  params.velocity = textareaFloat(ui_TextArea9);
  params.current = textareaFloat(ui_TextArea6);
  params.kp = textareaFloat(ui_TextArea7);
  params.ki = textareaFloat(ui_TextArea8);
  return params;
}

static void appendFloatLE(uint8_t *buffer, size_t &index, float value) {
  union { float f; uint8_t b[4]; } converter;
  converter.f = value;
  for (uint8_t i = 0; i < 4; i++) buffer[index++] = converter.b[i];
}

static void appendInt32LE(uint8_t *buffer, size_t &index, int32_t value) {
  buffer[index++] = (uint8_t)(value & 0xFF);
  buffer[index++] = (uint8_t)((value >> 8) & 0xFF);
  buffer[index++] = (uint8_t)((value >> 16) & 0xFF);
  buffer[index++] = (uint8_t)((value >> 24) & 0xFF);
}

static int32_t readInt32LE(const uint8_t *buffer) {
  return (int32_t)((uint32_t)buffer[0] | ((uint32_t)buffer[1] << 8) |
                   ((uint32_t)buffer[2] << 16) | ((uint32_t)buffer[3] << 24));
}

static uint8_t qtFrameChecksum(const uint8_t *frame) {
  uint8_t checksum = 0;
  for (uint8_t i = 2; i < 13; i++) checksum += frame[i];
  return checksum;
}

static bool isValidQtFrame(const uint8_t *frame, size_t length) {
  if (length != QT_FRAME_LENGTH) return false;
  if (frame[0] != 0x6D || frame[1] != 0x6A) return false;
  return qtFrameChecksum(frame) == frame[13];
}

static bool isForceSensorZeroFrame(const uint8_t *frame) {
  return frame[2] == 0x01 && frame[3] == FORCE_SENSOR_CMD_GROUP &&
         frame[4] == FORCE_SENSOR_ZERO_CMD && readInt32LE(&frame[5]) != 0;
}

static void rs485WriteRaw(const uint8_t *data, size_t length) {
  if (length == 0) return;
  bool useMutex = (rs485Mutex != nullptr);
  if (useMutex && xSemaphoreTake(rs485Mutex, pdMS_TO_TICKS(500)) != pdTRUE) return;
  Serial.printf("RS485 write %u bytes:", (unsigned)length);
  for (size_t i = 0; i < length; i++) Serial.printf(" %02X", data[i]);
  Serial.println();
#if RS485_DE_RE_PIN >= 0
  digitalWrite(RS485_DE_RE_PIN, HIGH);
  delayMicroseconds(20);
  RS485_SERIAL.write(data, length);
  RS485_SERIAL.flush();
  delayMicroseconds(20);
  digitalWrite(RS485_DE_RE_PIN, LOW);
#else
  RS485_SERIAL.write(data, length);
  RS485_SERIAL.flush();
#endif
  if (useMutex) xSemaphoreGive(rs485Mutex);
}

static void rs485SendQtFrame(uint8_t cmd1, uint8_t cmd2, const uint8_t dataBytes[8]) {
  uint8_t frame[14]; size_t index = 0;
  frame[index++] = 0x6D; frame[index++] = 0x6A; frame[index++] = 0x01;
  frame[index++] = cmd1; frame[index++] = cmd2;
  for (uint8_t i = 0; i < 8; i++) frame[index++] = dataBytes[i];
  frame[index++] = qtFrameChecksum(frame);
  rs485WriteRaw(frame, index);
}

static void rs485SendIntFrame(uint8_t cmd1, uint8_t cmd2, int32_t value) {
  uint8_t dataBytes[8] = {0}; size_t index = 0;
  appendInt32LE(dataBytes, index, value);
  rs485SendQtFrame(cmd1, cmd2, dataBytes);
}

static void rs485SendFloatFrame(uint8_t cmd1, uint8_t cmd2, float value) {
  uint8_t dataBytes[8] = {0}; size_t index = 0;
  appendFloatLE(dataBytes, index, value);
  rs485SendQtFrame(cmd1, cmd2, dataBytes);
}

static void rs485SendMotor1Param(uint8_t command, lv_obj_t *textarea) {
  rs485SendFloatFrame(0x06, command, textareaFloat(textarea));
}

static void rs485SendMotorStartStop(uint8_t motorId, bool start) {
  uint8_t cmd1 = (motorId == 1) ? 0x01 : 0x11;
  rs485SendIntFrame(cmd1, 0x01, start ? 1 : 0);
}

static void rs485SendMotor1Params(const MotorParams &params) {
  rs485SendFloatFrame(0x06, 0x01, params.angle);
  rs485SendFloatFrame(0x06, 0x02, params.velocity);
  rs485SendFloatFrame(0x06, 0x03, params.current);
  rs485SendFloatFrame(0x06, 0x04, params.kp);
  rs485SendFloatFrame(0x06, 0x05, params.ki);
}

static void updateRunButton(lv_obj_t *buttonLabel, lv_obj_t *button, bool running) {
  lv_label_set_text(buttonLabel, running ? "Stop" : "Start");
  lv_obj_set_style_bg_color(button, lv_color_hex(running ? 0xC43E3E : 0x2F80ED), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(button, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void bleNotifyStatus(uint8_t status, uint8_t detail) {
  if (!deviceConnected || pTxCharacteristic == nullptr) return;
  uint8_t statusData[4] = {0xA5, status, detail, 0x00};
  pTxCharacteristic->setValue(statusData, sizeof(statusData));
  pTxCharacteristic->notify();
}

static void bleNotifyRunState(uint8_t motorId, bool running) {
  if (!deviceConnected || pTxCharacteristic == nullptr) return;
  uint8_t statusData[4] = {0xA6, motorId, running ? 1 : 0, 0x00};
  pTxCharacteristic->setValue(statusData, sizeof(statusData));
  pTxCharacteristic->notify();
}

static void forwardControllerStatusToBle() {
  static uint8_t status[13];
  static uint8_t index = 0;
  static uint8_t expected = 0;

  while (RS485_SERIAL.available() > 0) {
    const uint8_t value = static_cast<uint8_t>(RS485_SERIAL.read());
    if (index == 0) {
      if (value == 0xA7) expected = 4;
      else if (value == 0xA8) expected = 13;
      else continue;
    }
    status[index++] = value;
    if (index < expected) continue;

    const uint8_t length = expected;
    index = 0;
    expected = 0;
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < length - 1; ++i) checksum ^= status[i];
    if (status[length - 1] != checksum) {
      Serial.println("STM32 status checksum failed");
      continue;
    }
    if (deviceConnected && pTxCharacteristic != nullptr) {
      pTxCharacteristic->setValue(status, length);
      pTxCharacteristic->notify();
    }
  }
}

static void syncRunStateFromQtFrame(const uint8_t *frame) {
  if (frame[4] != 0x01) return;
  lv_obj_t *buttonLabel = nullptr; lv_obj_t *button = nullptr;
  bool *running = nullptr; uint8_t motorId = 0;
  if (frame[3] == 0x01) { motorId = 1; running = &motor1Running; buttonLabel = ui_Label16; button = ui_DIANJI1Qiting; }
  else if (frame[3] == 0x11) { motorId = 2; running = &motor2Running; buttonLabel = ui_Label17; button = ui_Button8; }
  else return;
  bool newRunning = readInt32LE(&frame[5]) != 0;
  lvgl_port_lock(-1); *running = newRunning;
  updateRunButton(buttonLabel, button, newRunning);
  lvgl_port_unlock();
  bleNotifyRunState(motorId, newRunning);
}

static void toggleMotorRun(uint8_t motorId, bool &running, lv_obj_t *buttonLabel, lv_obj_t *button) {
  running = !running;
  rs485SendMotorStartStop(motorId, running);
  updateRunButton(buttonLabel, button, running);
  bleNotifyRunState(motorId, running);
}

static uint16_t mapToUInt(float value, float minValue, float maxValue, uint16_t maxInt) {
  return (uint16_t)((value - minValue) / (maxValue - minValue) * (float)maxInt);
}

static void rs485SendMotor2MitParams(const MotorParams &params) {
  uint8_t dataBytes[8] = {0};
  uint16_t angleInt = mapToUInt(params.angle, -12.57f, 12.57f, 65535);
  uint16_t velocityInt = mapToUInt(params.velocity, -33.0f, 33.0f, 4095);
  uint16_t kpInt = (uint16_t)((params.kp / 500.0f) * 4095.0f);
  uint16_t kdInt = (uint16_t)((params.ki / 5.0f) * 4095.0f);
  uint16_t torqueInt = mapToUInt(params.current, -14.0f, 14.0f, 4095);
  dataBytes[0] = (uint8_t)((angleInt >> 8) & 0xFF); dataBytes[1] = (uint8_t)(angleInt & 0xFF);
  dataBytes[2] = (uint8_t)((velocityInt >> 4) & 0xFF);
  dataBytes[3] = (uint8_t)(((velocityInt & 0x0F) << 4) | ((kpInt >> 8) & 0x0F));
  dataBytes[4] = (uint8_t)(kpInt & 0xFF);
  dataBytes[5] = (uint8_t)((kdInt >> 4) & 0xFF);
  dataBytes[6] = (uint8_t)(((kdInt & 0x0F) << 4) | ((torqueInt >> 8) & 0x0F));
  dataBytes[7] = (uint8_t)(torqueInt & 0xFF);
  rs485SendQtFrame(0x16, 0x01, dataBytes);
}

static void rs485SendMotor2ZeroSet() { rs485SendIntFrame(0x11, 0x03, 1); }

static void modeEventCb(lv_event_t *event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
  uint8_t command = (uint8_t)(uintptr_t)lv_event_get_user_data(event);
  Serial.printf("Mode button: 0x%02X\\r\\n", command);
  switch (command) {
    case TOUCH_MODE_SELECT:
      lv_obj_clear_flag(ui_ModePanel, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(ui_ModePanel);
      break;
    case TOUCH_MODE_ORIGINAL:
      g_current_mode = 0; lv_obj_add_flag(ui_ModePanel, LV_OBJ_FLAG_HIDDEN);
      Serial.println("Mode: Original"); break;
    case TOUCH_MODE_PASSIVE:
      g_current_mode = 1; lv_obj_add_flag(ui_ModePanel, LV_OBJ_FLAG_HIDDEN);
      Serial.println("Mode: Passive Rehab"); break;
    case TOUCH_MODE_ACTIVE:
      g_current_mode = 2; lv_obj_add_flag(ui_ModePanel, LV_OBJ_FLAG_HIDDEN);
      Serial.println("Mode: Active Rehab"); break;
    case TOUCH_MODE_COMPOSITE:
      g_current_mode = 3; lv_obj_add_flag(ui_ModePanel, LV_OBJ_FLAG_HIDDEN);
      Serial.println("Mode: Composite"); break;
  }
}

static void buttonEventCb(lv_event_t *event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
  uint8_t command = (uint8_t)(uintptr_t)lv_event_get_user_data(event);
  Serial.printf("Touch button command: 0x%02X\\r\\n", command);
  switch (command) {
    case TOUCH_MOTOR1_START: toggleMotorRun(1, motor1Running, ui_Label16, ui_DIANJI1Qiting); break;
    case TOUCH_MOTOR2_SEND_MIT: rs485SendMotor2MitParams(readHorizonParams()); break;
    case TOUCH_MOTOR2_RESET_ZERO: rs485SendMotor2ZeroSet(); break;
    case TOUCH_MOTOR2_START: toggleMotorRun(2, motor2Running, ui_Label17, ui_Button8); break;
    case TOUCH_MOTOR1_ANGLE: rs485SendMotor1Param(0x01, ui_TextArea5); break;
    case TOUCH_MOTOR1_VELOCITY: rs485SendMotor1Param(0x02, ui_TextArea4); break;
    case TOUCH_MOTOR1_CURRENT: rs485SendMotor1Param(0x03, ui_TextArea3); break;
    case TOUCH_MOTOR1_KP: rs485SendMotor1Param(0x04, ui_TextArea2); break;
    case TOUCH_MOTOR1_KI: rs485SendMotor1Param(0x05, ui_TextArea1); break;
  }
}

static void textareaEventCb(lv_event_t *event) {
  lv_event_code_t code = lv_event_get_code(event);
  if (code == LV_EVENT_FOCUSED || code == LV_EVENT_CLICKED) {
    lv_obj_t *textarea = lv_event_get_target(event);
    lv_keyboard_set_textarea(ui_Keyboard1, textarea);
    lv_obj_clear_flag(ui_Keyboard1, LV_OBJ_FLAG_HIDDEN);
  }
}

static void keyboardEventCb(lv_event_t *event) {
  lv_event_code_t code = lv_event_get_code(event);
  if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
    lv_keyboard_set_textarea(ui_Keyboard1, nullptr);
    lv_obj_add_flag(ui_Keyboard1, LV_OBJ_FLAG_HIDDEN);
  }
}

/* ========== 被动康复标签页UI ========== */
static lv_obj_t * rehabScreen = NULL;
static lv_obj_t * tabControlBtn = NULL;
static lv_obj_t * tabPassiveBtn = NULL;
static lv_obj_t * rehabToggleBtn = NULL;
static lv_obj_t * rehabSpeedSb = NULL;
static lv_obj_t * rehabCyclesSb = NULL;
static lv_obj_t * rehabRangeMinSb = NULL;
static lv_obj_t * rehabRangeMaxSb = NULL;
static lv_obj_t * rehabPauseSb = NULL;
static lv_obj_t * rehabDirDd = NULL;
static bool rehabRunning = false;

static void rehabToggleHandler(lv_event_t * e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  if (!rehabRunning) {
    rehabRunning = true;
    lv_obj_set_style_bg_color(rehabToggleBtn, lv_color_hex(0xC43E3E), LV_PART_MAIN);
    lv_label_set_text(lv_obj_get_child(rehabToggleBtn, 0), "STOP");
    const float speed = (float)lv_spinbox_get_value(rehabSpeedSb) / 1000.0f;
    const uint32_t cycles = (uint32_t)lv_spinbox_get_value(rehabCyclesSb);
    const float minRad = (float)lv_spinbox_get_value(rehabRangeMinSb) * PI / 180.0f;
    const float maxRad = (float)lv_spinbox_get_value(rehabRangeMaxSb) * PI / 180.0f;
    const uint32_t pauseMs = (uint32_t)lv_spinbox_get_value(rehabPauseSb);
    const uint8_t direction = (uint8_t)lv_dropdown_get_selected(rehabDirDd) + 1U;
    uint8_t payload[8] = {0};

    memcpy(payload, &minRad, sizeof(minRad));
    memcpy(payload + 4, &maxRad, sizeof(maxRad));
    rs485SendQtFrame(0x30, 0x03, payload);
    memset(payload, 0, sizeof(payload));
    memcpy(payload, &pauseMs, sizeof(pauseMs));
    rs485SendQtFrame(0x30, 0x04, payload);
    delay(30);
    memset(payload, 0, sizeof(payload));
    memcpy(payload, &speed, sizeof(speed));
    memcpy(payload + 4, &cycles, sizeof(cycles));
    rs485SendQtFrame(0x30, 0x01, payload);
    memset(payload, 0, sizeof(payload));
    payload[0] = direction;
    rs485SendQtFrame(0x30, 0x05, payload);
    Serial.println("Passive START");
  } else {
    rehabRunning = false;
    lv_obj_set_style_bg_color(rehabToggleBtn, lv_color_hex(0x2F80ED), LV_PART_MAIN);
    lv_label_set_text(lv_obj_get_child(rehabToggleBtn, 0), "START");
    rs485SendIntFrame(0x30, 0x02, 0);
    Serial.println("Passive STOP");
  }
}

static void tabSwitchHandler(lv_event_t * e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  uintptr_t tab = (uintptr_t)lv_event_get_user_data(e);
  if (tab == 0) {
    lv_scr_load(ui_Screen1);
    lv_obj_set_style_bg_color(tabControlBtn, lv_color_hex(0x2F80ED), LV_PART_MAIN);
    lv_obj_set_style_bg_color(tabPassiveBtn, lv_color_hex(0x555555), LV_PART_MAIN);
  } else {
    lv_scr_load(rehabScreen);
    lv_obj_set_style_bg_color(tabPassiveBtn, lv_color_hex(0x2F80ED), LV_PART_MAIN);
    lv_obj_set_style_bg_color(tabControlBtn, lv_color_hex(0x555555), LV_PART_MAIN);
  }
}

static void createRehabUI() {
  rehabScreen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(rehabScreen, lv_color_hex(0x16213E), LV_PART_MAIN);
  lv_obj_set_style_pad_all(rehabScreen, 15, LV_PART_MAIN);

  /* Title */
  lv_obj_t * title = lv_label_create(rehabScreen);
  lv_label_set_text(title, "Passive Rehabilitation");
  lv_obj_set_style_text_color(title, lv_color_hex(0xE0E7FF), LV_PART_MAIN);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_18, LV_PART_MAIN);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);

  /* Helper: create a labeled spinbox */
  auto makeSpin = [](lv_obj_t * parent, int x, int y, const char * lblText,
                      int w, int h, int min, int max, int step, int digits, int val) {
    lv_obj_t * lbl = lv_label_create(parent);
    lv_label_set_text(lbl, lblText);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0x8E99B0), LV_PART_MAIN);
    lv_obj_set_pos(lbl, x, y);
    lv_obj_t * sb = lv_spinbox_create(parent);
    lv_obj_set_size(sb, w, h);
    lv_obj_set_pos(sb, x, y + 22);
    lv_spinbox_set_range(sb, min, max);
    lv_spinbox_set_step(sb, step);
    lv_spinbox_set_value(sb, val);
    lv_obj_set_style_bg_color(sb, lv_color_hex(0x0F3460), LV_PART_MAIN);
    lv_obj_set_style_text_color(sb, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_border_color(sb, lv_color_hex(0x1A5276), LV_PART_MAIN);
    lv_obj_set_style_radius(sb, 5, LV_PART_MAIN);
    return sb;
  };

  int y0 = 85, yGap = 75;
  int xCol1 = 25, xCol2 = 420, wCol = 200;

  /* Row 1: Speed + Cycles */
  rehabSpeedSb = makeSpin(rehabScreen, xCol1, y0, "Speed (rad/s)", wCol, 40, 1, 1000, 10, 3, 200);
  rehabCyclesSb = makeSpin(rehabScreen, xCol2, y0, "Cycles", 150, 40, 1, 999, 1, 0, 10);

  /* Row 2: Range Min + Max */
  int y1 = y0 + yGap;
  rehabRangeMinSb = makeSpin(rehabScreen, xCol1, y1, "Range Min (deg)", wCol, 40, -120, 0, 5, 1, -60);
  rehabRangeMaxSb = makeSpin(rehabScreen, xCol2, y1, "Range Max (deg)", 150, 40, 0, 120, 5, 1, 60);

  /* Row 3: Pause + Direction */
  int y2 = y1 + yGap;
  rehabPauseSb = makeSpin(rehabScreen, xCol1, y2, "Pause (ms)", wCol, 40, 0, 10000, 100, 0, 500);
  rehabDirDd = lv_dropdown_create(rehabScreen);
  lv_obj_set_pos(rehabDirDd, xCol2, y2 + 22);
  lv_obj_set_size(rehabDirDd, 150, 40);
  lv_dropdown_set_options(rehabDirDd, "Flex\nExtend");
  lv_obj_set_style_bg_color(rehabDirDd, lv_color_hex(0x0F3460), LV_PART_MAIN);
  lv_obj_set_style_text_color(rehabDirDd, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_t * dirLbl = lv_label_create(rehabScreen);
  lv_label_set_text(dirLbl, "Direction");
  lv_obj_set_style_text_color(dirLbl, lv_color_hex(0x8E99B0), LV_PART_MAIN);
  lv_obj_set_pos(dirLbl, xCol2, y2);

  /* Toggle button */
  rehabToggleBtn = lv_btn_create(rehabScreen);
  lv_obj_set_size(rehabToggleBtn, 320, 55);
  lv_obj_align(rehabToggleBtn, LV_ALIGN_BOTTOM_MID, 0, -40);
  lv_obj_set_style_bg_color(rehabToggleBtn, lv_color_hex(0x2F80ED), LV_PART_MAIN);
  lv_obj_set_style_radius(rehabToggleBtn, 10, LV_PART_MAIN);
  lv_obj_t * tlbl = lv_label_create(rehabToggleBtn);
  lv_label_set_text(tlbl, "START");
  lv_obj_set_style_text_color(tlbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_text_font(tlbl, &lv_font_montserrat_18, LV_PART_MAIN);
  lv_obj_center(tlbl);
  lv_obj_add_event_cb(rehabToggleBtn, rehabToggleHandler, LV_EVENT_CLICKED, NULL);

  /* Tab buttons on both screens */
  auto addTabs = [](lv_obj_t * parent, bool isControl) {
    tabControlBtn = lv_btn_create(parent);
    lv_obj_set_size(tabControlBtn, 95, 34);
    lv_obj_align(tabControlBtn, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_set_style_radius(tabControlBtn, 6, LV_PART_MAIN);
    lv_obj_set_style_bg_color(tabControlBtn, lv_color_hex(isControl ? 0x2F80ED : 0x4A5568), LV_PART_MAIN);
    lv_obj_t * l = lv_label_create(tabControlBtn);
    lv_label_set_text(l, "Control");
    lv_obj_set_style_text_color(l, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_center(l);
    lv_obj_add_event_cb(tabControlBtn, tabSwitchHandler, LV_EVENT_CLICKED, (void*)0);

    tabPassiveBtn = lv_btn_create(parent);
    lv_obj_set_size(tabPassiveBtn, 95, 34);
    lv_obj_align(tabPassiveBtn, LV_ALIGN_TOP_LEFT, 105, 5);
    lv_obj_set_style_radius(tabPassiveBtn, 6, LV_PART_MAIN);
    lv_obj_set_style_bg_color(tabPassiveBtn, lv_color_hex(isControl ? 0x4A5568 : 0x2F80ED), LV_PART_MAIN);
    l = lv_label_create(tabPassiveBtn);
    lv_label_set_text(l, "Passive");
    lv_obj_set_style_text_color(l, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_center(l);
    lv_obj_add_event_cb(tabPassiveBtn, tabSwitchHandler, LV_EVENT_CLICKED, (void*)1);
  };

  addTabs(ui_Screen1, true);
  addTabs(rehabScreen, false);

  Serial.println("Rehab screen ready");
}

static void registerUiEvents() {
  lv_obj_add_event_cb(ui_DIANJI1Qiting, buttonEventCb, LV_EVENT_CLICKED, (void *)(uintptr_t)TOUCH_MOTOR1_START);
  lv_obj_add_event_cb(ui_Button4, buttonEventCb, LV_EVENT_CLICKED, (void *)(uintptr_t)TOUCH_MOTOR2_SEND_MIT);
  lv_obj_add_event_cb(ui_Button6, buttonEventCb, LV_EVENT_CLICKED, (void *)(uintptr_t)TOUCH_MOTOR2_RESET_ZERO);
  lv_obj_add_event_cb(ui_Button8, buttonEventCb, LV_EVENT_CLICKED, (void *)(uintptr_t)TOUCH_MOTOR2_START);
  lv_obj_add_flag(ui_Label9, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(ui_Label8, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(ui_Label7, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(ui_Label5, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(ui_Label4, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(ui_Label9, buttonEventCb, LV_EVENT_CLICKED, (void *)(uintptr_t)TOUCH_MOTOR1_ANGLE);
  lv_obj_add_event_cb(ui_Label8, buttonEventCb, LV_EVENT_CLICKED, (void *)(uintptr_t)TOUCH_MOTOR1_VELOCITY);
  lv_obj_add_event_cb(ui_Label7, buttonEventCb, LV_EVENT_CLICKED, (void *)(uintptr_t)TOUCH_MOTOR1_CURRENT);
  lv_obj_add_event_cb(ui_Label5, buttonEventCb, LV_EVENT_CLICKED, (void *)(uintptr_t)TOUCH_MOTOR1_KP);
  lv_obj_add_event_cb(ui_Label4, buttonEventCb, LV_EVENT_CLICKED, (void *)(uintptr_t)TOUCH_MOTOR1_KI);
  lv_obj_add_event_cb(ui_ModeBtn, modeEventCb, LV_EVENT_CLICKED, (void *)(uintptr_t)TOUCH_MODE_SELECT);
  lv_obj_add_event_cb(ui_ModeBtnOriginal, modeEventCb, LV_EVENT_CLICKED, (void *)(uintptr_t)TOUCH_MODE_ORIGINAL);
  lv_obj_add_event_cb(ui_ModeBtnPassive, modeEventCb, LV_EVENT_CLICKED, (void *)(uintptr_t)TOUCH_MODE_PASSIVE);
  lv_obj_add_event_cb(ui_ModeBtnActive, modeEventCb, LV_EVENT_CLICKED, (void *)(uintptr_t)TOUCH_MODE_ACTIVE);
  lv_obj_add_event_cb(ui_ModeBtnComposite, modeEventCb, LV_EVENT_CLICKED, (void *)(uintptr_t)TOUCH_MODE_COMPOSITE);
  lv_obj_add_event_cb(ui_ModeBtnClose, modeEventCb, LV_EVENT_CLICKED, (void *)(uintptr_t)TOUCH_MODE_ORIGINAL);
  lv_obj_t *textareas[] = { ui_TextArea1, ui_TextArea2, ui_TextArea3, ui_TextArea4, ui_TextArea5,
    ui_TextArea6, ui_TextArea7, ui_TextArea8, ui_TextArea9, ui_TextArea10 };
  for (lv_obj_t *textarea : textareas) {
    lv_obj_add_event_cb(textarea, textareaEventCb, LV_EVENT_ALL, nullptr);
    lv_textarea_set_one_line(textarea, true);
  }
  lv_obj_add_event_cb(ui_Keyboard1, keyboardEventCb, LV_EVENT_ALL, nullptr);
  lv_obj_add_flag(ui_Keyboard1, LV_OBJ_FLAG_HIDDEN);
  updateRunButton(ui_Label16, ui_DIANJI1Qiting, motor1Running);
  updateRunButton(ui_Label17, ui_Button8, motor2Running);
}

class BleServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *server) override { deviceConnected = true; Serial.println("BLE connected"); }
  void onDisconnect(BLEServer *server) override {
    deviceConnected = false; Serial.println("BLE disconnected"); BLEDevice::startAdvertising();
  }
};

class BleRxCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {
    const uint8_t *payload = characteristic->getData();
    size_t length = characteristic->getLength();
    Serial.printf("BLE RX %u bytes:", (unsigned)length);
    for (size_t i = 0; i < length; i++) Serial.printf(" %02X", payload[i]);
    Serial.println();
    if (length == 0 || (length % QT_FRAME_LENGTH) != 0) { bleNotifyStatus(0x01, (uint8_t)length); return; }
    for (size_t offset = 0; offset < length; offset += QT_FRAME_LENGTH) {
      const uint8_t *frame = payload + offset;
      if (!isValidQtFrame(frame, QT_FRAME_LENGTH)) { bleNotifyStatus(0x02, (uint8_t)(offset / QT_FRAME_LENGTH)); return; }
    }
    for (size_t offset = 0; offset < length; offset += QT_FRAME_LENGTH) {
      const uint8_t *frame = payload + offset;
      if (isForceSensorZeroFrame(frame)) Serial.println("BLE force sensor zero forwarded");
      rs485WriteRaw(frame, QT_FRAME_LENGTH);
      syncRunStateFromQtFrame(frame);
    }
    bleNotifyStatus(0x00, (uint8_t)(length / QT_FRAME_LENGTH));
  }
};

static void setupBle() {
  BLEDevice::init(BLE_DEVICE_NAME);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new BleServerCallbacks());
  BLEService *service = pServer->createService(BLE_SERVICE_UUID);
  pRxCharacteristic = service->createCharacteristic(BLE_RX_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  pRxCharacteristic->setCallbacks(new BleRxCallbacks());
  pTxCharacteristic = service->createCharacteristic(BLE_TX_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->addDescriptor(new BLE2902());
  uint8_t readyStatus[4] = {0xA5, 0x10, 0x00, 0x00};
  pTxCharacteristic->setValue(readyStatus, sizeof(readyStatus));
  service->start();
  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(BLE_SERVICE_UUID);
  advertising->setScanResponse(true);
  advertising->setMinPreferred(0x06);
  advertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("BLE advertising started");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  rs485Mutex = xSemaphoreCreateMutex();
#if RS485_DE_RE_PIN >= 0
  pinMode(RS485_DE_RE_PIN, OUTPUT); digitalWrite(RS485_DE_RE_PIN, LOW);
#endif
  RS485_SERIAL.begin(RS485_BAUD, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
  Serial.println(); Serial.println("kangfu display build " __DATE__ " " __TIME__);
  displayBoard = new Board(); displayBoard->init();
#if LVGL_PORT_AVOID_TEARING_MODE
  auto lcd = displayBoard->getLCD();
  lcd->configFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
#endif
  if (!displayBoard->begin()) { Serial.println("displayBoard->begin() failed"); while (true) delay(1000); }
  auto lcdAfterBegin = displayBoard->getLCD();
  if (lcdAfterBegin && lcdAfterBegin->getBasicAttributes().basic_bus_spec.isFunctionValid(
      LCD::BasicBusSpecification::FUNC_DISPLAY_ON_OFF)) lcdAfterBegin->setDisplayOnOff(true);
  if (!lvgl_port_init(displayBoard->getLCD(), displayBoard->getTouch())) {
    Serial.println("lvgl_port_init() failed"); while (true) delay(1000);
  }
  lvgl_port_lock(-1); ui_init(); createRehabUI(); registerUiEvents(); lvgl_port_unlock();
  setupBle();
}

void loop() {
  forwardControllerStatusToBle();
  delay(2);
}
