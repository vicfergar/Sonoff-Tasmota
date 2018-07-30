/*
  xdrv_95_rowentafan.ino - Rowenta fan controller support for Sonoff-Tasmota

  Copyright (C) 2018  Victor Ferrer

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*********************************************************************************************\
 * Rowenta Fan controller
 *
 * 
\*********************************************************************************************/

#define ROWFAN_LED_LOW 4
#define ROWFAN_LED_MEDIUM 14
#define ROWFAN_LED_HIGH 12
#define ROWFAN_LED_BOOST 13
#define ROWFAN_LED_MOSQUITO 5

#define ROWFAN_BTN_POWER ROWFAN_LED_BOOST
#define ROWFAN_BTN_SPEED ROWFAN_LED_LOW
#define ROWFAN_BTN_BOOST ROWFAN_LED_HIGH
#define ROWFAN_BTN_MOSQUITO ROWFAN_LED_MEDIUM

typedef enum
{
  UNDEFINED = 0,
  OFF,
  MINIMUM,
  MEDIUM,
  FAST,
  BOOST,
  MOSQUITO
} RowFanStates;

typedef enum
{
  SPEED_LOW = 0,
  SPEED_MED,
  SPEED_HIGH,
  SPEED_BOOST,
  SPEED_MOSQUITO,
  
  LAST
} RowFanLedsTypes;

uint8_t rowfan_type;
bool rowfan_publish;

uint8_t rowfan_leds_pinout[] = { ROWFAN_LED_LOW, ROWFAN_LED_MEDIUM, ROWFAN_LED_HIGH, ROWFAN_LED_BOOST, ROWFAN_LED_MOSQUITO };
RowFanStates rowfan_state;
RowFanStates rowfan_desired_state;
bool rowfan_leds_readings[RowFanLedsTypes::LAST];
bool rowfan_valid_leds_readings;
uint8_t rowfan_counter_leds_readings = 0;
int rowfan_active_btn_pin = -1;

void rowfan_init()
{
  rowfan_state = RowFanStates::UNDEFINED;
  rowfan_desired_state = RowFanStates::UNDEFINED;

  for (int i = 0; i < RowFanLedsTypes::LAST; i++) {
    pinMode(rowfan_leds_pinout[i], INPUT);
  }
}

void rowfan_update_leds_state()
{
  int total = 0;
  rowfan_valid_leds_readings = true;
  
  for (byte i = 0; i < RowFanLedsTypes::LAST; i++) {
    rowfan_leds_readings[i] = digitalRead(rowfan_leds_pinout[i]);
    total += rowfan_leds_readings[i];
    rowfan_valid_leds_readings = (total < 3);
  }

  if(rowfan_valid_leds_readings) {
    if ((total - rowfan_leds_readings[RowFanLedsTypes::SPEED_MOSQUITO]) == 0) {
      if (rowfan_counter_leds_readings > 0) rowfan_counter_leds_readings--;
      rowfan_valid_leds_readings &= rowfan_counter_leds_readings == 0;
    } else rowfan_counter_leds_readings = 20;
  }
}

void rowfan_refresh_state()
{
  rowfan_publish = rowfan_state == RowFanStates::UNDEFINED;

  if (!rowfan_leds_readings[RowFanLedsTypes::SPEED_MOSQUITO]) {
    rowfan_state = MOSQUITO;
  } else if (rowfan_leds_readings[RowFanLedsTypes::SPEED_LOW]) {
    rowfan_state = MINIMUM;
  } else if (rowfan_leds_readings[RowFanLedsTypes::SPEED_MED]) {
    rowfan_state = MEDIUM;
  } else if (rowfan_leds_readings[RowFanLedsTypes::SPEED_HIGH]) {
    rowfan_state = FAST;
  } else if (rowfan_leds_readings[RowFanLedsTypes::SPEED_BOOST]) {
    rowfan_state = BOOST;
  } else {
    rowfan_state = OFF;
  }
}

void rowfan_control_state()
{
  if(rowfan_desired_state == rowfan_state || rowfan_desired_state == RowFanStates::UNDEFINED) {
    rowfan_desired_state = RowFanStates::UNDEFINED;
    return;
  } else if(rowfan_desired_state == RowFanStates::OFF && rowfan_state != RowFanStates::OFF) {
    rowfan_btn_press(ROWFAN_BTN_POWER);
  } else {
    if(rowfan_state == RowFanStates::OFF) {
      rowfan_btn_press(ROWFAN_BTN_POWER);
    } else if(rowfan_desired_state == RowFanStates::MOSQUITO) {
      rowfan_btn_press(ROWFAN_BTN_MOSQUITO);
    } else if(rowfan_desired_state == RowFanStates::BOOST) {
      rowfan_btn_press(ROWFAN_BTN_BOOST);
    } else {
      rowfan_btn_press(ROWFAN_BTN_SPEED);
    }
  }
}

void rowfan_btn_press(int pin)
{
  if(rowfan_active_btn_pin < 0)
  {
    rowfan_active_btn_pin = pin;
    pinMode(rowfan_active_btn_pin, OUTPUT);
    digitalWrite(rowfan_active_btn_pin, 0);
  }
}

/*********************************************************************************************/

void RowFanInit()
{
  rowfan_type = ROWENTA_FAN == Settings.module;
  rowfan_publish = false;

  if (rowfan_type) {
    rowfan_init();
  }
}

void RowFanMqttPublish(uint8_t state)
{
  snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_FANSPEED "\":%d}"), state - 1);
  MqttPublishPrefixTopic_P(RESULT_OR_STAT, PSTR(D_CMND_FANSPEED), Settings.flag.mqtt_power_retain);
  mqtt_data[0] = '\0';
}

void RowFanLoop()
{
  if (rowfan_active_btn_pin > 0) {
      pinMode(rowfan_active_btn_pin, INPUT);
      rowfan_active_btn_pin = -1;
  }
  
  rowfan_update_leds_state();
  
  if (rowfan_valid_leds_readings) {
    rowfan_refresh_state();
    rowfan_control_state();
  }

  if (rowfan_publish) {
    rowfan_publish = false;

    if (rowfan_desired_state == RowFanStates::UNDEFINED) {
      RowFanMqttPublish(rowfan_state);
    }
  }
}

boolean RowFanCommand()
{
  char command [CMDSZ];
  boolean serviced = true;
  uint16_t payload = XdrvMailbox.payload;

  int command_code = GetCommandCode(command, sizeof(command), XdrvMailbox.topic, kTasmotaCommands);
  if (-1 == command_code) {
    serviced = false;  // Unknown command
  } else if (CMND_POWER == command_code) {
    if (payload == 0) {
      rowfan_desired_state = RowFanStates::OFF;
    } else if (payload == 1 && rowfan_desired_state == RowFanStates::OFF){
      rowfan_desired_state = RowFanStates::MINIMUM;
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, rowfan_desired_state != RowFanStates::OFF);
  } else if (CMND_FANSPEED == command_code) {
    if (payload >= 0 && payload <= 5) {
      rowfan_desired_state = (RowFanStates)(XdrvMailbox.payload + 1);
      RowFanMqttPublish(rowfan_desired_state);
    } else {
      RowFanMqttPublish(rowfan_state);
    }    
  } else {
    serviced = false;  // Unknown command
  }
  return serviced;
}

/*********************************************************************************************\
 * Interface
\*********************************************************************************************/

#define XDRV_95

boolean Xdrv95(byte function)
{
  boolean result = false;

  if (function == FUNC_INIT) {
    RowFanInit();
  } else if (rowfan_type) {
    switch (function) {
      case FUNC_EVERY_50_MSECOND:
        RowFanLoop();
        break;
      case FUNC_COMMAND:
        result = RowFanCommand();
        break;
    }
  }
  return result;
}
