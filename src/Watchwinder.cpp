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

  readConfigFromSPIFFS();

}

/********************************** MANAGE WIFI AND MQTT DISCONNECTION *****************************************/
void manageDisconnections() {
  
  // shut down stepper motor if wifi disconnects
  stepperMotorOn = false;
  writeStep(arrayDefault);       

}

/********************************** MQTT SUBSCRIPTIONS *****************************************/
void manageQueueSubscription() {
  
  mqttClient.subscribe(SMARTOSTAT_CLIMATE_STATE_TOPIC);
  mqttClient.subscribe(WATCHWINDER_CMND_REBOOT);   
  mqttClient.subscribe(WATCHWINDER_CMND_SHOWLASTPAGE);            
  mqttClient.subscribe(WATCHWINDER_CMND_TOPIC);
  mqttClient.subscribe(WATCHWINDER_CMND_POWER);
  mqttClient.subscribe(WATCHWINDER_SETTINGS);
  
}

/********************************** MANAGE HARDWARE BUTTON *****************************************/
void manageHardwareButton() {
 
}

/********************************** START CALLBACK*****************************************/
void callback(char* topic, byte* payload, unsigned int length) {
  // Serial.print(F("Message arrived from [");
  // Serial.print(topic);
  // Serial.println(F("] ");

  char message[length + 1];
  for (unsigned int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  //Serial.println(message);

  if(strcmp(topic, WATCHWINDER_CMND_REBOOT) == 0) {
    if (!processRebootCmnd(message)) {
      return;
    }
  }
  if(strcmp(topic, WATCHWINDER_CMND_SHOWLASTPAGE) == 0) {
    if (!processShowLastPageCmnd(message)) {
      return;
    }
  }
  if(strcmp(topic, SMARTOSTAT_CLIMATE_STATE_TOPIC) == 0) {
    if (!processSmartostatClimateJson(message)) {
      return;
    }
  }
  if(strcmp(topic, WATCHWINDER_CMND_TOPIC) == 0) {
    if (!processDisplayCmnd(message)) {
      return;
    }
  }
  if(strcmp(topic, WATCHWINDER_CMND_POWER) == 0) {
    if (!processCmndPower(message)) {
      return;
    }
  }
  if(strcmp(topic, WATCHWINDER_SETTINGS) == 0) {
    if (!processCmndSettings(message)) {
      return;
    }
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
bool processSmartostatClimateJson(char* message) {
  StaticJsonDocument<BUFFER_SIZE> doc;
  DeserializationError error = deserializeJson(doc, message);
  if (error) {
    Serial.println(F("parseObject() failed 2"));
    return false;
  }

  if (doc.containsKey("smartostat")) {
    const char* timeConst = doc["Time"];
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
    const char* haVersionConst = doc["haVersion"];
    haVersion = haVersionConst;
  }
  return true;
}

bool processDisplayCmnd(char* message) {
  if(strcmp(message, ON_CMD) == 0) {
    stateOn = true;
  } else if(strcmp(message, OFF_CMD) == 0) {
    stateOn = false;
  }
  screenSaverTriggered = false;
  sendPowerState();
  return true;
}

bool processCmndPower(char* message) {
  #ifdef TARGET_WATCHWINDER_3
    anticlockwise = true;
  # else
    anticlockwise = false;
  #endif
  if(strcmp(message, ON_CMD) == 0) {
    stepperMotorOn = true;
    stateOn = true;
  } else if(strcmp(message, OFF_CMD) == 0) {
    stepperMotorOn = false;
    stateOn = false;
    numbersOfRotationDone = 0;  
    writeConfigToSPIFFS();
  }  
  screenSaverTriggered = false;
  sendPowerState();
  sendMotorPowerState();
  return true;
}

bool processCmndSettings(char* message) {
  StaticJsonDocument<BUFFER_SIZE> doc;
  DeserializationError error = deserializeJson(doc, message);
  if (error) {
    Serial.println(F("parseObject() failed 2"));
    return false;
  }
  if (doc.containsKey("orientation")) {
    String orientationConst = doc["orientation"];
    orientation = orientationConst;

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

    int rotationSpeedConst = doc["rotation_speed"];
    steptime = rotationSpeedConst;
    int rotationNumberConst = doc["rotation_number"];
    rotationNumber = rotationNumberConst;   
    String keepItWoundConst = doc["keep_it_wound"];
    keepItWound = keepItWoundConst;    
  }
  drawOrShutDownDisplay();
  return true;
}

bool processRebootCmnd(char* message) {
  String rebootState = message;
  sendRebootState(OFF_CMD);
  if (rebootState == OFF_CMD) {      
    sendRebootCmnd();
  }
  return true;
}

bool processShowLastPageCmnd(char* message) {
  if(strcmp(message, ON_CMD) == 0) {
    showLastPage = true;
    stateOn = true;
    sendMotorPowerState();
    sendPowerState();
  } else if(strcmp(message, OFF_CMD) == 0) {
    showLastPage = false;
  } 
  return true;
}

/********************************** SEND STATE *****************************************/
void sendPowerState() {
  drawOrShutDownDisplay();
  mqttClient.publish(WATCHWINDER_STATE_TOPIC, (stateOn) ? ON_CMD : OFF_CMD, true);
}

void sendMotorPowerState() {
  mqttClient.publish(WATCHWINDER_STAT_POWER, (stepperMotorOn) ? ON_CMD : OFF_CMD, true);
}

void sendPowerStateCmnd() {
  mqttClient.publish(WATCHWINDER_CMND_TOPIC, (stateOn) ? ON_CMD : OFF_CMD, true);
}

void sendMotorPowerStateCmnd() {
  mqttClient.publish(WATCHWINDER_CMND_POWER, (stepperMotorOn) ? ON_CMD : OFF_CMD, true);
}

void sendInfoState() {
  StaticJsonDocument<BUFFER_SIZE> doc;

  JsonObject root = doc.to<JsonObject>();

  root["Whoami"] = WIFI_DEVICE_NAME;
  root["IP"] = WiFi.localIP().toString();
  root["MAC"] = WiFi.macAddress();
  root["ver"] = VERSION;

  root["State"] = (stateOn) ? ON_CMD : OFF_CMD;
  root["Time"] = timedate;
  root["rotation_done"] = numbersOfRotationDone;

  char buffer[measureJson(root) + 1];
  serializeJson(root, buffer, sizeof(buffer));

  // publish state only if it has received time from HA
  if (timedate != OFF_CMD) {
    mqttClient.publish(WATCHWINDER_INFO_TOPIC, buffer, true);
  }
}

void sendRebootState(const char* onOff) {   
  mqttClient.publish(WATCHWINDER_STAT_REBOOT, onOff, true);
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

// Go to home page after five minutes of inactivity and write SPIFFS
void writeConfigToSPIFFSAfterMinute() {
  if(millis() > timeNowWriteSpiffsMinute + delay_1_minute) {
    timeNowWriteSpiffsMinute = millis();
    // Write data to file system
    writeConfigToSPIFFS();
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

/********************************** SPIFFS MANAGEMENT *****************************************/
void readConfigFromSPIFFS() {
  bool error = false;
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println(F("Mounting SPIFSS..."));
  display.display();
  // SPIFFS.remove("/config.json");
  if (SPIFFS.begin()) {
    display.println(F("FS mounted"));
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      display.println(F("Reading config.json file..."));
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        display.println(F("Config OK"));
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonDocument doc(1024);        
        DeserializationError deserializeError = deserializeJson(doc, buf.get());
        Serial.println(F("\nReading config.json"));
        serializeJsonPretty(doc, Serial);
        if (!deserializeError) {
          display.println(F("JSON parsed"));
          if(doc["numbersOfRotationDone"]) {
            display.println(F("Reload previously stored values."));
            numbersOfRotationDone = doc["numbersOfRotationDone"]; 
          } else {
            error = true;
            display.println(F("JSON file is empty"));
          }
        } else {
          error = true;
          display.println(F("Failed to load json config"));
        }
      }
    }
  } else {
    error = true;
    display.println(F("failed to mount FS"));
  }
  display.display();
  if (!error) {
    delay(DELAY_10);
  }
}

void writeConfigToSPIFFS() {
    if (SPIFFS.begin()) {
      Serial.println(F("\nSaving config.json\n"));
      DynamicJsonDocument doc(1024);
      doc["numbersOfRotationDone"] = numbersOfRotationDone;     
      // SPIFFS.format();
      File configFile = SPIFFS.open("/config.json", "w");
      if (!configFile) {
        Serial.println(F("Failed to open config file for writing"));
      }
      serializeJsonPretty(doc, Serial);
      serializeJson(doc, configFile);
      configFile.close();
    } else {
      Serial.println(F("Failed to mount FS for write"));
    }
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
    writeConfigToSPIFFSAfterMinute();
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
    writeConfigToSPIFFS();
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