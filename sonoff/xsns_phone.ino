/*
Copyright (c) 2016 Theo Arends.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef USE_ENTRY_PHONE
/*********************************************************************************************\
 * ENTRY_PHONE
 *
 * Source: Victor Ferrer https://github.com/vicfergar/Sonoff-MQTT-OTA-Arduino
\*********************************************************************************************/

#define PHONE_LOOP_INTERVAL 1000
#define PHONE_OPEN_TIME 1000

Ticker tickerPhone;

typedef enum
{
   WAIT_OP = 0,
   REQUEST_OPEN_DOOR,
   OPENING_DOOR
} EntryPhoneStates;

int _phone_openingDoorTime = 0;
int _phone_doorPin;
EntryPhoneStates _phone_currentState = WAIT_OP;

void entryphone_init()
{
  //Set door pinout
  _phone_doorPin = pin[GPIO_OPEN_DOOR];
  pinMode(_phone_doorPin, OUTPUT);
  digitalWrite(_phone_doorPin, HIGH); //disable pin
  
  tickerPhone.attach_ms(PHONE_LOOP_INTERVAL, entryphone_everysecond);
}

boolean entryphone_command(char *type, uint16_t index, char *dataBuf, uint16_t data_len, int16_t payload, char *svalue, uint16_t ssvalue)
{
  boolean serviced = true; 

  if (!strcmp(type,"OPENDOOR")) {
    snprintf_P(svalue, ssvalue, PSTR("{\"OpenDoor\":\"Done\"}"));
    _phone_currentState = REQUEST_OPEN_DOOR;
  }
  else {
    serviced = false;
  }
  return serviced;
}

void entryphone_everysecond()
{
  if(_phone_currentState == REQUEST_OPEN_DOOR){
    _phone_currentState = OPENING_DOOR;
    _phone_openingDoorTime = PHONE_OPEN_TIME;
    digitalWrite(_phone_doorPin, LOW); //enable pin
  }
  else if(_phone_currentState == OPENING_DOOR){
    _phone_openingDoorTime -= PHONE_LOOP_INTERVAL;
    if(_phone_openingDoorTime <= 0){
      _phone_currentState = WAIT_OP;
      digitalWrite(_phone_doorPin, HIGH); //disable pin
    }
  }
}
#endif // USE_ENTRY_PHONE
