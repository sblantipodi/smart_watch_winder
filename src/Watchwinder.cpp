/*
  Watchwinder.cpp - Smart Watch Winder
  
  Copyright (C) 2020  Davide Perini
  
  Permission is hereby granted, free of charge, to any person obtaining a copy of 
  this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
  copies of the Software, and to permit persons to whom the Software is 
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in 
  all copies or substantial portions of the Software.
  
  You should have received a copy of the MIT License along with this program.  
  If not, see <https://opensource.org/licenses/MIT/>.
  Components:
    - Arduino C++ sketch running on an ESP8266EX D1 Mini from Lolin running at 160MHz
    - Raspberry + Home Assistant for Web GUI, automations and MQTT server
    - ULN2003 BYJ48 Stepper motor  
    - SD1306 OLED 128x64 pixel 0.96"
    - 1000uf capacitor for 5V power stabilization
    - Google Home Mini for Voice Recognition  
*/

#include <FS.h> //this needs to be first, or it all crashes and burns...
#include <Watchwinder.h>

/********************************** START SETUP*****************************************/
void setup() {

  Serial.begin(SERIAL_RATE);

  // Stepper Motor PINS
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  
  // LED_BUILTIN
  pinMode(LED_BUILTIN, OUTPUT);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.setTextColor(WHITE);

  // Bootsrap setup() with Wifi and MQTT functions
  bootstrapManager.bootstrapSetup(manageDisconnections, manageHardwareButton, callback);

  readConfigFromStorage();

}

/********************************** MANAGE WIFI AND MQTT DISCONNECTION *****************************************/
void manageDisconnections() {
  
  // shut down stepper motor if wifi disconnects
  stepperMotorOn = false;
  writeStep(arrayDefault);       

}

/********************************** MQTT SUBSCRIPTIONS *****************************************/
void manageQueueSubscription() {
  
  bootstrapManager.subscribe(SMARTOSTAT_CLIMATE_STATE_TOPIC);
  bootstrapManager.subscribe(WATCHWINDER_CMND_REBOOT);   
  bootstrapManager.subscribe(WATCHWINDER_CMND_SHOWLASTPAGE);            
  bootstrapManager.subscribe(WATCHWINDER_CMND_TOPIC);
  bootstrapManager.subscribe(WATCHWINDER_CMND_POWER);
  bootstrapManager.subscribe(WATCHWINDER_SETTINGS);
  
}

/********************************** MANAGE HARDWARE BUTTON *****************************************/
void manageHardwareButton() {
 
}

/********************************** START CALLBACK*****************************************/
void callback(char* topic, byte* payload, unsigned int length) {

  StaticJsonDocument<BUFFER_SIZE> json = bootstrapManager.parseQueueMsg(topic, payload, length);

  if(strcmp(topic, WATCHWINDER_CMND_REBOOT) == 0) {
    processRebootCmnd(json);
  } else if(strcmp(topic, WATCHWINDER_CMND_SHOWLASTPAGE) == 0) {
    processShowLastPageCmnd(json);
  } else if(strcmp(topic, SMARTOSTAT_CLIMATE_STATE_TOPIC) == 0) {
    processSmartostatClimateJson(json);
  } else if(strcmp(topic, WATCHWINDER_CMND_TOPIC) == 0) {
    processDisplayCmnd(json);
  } else if(strcmp(topic, WATCHWINDER_CMND_POWER) == 0) {
    processCmndPower(json);
  } else if(strcmp(topic, WATCHWINDER_SETTINGS) == 0) {
    processCmndSettings(json);
  }

}

void drawOrShutDownDisplay() {

  if (stateOn) {
    draw();
  } else {
    display.clearDisplay();
    display.display();
  }

}

void draw() {

  display.clearDisplay();
  
  // Show last page, showLastPage can only be triggered via MQTT due there is no physical button
  if (showLastPage) {
    bootstrapManager.drawInfoPage(VERSION, AUTHOR);
  } else {
    
    bootstrapManager.drawScreenSaver(AUTHOR + " domotics");

    if (showHaSplashScreen) {
      drawCenterScreenLogo(showHaSplashScreen, HABIGLOGO, HABIGLOGOW, HABIGLOGOH, DELAY_10);
    } 

    // Print rotation done
    display.setTextSize(4);
    display.setTextWrap(false);
    display.setCursor(6, 2);
    remainingRotation = rotationNumber - numbersOfRotationDone;
    display.printf("%03d", numbersOfRotationDone);

    // Print top right logo
    display.setTextSize(1);
    if (moving) {
      display.drawBitmap((SCREEN_WIDTH-(counterLogoW + 3)), -6, counterLogo, counterLogoW, counterLogoH, 1);
    } else {    
      if (stepperMotorOn) {
        display.drawBitmap((SCREEN_WIDTH-45), -2, pauseLogo, pauseLogoW, pauseLogoH, 1);
      } else {
        display.drawBitmap((SCREEN_WIDTH-(counterLogoW + 3)), -6, powerLogo, powerLogoW, powerLogoH, 1);
      }    
    }

    // Speedometer logo
    if (steptime < 1400) {
      display.drawBitmap(0, (SCREEN_HEIGHT-24), speedometerLogo, speedometerLogoW, speedometerLogoH, 1);
    } else if (steptime > 1400 && steptime < 2000) {
      display.drawBitmap(0, (SCREEN_HEIGHT-24), speedometerMediumLogo, speedometerMediumLogoW, speedometerMediumLogoH, 1);
    } else {
      display.drawBitmap(0, (SCREEN_HEIGHT-24), speedometerSlowLogo, speedometerSlowLogoW, speedometerSlowLogoH, 1);
    }

    // Keep it wound logo
    if (keepItWound == "on") {
      display.drawBitmap((SCREEN_WIDTH-32), (SCREEN_HEIGHT-24), watchLogo, watchLogoW, watchLogoH, 1);
    } else {
      display.drawBitmap((SCREEN_WIDTH-32), (SCREEN_HEIGHT-24), watchOffLogo, watchOffLogoW, watchOffLogoH, 1);
    }

    // Arrows logos 
    #ifdef TARGET_WATCHWINDER_3
      if (anticlockwise && moving) {
        display.drawBitmap(28, 47, arrowLeftActionLogo, arrowLeftLogoW, arrowLeftLogoH, 1);
        if (orientation == MISTO) {
          display.drawBitmap(80, 47, arrowRightLogo, arrowRightLogoW, arrowRightLogoH, 1);
        }
      }  
      if (!anticlockwise && moving) {
        display.drawBitmap(80, 47, arrowRightActionLogo, arrowRightLogoW, arrowRightLogoH, 1);
        if (orientation == MISTO) {
          display.drawBitmap(28, 47, arrowLeftLogo, arrowLeftLogoW, arrowLeftLogoH, 1);
        }
      }    
      if (anticlockwise && !moving) {
        display.drawBitmap(28, 47, arrowLeftLogo, arrowLeftLogoW, arrowLeftLogoH, 1);
        if (orientation == MISTO) {
          display.drawBitmap(80, 47, arrowRightLogo, arrowRightLogoW, arrowRightLogoH, 1);
        }
      }  
      if (!anticlockwise && !moving) {
        display.drawBitmap(80, 47, arrowRightLogo, arrowRightLogoW, arrowRightLogoH, 1);
        if (orientation == MISTO) {
          display.drawBitmap(28, 47, arrowLeftLogo, arrowLeftLogoW, arrowLeftLogoH, 1);
        }
      } 
    #else
      if (!anticlockwise && moving) {
        display.drawBitmap(28, 47, arrowLeftActionLogo, arrowLeftLogoW, arrowLeftLogoH, 1);
        if (orientation == MISTO) {
          display.drawBitmap(80, 47, arrowRightLogo, arrowRightLogoW, arrowRightLogoH, 1);
        }
      }  
      if (anticlockwise && moving) {
        display.drawBitmap(80, 47, arrowRightActionLogo, arrowRightLogoW, arrowRightLogoH, 1);
        if (orientation == MISTO) {
          display.drawBitmap(28, 47, arrowLeftLogo, arrowLeftLogoW, arrowLeftLogoH, 1);
        }
      }    
      if (!anticlockwise && !moving) {
        display.drawBitmap(28, 47, arrowLeftLogo, arrowLeftLogoW, arrowLeftLogoH, 1);
        if (orientation == MISTO) {
          display.drawBitmap(80, 47, arrowRightLogo, arrowRightLogoW, arrowRightLogoH, 1);
        }
      }  
      if (anticlockwise && !moving) {
        display.drawBitmap(80, 47, arrowRightLogo, arrowRightLogoW, arrowRightLogoH, 1);
        if (orientation == MISTO) {
          display.drawBitmap(28, 47, arrowLeftLogo, arrowLeftLogoW, arrowLeftLogoH, 1);
        }
      } 
    #endif
    
    
    // Max Rotation number
    display.setTextSize(1);
    display.setCursor(41, 35);
    display.printf("-|"); display.printf("%03d", rotationNumber); display.printf("|-");

    // Remaining rotation
    display.setTextSize(2);
    display.setCursor(44, SCREEN_HEIGHT-17);
    display.printf("%03d", remainingRotation);
    
    display.setTextWrap(true);
  }
  display.display();

}

void drawCenterScreenLogo(bool &triggerBool, const unsigned char* logo, const int logoW, const int logoH, const int delayInt) {

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.drawBitmap(
    (display.width()  - logoW) / 2,
    (display.height() - logoH) / 2,
    logo, logoW, logoH, 1);
  display.display();
  delay(delayInt);
  triggerBool = false;

}

void drawRoundRect() {

  display.drawRoundRect(47, 19, 72, 27, 10, WHITE);
  display.drawRoundRect(47, 20, 71, 25, 9, WHITE);
  display.drawRoundRect(47, 20, 71, 25, 10, WHITE);
  display.drawRoundRect(47, 20, 71, 25, 8, WHITE);
  display.drawRoundRect(48, 20, 70, 25, 10, WHITE);

}

/********************************** START PROCESS JSON*****************************************/
bool processSmartostatClimateJson(StaticJsonDocument<BUFFER_SIZE> json) {
  
  if (json.containsKey("smartostat")) {
    String timeConst = json["Time"];
    // On first boot the timedate variable is OFF
    if (timedate == OFF_CMD) {
      helper.setDateTime(timeConst);
      lastBoot = date + " " + currentime;
    } else {
      helper.setDateTime(timeConst);
    }
    // lastMQTTConnection and lastWIFiConnection are resetted on every disconnection
    if (lastMQTTConnection == OFF_CMD) {
      lastMQTTConnection = date + " " + currentime;
    }
    if (lastWIFiConnection == OFF_CMD) {
      lastWIFiConnection = date + " " + currentime;
    }
    haVersion = helper.getValue(json["haVersion"]);
  }

  return true;

}

bool processDisplayCmnd(StaticJsonDocument<BUFFER_SIZE> json) {

  String message = helper.isOnOff(json);
  if (message == ON_CMD) {
    stateOn = true;
  } else if(message == OFF_CMD) {
    stateOn = false;
  }
  screenSaverTriggered = false;
  sendPowerState();
  return true;

}

bool processCmndPower(StaticJsonDocument<BUFFER_SIZE> json) {

  #ifdef TARGET_WATCHWINDER_3
    anticlockwise = true;
  # else
    anticlockwise = false;
  #endif
  String message = helper.isOnOff(json);
  if (message == ON_CMD) {
    stepperMotorOn = true;
    stateOn = true;
  } else if(message == OFF_CMD) {
    stepperMotorOn = false;
    stateOn = false;
    numbersOfRotationDone = 0;  
  }  
  screenSaverTriggered = false;
  sendPowerState();
  sendMotorPowerState();
  return true;

}

bool processCmndSettings(StaticJsonDocument<BUFFER_SIZE> json) {

  if (json.containsKey("orientation")) {
    String orientation = helper.getValue(json["orientation"]);

    if (orientation == ORARIO) {
      anticlockwise = true;
    } else if (orientation != ORARIO) {
      anticlockwise = false;
    }    
    #ifdef TARGET_WATCHWINDER_3
      if (orientation == ORARIO) {
        anticlockwise = false;
      } else if (orientation != ORARIO) {
        anticlockwise = true;
      } 
    #endif 

    int rotationSpeedConst = json["rotation_speed"];
    steptime = rotationSpeedConst;
    int rotationNumberConst = json["rotation_number"];
    rotationNumber = rotationNumberConst;   
    keepItWound = helper.getValue(json["keep_it_wound"]);

    int brightness = json["brightness"];
    display.ssd1306_command(0x81);
    display.ssd1306_command(brightness); //min 10 max 255
    display.ssd1306_command(0xD9);
    if (brightness <= 80) {        
      display.ssd1306_command(17); 
    } else {
      display.ssd1306_command(34); 
    }
  }
  drawOrShutDownDisplay();
  return true;

}

bool processRebootCmnd(StaticJsonDocument<BUFFER_SIZE> json) {

  rebootState = helper.isOnOff(json);
  sendRebootState(OFF_CMD);
  if (rebootState == OFF_CMD) {      
    sendRebootCmnd();
  }
  return true;

}

bool processShowLastPageCmnd(StaticJsonDocument<BUFFER_SIZE> json) {

  String message = helper.isOnOff(json);
  if (message == ON_CMD) {
    showLastPage = true;
    stateOn = true;
    sendMotorPowerState();
    sendPowerState();
  } else if(message == OFF_CMD) {
    showLastPage = false;
  } 
  return true;

}

/********************************** SEND STATE *****************************************/
void sendPowerState() {

  drawOrShutDownDisplay();
  bootstrapManager.publish(WATCHWINDER_STATE_TOPIC, (stateOn) ? helper.string2char(ON_CMD) : helper.string2char(OFF_CMD), true);

}

void sendMotorPowerState() {

  bootstrapManager.publish(WATCHWINDER_STAT_POWER, (stepperMotorOn) ? helper.string2char(ON_CMD) : helper.string2char(OFF_CMD), true);

}

void sendPowerStateCmnd() {

  bootstrapManager.publish(WATCHWINDER_CMND_TOPIC, (stateOn) ? helper.string2char(ON_CMD) : helper.string2char(OFF_CMD), true);

}

void sendMotorPowerStateCmnd() {

  bootstrapManager.publish(WATCHWINDER_CMND_POWER, (stepperMotorOn) ? helper.string2char(ON_CMD) : helper.string2char(OFF_CMD), true);

}

void sendInfoState() {

  JsonObject root = bootstrapManager.getJsonObject();
  root["State"] = (stateOn) ? ON_CMD : OFF_CMD;
  root["rotation_done"] = numbersOfRotationDone;

  bootstrapManager.sendState(WATCHWINDER_INFO_TOPIC, root, VERSION); 

}

void sendRebootState(String onOff) {   

  bootstrapManager.publish(WATCHWINDER_STAT_REBOOT, helper.string2char(onOff), true);

}

void sendRebootCmnd() {  

  delay(DELAY_10);
  ESP.restart();

}

void writeStep(int outArray[4]) {

  digitalWrite(IN1, outArray[0]);
  digitalWrite(IN2, outArray[1]);
  digitalWrite(IN3, outArray[2]);
  digitalWrite(IN4, outArray[3]);

}

void stepper() {

  if ((Step >= 0) && (Step < 8)) {
    writeStep(stepsMatrix[Step]);
  } else {
    writeStep(arrayDefault);
  }
  setDirection();

}

void setDirection() {

  (anticlockwise == true) ? (Step++) : (Step--);
  if (Step > 7) {
    Step = 0;
  } else if (Step < 0) {
    Step = 7;

  }
}

void stepperMotorManager() {

  if (stepsLeft > 0) {
    moving = true;
    if (stepsLeft == NBSTEPS) {
      drawOrShutDownDisplay();
    }
    currentMicros = micros();
    if (currentMicros - lastTime >= steptime) {
      stepper();
      thistime += micros() - lastTime;
      lastTime = micros();
      stepsLeft--;        
    }
  } else if (stepsLeft == 0) {
    moving = false;
    drawOrShutDownDisplay();
    stepsLeft = -1;
  }

}

// Send status to MQTT broker every ten seconds
void delayAndSendStatus() {

  if(millis() > timeNowStatus + tenSecondsPeriod){
    timeNowStatus = millis();
    ledTriggered = true;
    sendPowerState();
    sendInfoState();
  }

}

// Trigger screensaver evert 5 minutes
void triggerScreenSaverAfterFiveMinutes() {

  if(millis() > timeNowGoHomeAfterFiveMinutes + fiveMinutesPeriod) {
    timeNowGoHomeAfterFiveMinutes = millis();
    screenSaverTriggered = true;    
  }

}

// Go to home page after five minutes of inactivity and write to Storage
void writeConfigToStorageAfterMinute() {

  if(millis() > timeNowWriteStorageMinute + delay_1_minute) {
    timeNowWriteStorageMinute = millis();
    // Write data to file system
    writeConfigToStorage();
  }

}

// Manage Stepper Motor every seconds
void manageStepperMotorEverySeconds() {

  // ((NBSTEPS * steptime) / 1000) equals to the time spent for a complete rotation
  int millisecondsDelay = (((NBSTEPS * steptime) / 1000) + rotationDelayPeriord);
  if(millis() > timeNowManageStepperMotorAfterSeconds + millisecondsDelay) {
    timeNowManageStepperMotorAfterSeconds = millis();
    currentMicros = 0;
    stepsLeft = NBSTEPS;
    thistime = 0;
    lastTime = micros();
    if (orientation == MISTO) {
      anticlockwise = !anticlockwise;
      if (!anticlockwise) {
        numbersOfRotationDone++;
      }
    } else {
      numbersOfRotationDone++;
    }
    stepsLeft = NBSTEPS;
    drawOrShutDownDisplay();
  }

}

/********************************** LITTLE FS MANAGEMENT *****************************************/
void readConfigFromStorage() {
  
  DynamicJsonDocument doc(1024);
  doc = bootstrapManager.readLittleFS("config.json");

  if (!(doc.containsKey(VALUE) && doc[VALUE] == ERROR)) {
    Serial.println(F("\nReload previously stored values."));
    numbersOfRotationDone = doc["numbersOfRotationDone"]; 
  }

}

void writeConfigToStorage() {

  DynamicJsonDocument doc(1024);
  doc["numbersOfRotationDone"] = numbersOfRotationDone;     
  bootstrapManager.writeToLittleFS(doc, "config.json");      

}

/********************************** START MAIN LOOP *****************************************/
// Screen drawing call is done outside the loop for non blocking low delay loop needed to drive stepper motor without rattling
void loop() {  

  // Bootsrap loop() with Wifi, MQTT and OTA functions
  bootstrapManager.bootstrapLoop(manageDisconnections, manageQueueSubscription, manageHardwareButton);

  // Send status on MQTT Broker every n seconds
  delayAndSendStatus();
  
  // Trigger screensaver every 5 minutes
  triggerScreenSaverAfterFiveMinutes();
  
  // Write SPIFFS every minute, saves the numbers of rotation.
  if (stepperMotorOn == true) {
    writeConfigToStorageAfterMinute();
  }

  // Shut down stepper motor if numbers of daily rotation has been reached
  if (numbersOfRotationDone > rotationNumber) {
    numbersOfRotationDone = 0;
    stepperMotorOn = false;
    sendMotorPowerState();
    if (showLastPage) {
      stateOn = true;
    } else {
      stateOn = false;
    }
    sendPowerState();
    sendPowerStateCmnd();
    sendMotorPowerStateCmnd();
    writeConfigToStorage();
  } else if (showLastPage) {
    drawOrShutDownDisplay();
  }

  // Manage Stepper Motor every two seconds
  if (stepperMotorOn) {
    stepperMotorManager();
    manageStepperMotorEverySeconds();
  } else {
    moving = false;
    // shut down stepper motor
    writeStep(arrayDefault);
    if (!stateOn) {
      display.clearDisplay();
      display.display();
    }
  }

  bootstrapManager.nonBlokingBlink();

}