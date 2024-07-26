#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <set>
#include "KasaSmartPlug.h"
#include "config.h"
#include "./structs/device.h"
#include "AiEsp32RotaryEncoder.h"
#include "AiEsp32RotaryEncoderNumberSelector.h"
#include "Arduino.h"
#include "bitmap.h"

//Font Imports for Display
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>

#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(128,64, &Wire, -1);

//Menu Rotary Setting
#define MENU_ROTARY_ENCODER_DT_PIN      18
#define MENU_ROTARY_ENCODER_CLK_PIN     16
#define MENU_ROTARY_ENCODER_BUTTON_PIN  35

//Quick Rotary Setting
#define QUICK_ROTARY_ENCODER_DT_PIN     23
#define QUICK_ROTARY_ENCODER_CLK_PIN    17
#define QUICK_ROTARY_ENCODER_BUTTON_PIN 34
#define ROTARY_ENCODER_VCC_PIN          -1
#define ROTARY_ENCODER_STEPS            4

AiEsp32RotaryEncoder *rotaryEncoderQuick = new AiEsp32RotaryEncoder(QUICK_ROTARY_ENCODER_DT_PIN, QUICK_ROTARY_ENCODER_CLK_PIN, QUICK_ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);
AiEsp32RotaryEncoderNumberSelector numberSelectorQuick = AiEsp32RotaryEncoderNumberSelector();

AiEsp32RotaryEncoder rotaryEncoderMenu = AiEsp32RotaryEncoder(MENU_ROTARY_ENCODER_DT_PIN, MENU_ROTARY_ENCODER_CLK_PIN, MENU_ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);

//Bulb state for determining if bulb is on or off, initialized to off
int bulb_state = 0;

//Initializing brightness to 50 and cofing for Encoder
int currBrightness = 49;
bool brightnessChanged = false;

int individualBrightness = 0;
bool individualBrightnessChanged = false;

unsigned long lastChanged;

bool button = false;

//Debounce delay for handling button bounce
unsigned long lastDebounceTime = 0; 
const unsigned long debounceDelay = 1000;

//Which display is currently being shown
//0: Main Menu
//1: Device Brightness Screen
//2: All device management screen
int displayType = 0;

//Menu Item selection variables
int selectedItem = 0;
int previousItem;
int nextItem;
KASASmartBulb* currentBulb;

KASAUtil kasaUtil;

void addFromConfig(UserDevice userdevices[]){
    for(int i = 0; i < 4; i++){
        kasaUtil.CreateDevice(userdevices[i].name, userdevices[i].ip, userdevices[i].type);
    }
} 

void connectToWifi(){
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(".");
  }
  Serial.println("Connected to the WiFi network");
}

void discoverDevices(){
  int found;
  found = kasaUtil.ScanDevices();
  Serial.printf("\r\n Found device = %d", found);

  // Print out devices name and ip address found..
  for (int i = 0; i < found; i++)
  {
    KASADevice *p = kasaUtil.GetSmartPlugByIndex(i);
    Serial.printf("\r\n %d. %s IP: %s Relay: %d", i, p->alias, p->ip_address, p->state);  
  } 
}

void quick_rotary_onButtonClick(){
  unsigned long currentTime = millis();
  if ((currentTime - lastDebounceTime) > debounceDelay) {
    // Set the button pressed flag
    kasaUtil.ToggleAll(bulb_state);
    if(bulb_state == 0){
      bulb_state = 1;
    } else {
      bulb_state = 0;
    }
    // Update the last debounce time
    lastDebounceTime = currentTime;
  }
}

void menu_rotary_onButtonClick(){
  unsigned long currentTime = millis();
  if ((currentTime - lastDebounceTime) > debounceDelay) {
    displayType = 1;
    currentBulb = static_cast<KASASmartBulb*>(kasaUtil.GetSmartPlugByIndex(selectedItem));
    individualBrightness = currentBulb->brightness;
    numberSelectorQuick.setValue(individualBrightness);
    lastChanged = millis();
    // Update the last debounce time
    lastDebounceTime = currentTime;
  }
}

void individual_brightness_onButtonClick(){
  unsigned long currentTime = millis();
  if((currentTime - lastDebounceTime) > debounceDelay){
    int currentState = currentBulb->state;
    if(currentState == 0){
      currentBulb->turnOn();
    } else {
      currentBulb->turnOff();
    }
  }
}

void quick_rotary_loop(){
    //dont print anything unless value changed
    if (rotaryEncoderQuick->encoderChanged())
    {
            displayType = 2;
            currBrightness = numberSelectorQuick.getValue();
            brightnessChanged = true;
            lastChanged = millis();
    }
    if (rotaryEncoderQuick->isEncoderButtonClicked())
    {
            quick_rotary_onButtonClick();
    }
}

void individual_brightness_rotary_loop(){
  if(rotaryEncoderQuick->encoderChanged()){
    individualBrightnessChanged = true;
    individualBrightness = numberSelectorQuick.getValue();
    lastChanged = millis();
  }
  if(rotaryEncoderQuick->isEncoderButtonClicked()){
    individual_brightness_onButtonClick();
  }
}

void menu_rotary_loop(){
    //dont print anything unless value changed
    if (rotaryEncoderMenu.encoderChanged())
    {
            selectedItem = rotaryEncoderMenu.readEncoder();
    }
    if (rotaryEncoderMenu.isEncoderButtonClicked())
    {
            menu_rotary_onButtonClick();
    }
}

void handleBrightnessChange(){
  //Change the brightness for all bulbs after .4 seconds of not turning the knob
  if(brightnessChanged && (millis() - lastChanged) > 400){
    brightnessChanged = false;
    kasaUtil.SetBrightnessAll(currBrightness);
    for(int i = 0; i < 3; i++){
      KASADevice* dev = kasaUtil.GetSmartPlugByIndex(i);
      KASASmartBulb* bulb = static_cast<KASASmartBulb*>(dev);
      bulb->brightness = currBrightness;
    }
  }

  //Return to main menu after 3 seconds of no activity on all brightness change
  if(millis() - lastChanged > 3000){
    displayType = 0;
  }
}

void handleIndividualBrightnessChange(){
  if(individualBrightnessChanged && (millis() - lastChanged) > 400){
    individualBrightnessChanged = false;
    currentBulb->setBrightness(individualBrightness);
  }
  if(millis() - lastChanged > 3000){
    displayType = 0;
  }
}

void IRAM_ATTR readEncoderISRQuick(){
  rotaryEncoderQuick->readEncoder_ISR();
}

void IRAM_ATTR readEncoderISRMenu(){
  rotaryEncoderMenu.readEncoder_ISR();
}

void main_menu_display_loop(){
  previousItem = selectedItem - 1;
  if(previousItem < 0){
    previousItem = 3 - 1;
  }
  nextItem = selectedItem + 1;
  if(nextItem >= 3){
    nextItem = 0;
  }

  display.clearDisplay();

  //Draws the background GUI 
  display.drawBitmap(4, 2,  bitmap_icon, 16, 16, 1);
  display.drawBitmap(4, 24,  bitmap_icon, 16, 16, 1);
  display.drawBitmap(4, 46,  bitmap_icon, 16, 16, 1);
  display.drawBitmap(0, 22, bitmap_item_sel_background, 128, 21, 1);
  display.drawBitmap(120, 0, bitmap_scrollbar_background, 8, 64, 1);

  display.setTextSize(1);
  display.setTextColor(WHITE);

  //Previous Item
  display.setFont(&FreeSans9pt7b);
  display.setCursor(26, 15);
  display.print(kasaUtil.GetSmartPlugByIndex(previousItem)->alias);

  //Current Item
  display.setFont(&FreeSansBold9pt7b);
  display.setCursor(26, 37);
  display.print(kasaUtil.GetSmartPlugByIndex(selectedItem)->alias);

  //Next Item
  display.setFont(&FreeSans9pt7b);
  display.setCursor(26, 59);
  display.print(kasaUtil.GetSmartPlugByIndex(nextItem)->alias);

  //Scroll position box
  display.fillRect(125, 64/3 * selectedItem, 3, 64/3, WHITE);

  //Display
  display.display();
}

void all_brightness_display_loop(){
  display.clearDisplay();
  display.setFont();
  display.setCursor(20, 20);
  display.print("Brightness: ");
  display.print(currBrightness);
  display.print("%");
  int barWidth = map(currBrightness, 0, 100, 0, 100);
  display.drawRect(14, 40, 100, 10, WHITE); // Draw the outline of the bar
  display.fillRect(14, 40, barWidth, 10, WHITE); // Fill the bar according to currBrightness
  display.display();
}

void individual_brightness_display_loop(){
  display.clearDisplay();
  display.setFont();
  display.setCursor(20, 20);
  display.print("Brightness: ");
  display.print(individualBrightness);
  display.print("%");
  int barWidth = map(individualBrightness, 0, 100, 0, 100);
  display.drawRect(14, 40, 100, 10, WHITE); // Draw the outline of the bar
  display.fillRect(14, 40, barWidth, 10, WHITE); // Fill the bar according to currBrightness
  display.display();
}

void setup() {
  //Initialize serial
  Serial.begin(115200);

  //Connect to wifi with ssid and password from config.h
  connectToWifi();

  //Quick Rotary encoder set up
  rotaryEncoderQuick->begin();
  rotaryEncoderQuick->setup(readEncoderISRQuick);
  numberSelectorQuick.attachEncoder(rotaryEncoderQuick);
  numberSelectorQuick.setRange(1, 99, 2, false, 0);
  numberSelectorQuick.setValue(currBrightness);
  rotaryEncoderQuick->disableAcceleration();

  //Menu Rotary encoder set up
  rotaryEncoderMenu.begin();
  rotaryEncoderMenu.setup(readEncoderISRMenu);
  rotaryEncoderMenu.setBoundaries(0,2,true);
  rotaryEncoderMenu.disableAcceleration();

  //Initiate devices from pre defined IP addresses and aliases
  addFromConfig(devices);

  //Display set up
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
}

void loop() {
  menu_rotary_loop();
  if(displayType == 0){
    main_menu_display_loop();
    quick_rotary_loop();
  } else if (displayType == 1){
    individual_brightness_display_loop();
    individual_brightness_rotary_loop();
    handleIndividualBrightnessChange();
  } else if (displayType == 2){
    all_brightness_display_loop();
    quick_rotary_loop();
    handleBrightnessChange();
  }
}
 