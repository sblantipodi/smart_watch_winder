/*
 * WATCHWINDER
 * 
 * Smart Watch Winder
 * DPsoftware (Davide Perini)
 * Components:
 *   - Arduino C++ sketch running on an ESP8266EX D1 Mini from Lolin running at 160MHz
 *   - Raspberry + Home Assistant for Web GUI, automations and MQTT server
 *   - ULN2003 BYJ48 Stepper motor  
 *   - SD1306 OLED 128x64 pixel 0.96"
 *   - 1000uf capacitor for 5V power stabilization
 *   - Google Home Mini for Voice Recognition
 * MIT license
 */
#include <Watchwinder.h>

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

  if(strcmp(topic, watchwinder_cmnd_reboot) == 0) {
    if (!processRebootCmnd(message)) {
      return;
    }
  }
  if(strcmp(topic, watchwinder_cmnd_showlastpage) == 0) {
    if (!processShowLastPageCmnd(message)) {
      return;
    }
  }
  if(strcmp(topic, smartostat_climate_state_topic) == 0) {
    if (!processSmartostatClimateJson(message)) {
      return;
    }
  }
  if(strcmp(topic, watchwinder_cmnd_topic) == 0) {
    if (!processDisplayCmnd(message)) {
      return;
    }
  }
  if(strcmp(topic, watchwinder_cmnd_power) == 0) {
    if (!processCmndPower(message)) {
      return;
    }
  }
  if(strcmp(topic, watchwinder_settings) == 0) {
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
    printLastPage();
  } else {
    if (screenSaverTriggered) {
      drawScreenSaver();
    }

    if (showHaSplashScreen) {
      drawCenterScreenLogo(showHaSplashScreen, habigLogo, habigLogoW, habigLogoH, delay_4000);
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

void drawScreenSaver() {
  display.clearDisplay();
  for (int i = 0; i < 50; i++) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(5,17);
    display.fillRect(0, 0, display.width(), display.height(), i%2 != 0 ? WHITE : BLACK);
    display.setTextColor(i%2 == 0 ? WHITE : BLACK);
    display.drawRoundRect(0, 0, display.width()-1, display.height()-1, display.height()/4, i%2 == 0 ? WHITE : BLACK);
    display.println(F("DPsoftware domotics"));
    display.display();
  }
  display.setTextColor(WHITE);
  screenSaverTriggered = false;
}

void printLastPage() {
  yoffset -= 1;
  // add/remove 8 pixel for every line yoffset <= -209, if you want to add a line yoffset <= -217
  if (yoffset <= -209) {
    yoffset = SCREEN_HEIGHT + 6;
    lastPageScrollTriggered = true;
  }
  int effectiveOffset = (yoffset >= 0 && !lastPageScrollTriggered) ? 0 : yoffset;

  display.drawBitmap((display.width()-habigLogoW)-1, effectiveOffset+5, habigLogo, habigLogoW, habigLogoH, 1);
  display.setCursor(0, effectiveOffset);
  display.setTextSize(1);
  display.print(F("WATCH WINDER "));
  display.println(VERSION);
  display.println(F("by DPsoftware"));
  display.println(F(""));
  
  display.print(F("HA: ")); display.print(F("(")); display.print(haVersion); display.println(F(")"));
  display.print(F("Wifi: ")); display.print(getQuality()); display.println(F("%")); 
  display.print(F("Heap: ")); display.print(ESP.getFreeHeap()/1024); display.println(F(" KB")); 
  display.print(F("Free Flash: ")); display.print(ESP.getFreeSketchSpace()/1024); display.println(F(" KB")); 
  display.print(F("Frequency: ")); display.print(ESP.getCpuFreqMHz()); display.println(F("MHz")); 

  display.print(F("Flash: ")); display.print(ESP.getFlashChipSize()/1024); display.println(F(" KB")); 
  display.print(F("Sketch: ")); display.print(ESP.getSketchSize()/1024); display.println(F(" KB")); 
  display.print(F("IP: ")); display.println(WiFi.localIP());
  display.println(F("MAC: ")); display.println(WiFi.macAddress());
  display.print(F("SDK: ")); display.println(ESP.getSdkVersion());
  display.print(F("Arduino Core: ")); display.println(ESP.getCoreVersion());
  display.println(F("Last Boot: ")); display.println(lastBoot);
  display.println(F("Last WiFi connection:")); display.println(lastWIFiConnection);
  display.println(F("Last MQTT connection:")); display.println(lastMQTTConnection);

  // add/remove 8 pixel for every line effectiveOffset+175, if you want to add a line effectiveOffset+183
  display.drawBitmap((((display.width()/2)-(arduinoLogoW/2))), effectiveOffset+175, arduinoLogo, arduinoLogoW, arduinoLogoH, 1);
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
    if (timedate == off_cmd) {
      setDateTime(timeConst);
      lastBoot = date + " " + currentime;
    } else {
      setDateTime(timeConst);
    }
    // lastMQTTConnection and lastWIFiConnection are resetted on every disconnection
    if (lastMQTTConnection == off_cmd) {
      lastMQTTConnection = date + " " + currentime;
    }
    if (lastWIFiConnection == off_cmd) {
      lastWIFiConnection = date + " " + currentime;
    }
    const char* haVersionConst = doc["haVersion"];
    haVersion = haVersionConst;
  }
  return true;
}

bool processDisplayCmnd(char* message) {
  if(strcmp(message, on_cmd) == 0) {
    stateOn = true;
  } else if(strcmp(message, off_cmd) == 0) {
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
  if(strcmp(message, on_cmd) == 0) {
    stepperMotorOn = true;
    stateOn = true;
  } else if(strcmp(message, off_cmd) == 0) {
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
  sendRebootState(off_cmd);
  if (rebootState == off_cmd) {      
    sendRebootCmnd();
  }
  return true;
}

bool processShowLastPageCmnd(char* message) {
  if(strcmp(message, on_cmd) == 0) {
    showLastPage = true;
    stateOn = true;
    sendMotorPowerState();
    sendPowerState();
  } else if(strcmp(message, off_cmd) == 0) {
    showLastPage = false;
  } 
  return true;
}

void setDateTime(const char* timeConst) {
  timedate = timeConst;
  date = timedate.substring(8,10) + "/" + timedate.substring(5,7) + "/" + timedate.substring(0,4);
  currentime = timedate.substring(11,16);
}

/********************************** SEND STATE *****************************************/
void sendPowerState() {
  drawOrShutDownDisplay();
  client.publish(watchwinder_state_topic, (stateOn) ? on_cmd : off_cmd, true);
}

void sendMotorPowerState() {
  client.publish(watchwinder_stat_power, (stepperMotorOn) ? on_cmd : off_cmd, true);
}

void sendPowerStateCmnd() {
  client.publish(watchwinder_cmnd_topic, (stateOn) ? on_cmd : off_cmd, true);
}

void sendMotorPowerStateCmnd() {
  client.publish(watchwinder_cmnd_power, (stepperMotorOn) ? on_cmd : off_cmd, true);
}

void sendInfoState() {
  StaticJsonDocument<BUFFER_SIZE> doc;

  JsonObject root = doc.to<JsonObject>();

  root["Whoami"] = SENSORNAME;
  root["IP"] = WiFi.localIP().toString();
  root["MAC"] = WiFi.macAddress();
  root["ver"] = VERSION;

  root["State"] = (stateOn) ? on_cmd : off_cmd;
  root["Time"] = timedate;
  root["rotation_done"] = numbersOfRotationDone;

  char buffer[measureJson(root) + 1];
  serializeJson(root, buffer, sizeof(buffer));

  // publish state only if it has received time from HA
  if (timedate != off_cmd) {
    client.publish(watchwinder_info_topic, buffer, true);
  }
}

void sendRebootState(const char* onOff) {   
  client.publish(watchwinder_stat_reboot, onOff, true);
}

void sendRebootCmnd() {   
  delay(delay_1500);
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

/********************************** START MQTT RECONNECT*****************************************/
void mqttReconnect() {
  // how many attemps to MQTT connection
  int brokermqttcounter = 0;
  // Loop until we're reconnected
  while (!client.connected()) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0,0);
    if (brokermqttcounter <= 20) {
      display.println(F("Connecting to"));
      display.println(F("MQTT Broker..."));
    } 
    // display.println(F("MQTT Broker...");
    display.display();

    // Attempt to connect
    if (client.connect(SENSORNAME, mqtt_username, mqtt_password)) {
      Serial.println(F("connected"));
      display.println(F(""));
      display.println(F("CONNECTED"));
      display.println(F(""));
      display.println(F("Reading data from"));
      display.println(F("the network..."));
      display.display();
      client.subscribe(smartostat_climate_state_topic);
      client.subscribe(watchwinder_cmnd_reboot);   
      client.subscribe(watchwinder_cmnd_showlastpage);            
      client.subscribe(watchwinder_cmnd_topic);
      client.subscribe(watchwinder_cmnd_power);
      client.subscribe(watchwinder_settings);

      delay(delay_2000);
      brokermqttcounter = 0;
      // reset the lastMQTTConnection to off, will be initialized by next time update
      lastMQTTConnection = off_cmd;
    } else {
      display.println(F("Number of attempts="));
      display.println(brokermqttcounter);
      display.display();
      // after 10 attemps all peripherals are shut down
      if (brokermqttcounter >= MAX_RECONNECT) {
        display.println(F("Max retry reached, powering off peripherals."));
        display.display();
        // shut down stepper motor if wifi disconnects
        stepperMotorOn = false;
        writeStep(arrayDefault);        
      } else if (brokermqttcounter > 10000) {
        brokermqttcounter = 0;
      }
      brokermqttcounter++;
      // Wait 5 seconds before retrying
      delay(500);
    }
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
    delay(delay_4000);
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

// https://stackoverflow.com/questions/9072320/split-string-into-string-array
String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}


/********************************** START SETUP WIFI *****************************************/
void setup_wifi() {

  unsigned int reconnectAttemp = 0;

  // DPsoftware domotics
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(5,17);
  display.drawRoundRect(0, 0, display.width()-1, display.height()-1, display.height()/4, WHITE);
  display.println(F("DPsoftware domotics"));
  display.display();

  delay(delay_3000);

  // Read config.json from SPIFFS
  readConfigFromSPIFFS();

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println(F("Connecting to: "));
  display.print(ssid); display.println(F("..."));
  display.display();

  Serial.println();
  Serial.print(F("Connecting to "));
  Serial.print(ssid);  

  delay(delay_2000);

  WiFi.persistent(false);   // Solve possible wifi init errors (re-add at 6.2.1.16 #4044, #4083)
  WiFi.disconnect(true);    // Delete SDK wifi config
  delay(200);
  WiFi.mode(WIFI_STA);      // Disable AP mode
  //WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setAutoConnect(true);
  // IP of the arduino, dns, gateway
  WiFi.config(arduinoip, mydns, mygateway);

  WiFi.hostname(SENSORNAME);

  // Set wifi power in dbm range 0/0.25, set to 0 to reduce PIR false positive due to wifi power
  WiFi.setOutputPower(0);

  WiFi.begin(ssid, password);

  // loop here until connection
  while (WiFi.status() != WL_CONNECTED) {
    
    delay(500);
    Serial.print(F("."));
    reconnectAttemp++;
    if (reconnectAttemp > 10) {
      display.setCursor(0,0);
      display.clearDisplay();
      display.print(F("Reconnect attemp= "));
      display.println(reconnectAttemp);
      if (reconnectAttemp >= MAX_RECONNECT) {
        display.println(F("Max retry reached, powering off peripherals."));
        // shut down stepper motor if wifi disconnects
        stepperMotorOn = false;
        writeStep(arrayDefault); 
      }
      display.display();
    } else if (reconnectAttemp > 10000) {
      reconnectAttemp = 0;
    }
  }

  display.println(F("WIFI CONNECTED"));
  display.println(WiFi.localIP());
  display.display();

  // reset the lastWIFiConnection to off, will be initialized by next time update
  lastWIFiConnection = off_cmd;

  delay(delay_1500);
}

/*
   Return the quality (Received Signal Strength Indicator) of the WiFi network.
   Returns a number between 0 and 100 if WiFi is connected.
   Returns -1 if WiFi is disconnected.
*/
int getQuality() {
  if (WiFi.status() != WL_CONNECTED)
    return -1;
  int dBm = WiFi.RSSI();
  if (dBm <= -100)
    return 0;
  if (dBm >= -50)
    return 100;
  return 2 * (dBm + 100);
}

// Blink LED_BUILTIN without bloking delay
void nonBlokingBlink() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval && ledTriggered) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    // blink led
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    blinkCounter++;
    if (blinkCounter >= blinkTimes) {
      blinkCounter = 0;
      ledTriggered = false;
      digitalWrite(LED_BUILTIN, HIGH);
    }
  }  
}

/********************************** START SETUP*****************************************/
void setup() {
  Serial.begin(serialRate);

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

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  //OTA SETUP
  ArduinoOTA.setPort(OTAport);
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(SENSORNAME);

  // No authentication by default
  ArduinoOTA.setPassword((const char *)OTApassword);

  ArduinoOTA.onStart([]() {
    Serial.println(F("Starting"));
  });
  ArduinoOTA.onEnd([]() {
    Serial.println(F("\nEnd"));
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println(F("Auth Failed"));
    else if (error == OTA_BEGIN_ERROR) Serial.println(F("Begin Failed"));
    else if (error == OTA_CONNECT_ERROR) Serial.println(F("Connect Failed"));
    else if (error == OTA_RECEIVE_ERROR) Serial.println(F("Receive Failed"));
    else if (error == OTA_END_ERROR) Serial.println(F("End Failed"));
  });
  ArduinoOTA.begin();

  Serial.println(F("Ready"));
  Serial.print(F("IP Address: "));
  Serial.println(WiFi.localIP());

}

/********************************** START MAIN LOOP *****************************************/
// Screen drawing call is done outside the loop for non blocking low delay loop needed to drive stepper motor without rattling
void loop() {  
  // Wifi management
  if (WiFi.status() != WL_CONNECTED) {
    delay(1);
    Serial.print(F("WIFI Disconnected. Attempting reconnection."));
    setup_wifi();
    return;
  }

  ArduinoOTA.handle();

  if (!client.connected()) {
    mqttReconnect();
  }
  client.loop();

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
    stepsLeft == 0;
    moving = false;
    // shut down stepper motor
    writeStep(arrayDefault);
    if (!stateOn) {
      display.clearDisplay();
      display.display();
    }
  }

  nonBlokingBlink();

}