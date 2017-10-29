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

#define PHONE_LOOP_INTERVAL 200
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
int _phone_callingPin;
EntryPhoneStates _phone_doortState = WAIT_OP;
bool _phone_isCalling;


void entryphone_init()
{
  //Set door pinout
  _phone_doorPin = 15;
  pinMode(_phone_doorPin, OUTPUT);
  digitalWrite(_phone_doorPin, LOW); //disable pin

  //Set calling door detector
  _phone_callingPin = 13;
  pinMode(_phone_callingPin, INPUT);
  _phone_isCalling = false;
  
  tickerPhone.attach_ms(PHONE_LOOP_INTERVAL, entryphone_everyinterval);
}

boolean entryphone_command(char *type, uint16_t index, char *dataBuf, uint16_t data_len, int16_t payload, char *svalue, uint16_t ssvalue)
{
  boolean serviced = true; 

  if (!strcmp(type,"OPENDOOR")) {
    snprintf_P(svalue, ssvalue, PSTR("{\"OpenDoor\":\"Done\"}"));
    _phone_doortState = REQUEST_OPEN_DOOR;
  }
  else {
    serviced = false;
  }
  return serviced;
}

void entryphone_everyinterval()
{
  bool callingValue = !digitalRead(_phone_callingPin);

  if(callingValue != _phone_isCalling){ 
    if(callingValue){ // CALLING
      char svalue[60];
      snprintf_P(svalue, sizeof(svalue), "{\"State\":\"CALLING\"}");
      mqtt_publish_topic_P(1, PSTR("ENTRY_PHONE"), svalue);
    }
    _phone_isCalling = callingValue;
  }
  
  if(_phone_doortState == REQUEST_OPEN_DOOR){
    _phone_doortState = OPENING_DOOR;
    _phone_openingDoorTime = PHONE_OPEN_TIME;
    digitalWrite(_phone_doorPin, HIGH); //enable pin
  }
  else if(_phone_doortState == OPENING_DOOR){
    _phone_openingDoorTime -= PHONE_LOOP_INTERVAL;
    if(_phone_openingDoorTime <= 0){
      _phone_doortState = WAIT_OP;
      digitalWrite(_phone_doorPin, LOW); //disable pin
    }
  }
}
#endif // USE_ENTRY_PHONE
