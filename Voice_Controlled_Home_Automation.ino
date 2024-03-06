/*Title: Initial Commit: Voice-Controlled Home Automation System

Description:
- Added initial files for Arduino IDE integration.
//- Included basic functionality for device communication and control.
//- Integrated voice recognition module for command interpretation.
//- Enabled basic device interaction via voice commands.


Co-authored-by: AStromanish*/

//Uncomment for Debugging 
 //#define ENABLE_DEBUG 
  
 #ifdef ENABLE_DEBUG 
        #define DEBUG_ESP_PORT Serial 
        #define NODEBUG_WEBSOCKETS 
        #define NDEBUG 
 #endif  
  
 #include <Arduino.h> 
 #ifdef ESP8266  
        #include <ESP8266WiFi.h> 
 #endif  
 #ifdef ESP32    
        #include <WiFi.h> 
 #endif 
  
 #include "SinricPro.h" 
 #include "SinricProSwitch.h" 
  
 #include <map> 
  //Replace the following with your Credential 

 #define WIFI_SSID         "astromanish"     
 #define WIFI_PASS         "astronaut" 
 #define APP_KEY           "6df1e244-aa2a-4cdd-8ad3-4e2ff7bfab63"      // Should look like "de0bxxxx-1x3x-4x3x-ax2x-5dabxxxxxxxx" 
 #define APP_SECRET        "9a47cd59-24b2-4e44-8f5b-d16305ffc761-dad6d070-a107-4ac6-8fd1-790ceac0689c"   // Should look like "5f36xxxx-x3x7-4x3x-xexe-e86724a9xxxx-4c4axxxx-3x3x-x5xe-x9x3-333d65xxxxxx" 
  
 // comment the following line if you use a toggle switches instead of tactile buttons 
 //#define TACTILE_BUTTON 1 
  
 #define BAUD_RATE   9600 
  
 #define DEBOUNCE_TIME 250 
  
 typedef struct {      // struct for the std::map below 
   int relayPIN; 
   int flipSwitchPIN; 
 } deviceConfig_t; 
  
  
 // this is the main configuration 
 // please put in your deviceId, the PIN for Relay and PIN for flipSwitch 
 // this can be up to N devices...depending on how much pin's available on your device ;) 
 // right now we have 4 devicesIds going to 4 relays and 4 flip switches to switch the relay manually 
 std::map<String, deviceConfig_t> devices = { 
     //{deviceId, {relayPIN,  flipSwitchPIN}} 
     {"", {  D1, D8 }},  //Fill out the Device Key no inside ""
     {"", {  D2, D7 }},  //Fill out the Device Key no inside ""
     {"", {  D3, D6 }},  //Fill out the Device Key no inside ""
     {"SWITCH_ID_NO_4_HERE", {  D4, D5 }}       
 }; 
  
 typedef struct {      // struct for the std::map below 
   String deviceId; 
   bool lastFlipSwitchState; 
   unsigned long lastFlipSwitchChange; 
 } flipSwitchConfig_t; 
  
 std::map<int, flipSwitchConfig_t> flipSwitches;    // this map is used to map flipSwitch PINs to deviceId and handling debounce and last flipSwitch state checks 
                                                   // it will be setup in "setupFlipSwitches" function, using informations from devices map 
  
 void setupRelays() {  
   for (auto &device : devices) {           // for each device (relay, flipSwitch combination) 
     int relayPIN = device.second.relayPIN; // get the relay pin 
     pinMode(relayPIN, OUTPUT);             // set relay pin to OUTPUT 
   } 
 } 
  
 void setupFlipSwitches() { 
   for (auto &device : devices)  {                     // for each device (relay / flipSwitch combination) 
     flipSwitchConfig_t flipSwitchConfig;              // create a new flipSwitch configuration 
  
     flipSwitchConfig.deviceId = device.first;         // set the deviceId 
     flipSwitchConfig.lastFlipSwitchChange = 0;        // set debounce time 
     flipSwitchConfig.lastFlipSwitchState = false;     // set lastFlipSwitchState to false (LOW) 
  
     int flipSwitchPIN = device.second.flipSwitchPIN;  // get the flipSwitchPIN 
  
     flipSwitches[flipSwitchPIN] = flipSwitchConfig;   // save the flipSwitch config to flipSwitches map 
     pinMode(flipSwitchPIN, INPUT);                   // set the flipSwitch pin to INPUT 
   } 
 } 
  
 bool onPowerState(String deviceId, bool &state) 
 { 
   Serial.printf("%s: %s\r\n", deviceId.c_str(), state ? "on" : "off"); 
   int relayPIN = devices[deviceId].relayPIN; // get the relay pin for corresponding device 
   digitalWrite(relayPIN, !state);             // set the new relay state 
   return true; 
 } 
  
 void handleFlipSwitches() { 
   unsigned long actualMillis = millis();                                          // get actual millis 
   for (auto &flipSwitch : flipSwitches) {                                         // for each flipSwitch in flipSwitches map 
     unsigned long lastFlipSwitchChange = flipSwitch.second.lastFlipSwitchChange;  // get the timestamp when flipSwitch was pressed last time (used to debounce / limit events) 
  
     if (actualMillis - lastFlipSwitchChange > DEBOUNCE_TIME) {                    // if time is > debounce time... 
  
       int flipSwitchPIN = flipSwitch.first;                                       // get the flipSwitch pin from configuration 
       bool lastFlipSwitchState = flipSwitch.second.lastFlipSwitchState;           // get the lastFlipSwitchState 
       bool flipSwitchState = digitalRead(flipSwitchPIN);                          // read the current flipSwitch state 
       if (flipSwitchState != lastFlipSwitchState) {                               // if the flipSwitchState has changed... 
 #ifdef TACTILE_BUTTON 
         if (flipSwitchState) {                                                    // if the tactile button is pressed  
 #endif       
           flipSwitch.second.lastFlipSwitchChange = actualMillis;                  // update lastFlipSwitchChange time 
           String deviceId = flipSwitch.second.deviceId;                           // get the deviceId from config 
           int relayPIN = devices[deviceId].relayPIN;                              // get the relayPIN from config 
           bool newRelayState = !digitalRead(relayPIN);                            // set the new relay State 
           digitalWrite(relayPIN, newRelayState);                                  // set the trelay to the new state 
  
           SinricProSwitch &mySwitch = SinricPro[deviceId];                        // get Switch device from SinricPro 
           mySwitch.sendPowerStateEvent(!newRelayState);                            // send the event 
 #ifdef TACTILE_BUTTON 
         } 
 #endif       
         flipSwitch.second.lastFlipSwitchState = flipSwitchState;                  // update lastFlipSwitchState 
       } 
     } 
   } 
 } 
  
 void setupWiFi() 
 { 
   Serial.printf("\r\n[Wifi]: Connecting"); 
   WiFi.begin(WIFI_SSID, WIFI_PASS); 
  
   while (WiFi.status() != WL_CONNECTED) 
   { 
     Serial.printf("."); 
     delay(250); 
   } 
   digitalWrite(LED_BUILTIN, HIGH); 
   Serial.printf("connected!\r\n[WiFi]: IP-Address is %s\r\n", WiFi.localIP().toString().c_str()); 
 } 
  
 void setupSinricPro() 
 { 
   for (auto &device : devices) 
   { 
     const char *deviceId = device.first.c_str(); 
     SinricProSwitch &mySwitch = SinricPro[deviceId]; 
     mySwitch.onPowerState(onPowerState); 
   } 
  
   SinricPro.begin(APP_KEY, APP_SECRET); 
   SinricPro.restoreDeviceStates(true); 
 } 
  
 void setup() 
 { 
   Serial.begin(BAUD_RATE); 
   setupRelays(); 
   setupFlipSwitches(); 
   setupWiFi(); 
   setupSinricPro(); 
 } 
  
 void loop() 
 { 
   SinricPro.handle(); 
   handleFlipSwitches(); 
 } 
