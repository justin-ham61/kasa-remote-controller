#include <WiFi.h>
#include "KasaSmartPlug.h"
#include "config.h"
#include "init.h"

int PORT_NUMBER = 18;
int port_state = 0;
int bulb_state = 0;

const char* dev_ip = "192.168.1.46";

int currState = 0;
int currBrightness;
bool button = false;

unsigned long lastDebounceTime = 0; 
const unsigned long debounceDelay = 1000;

KASAUtil kasaUtil;

void connectToWifi(){
  int found;
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(".");
  }
  Serial.println("Connected to the WiFi network");
  found = kasaUtil.ScanDevices();
  Serial.printf("\r\n Found device = %d", found);

  // Print out devices name and ip address found..
  for (int i = 0; i < found; i++)
  {
    KASADevice *p = kasaUtil.GetSmartPlugByIndex(i);
    Serial.printf("\r\n %d. %s IP: %s Relay: %d", i, p->alias, p->ip_address, p->state);  
  } 
}

void IRAM_ATTR onButtonPress(){
  unsigned long currentTime = millis();
  if ((currentTime - lastDebounceTime) > debounceDelay) {
    // Set the button pressed flag
    button = true;

    // Update the last debounce time
    lastDebounceTime = currentTime;
  }
}

void handleButtonPress(){
  kasaUtil.CreateAndDeliver(dev_ip, bulb_state, "bulb");
  if(bulb_state == 0){
    bulb_state = 1;
  } else {
    bulb_state = 0;
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  
  pinMode(PORT_NUMBER, INPUT_PULLUP);
  attachInterrupt(PORT_NUMBER, onButtonPress, FALLING);

  connectToWifi();

  int length = sizeof(ips)/sizeof(ips[0]);
  for(int i = 0; i < length; i++){
    Serial.println(ips[i]);
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  if(button){
    button = false;
    handleButtonPress();
    Serial.println("Pressed");
  }
}
