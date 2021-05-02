/***************************************************************************
  This is a library for the AMG88xx GridEYE 8x8 IR camera

  This sketch makes a 64 pixel thermal camera with the GridEYE sensor
  and a 128x128 tft screen https://www.adafruit.com/product/2088

  Designed specifically to work with the Adafruit AMG88 breakout
  ----> http://www.adafruit.com/products/3538

  These sensors use I2C to communicate. The device's I2C address is 0x69

  Adafruit invests time and resources providing this open source code,
  please support Adafruit andopen-source hardware by purchasing products
  from Adafruit!

  Written by Dean Miller for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ***************************************************************************/

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>


#include <Wire.h>
#include <Adafruit_AMG88xx.h>
#include <SR04.h>
#include <LiquidCrystal.h>
#include <SD.h>
#include<SoftwareSerial.h>
#include <ArduinoJson.h>

#define TFT_CS     53 //chip select pin for the TFT screen
#define TFT_RST    9  // you can also connect this to the Arduino reset
                      // in which case, set this #define pin to 0!
#define TFT_DC     8

//low range of the sensor (this will be blue on the screen)
#define MINTEMP 22

//high range of the sensor (this will be red on the screen)
#define MAXTEMP 34

//SR04
#define trig 12
#define echo 11
SR04 sr04 = SR04(echo,trig);
long dist;

//send to firebase nodemcu
SoftwareSerial SUART(15,14);

//LCD 16X2
LiquidCrystal lcd(27, 28, 29, 30, 31, 32);
File file;
int cardpin = 4;

//the colors we will be using
const uint16_t camColors[] = {0x480F,
0x400F,0x400F,0x400F,0x4010,0x3810,0x3810,0x3810,0x3810,0x3010,0x3010,
0x3010,0x2810,0x2810,0x2810,0x2810,0x2010,0x2010,0x2010,0x1810,0x1810,
0x1811,0x1811,0x1011,0x1011,0x1011,0x0811,0x0811,0x0811,0x0011,0x0011,
0x0011,0x0011,0x0011,0x0031,0x0031,0x0051,0x0072,0x0072,0x0092,0x00B2,
0x00B2,0x00D2,0x00F2,0x00F2,0x0112,0x0132,0x0152,0x0152,0x0172,0x0192,
0x0192,0x01B2,0x01D2,0x01F3,0x01F3,0x0213,0x0233,0x0253,0x0253,0x0273,
0x0293,0x02B3,0x02D3,0x02D3,0x02F3,0x0313,0x0333,0x0333,0x0353,0x0373,
0x0394,0x03B4,0x03D4,0x03D4,0x03F4,0x0414,0x0434,0x0454,0x0474,0x0474,
0x0494,0x04B4,0x04D4,0x04F4,0x0514,0x0534,0x0534,0x0554,0x0554,0x0574,
0x0574,0x0573,0x0573,0x0573,0x0572,0x0572,0x0572,0x0571,0x0591,0x0591,
0x0590,0x0590,0x058F,0x058F,0x058F,0x058E,0x05AE,0x05AE,0x05AD,0x05AD,
0x05AD,0x05AC,0x05AC,0x05AB,0x05CB,0x05CB,0x05CA,0x05CA,0x05CA,0x05C9,
0x05C9,0x05C8,0x05E8,0x05E8,0x05E7,0x05E7,0x05E6,0x05E6,0x05E6,0x05E5,
0x05E5,0x0604,0x0604,0x0604,0x0603,0x0603,0x0602,0x0602,0x0601,0x0621,
0x0621,0x0620,0x0620,0x0620,0x0620,0x0E20,0x0E20,0x0E40,0x1640,0x1640,
0x1E40,0x1E40,0x2640,0x2640,0x2E40,0x2E60,0x3660,0x3660,0x3E60,0x3E60,
0x3E60,0x4660,0x4660,0x4E60,0x4E80,0x5680,0x5680,0x5E80,0x5E80,0x6680,
0x6680,0x6E80,0x6EA0,0x76A0,0x76A0,0x7EA0,0x7EA0,0x86A0,0x86A0,0x8EA0,
0x8EC0,0x96C0,0x96C0,0x9EC0,0x9EC0,0xA6C0,0xAEC0,0xAEC0,0xB6E0,0xB6E0,
0xBEE0,0xBEE0,0xC6E0,0xC6E0,0xCEE0,0xCEE0,0xD6E0,0xD700,0xDF00,0xDEE0,
0xDEC0,0xDEA0,0xDE80,0xDE80,0xE660,0xE640,0xE620,0xE600,0xE5E0,0xE5C0,
0xE5A0,0xE580,0xE560,0xE540,0xE520,0xE500,0xE4E0,0xE4C0,0xE4A0,0xE480,
0xE460,0xEC40,0xEC20,0xEC00,0xEBE0,0xEBC0,0xEBA0,0xEB80,0xEB60,0xEB40,
0xEB20,0xEB00,0xEAE0,0xEAC0,0xEAA0,0xEA80,0xEA60,0xEA40,0xF220,0xF200,
0xF1E0,0xF1C0,0xF1A0,0xF180,0xF160,0xF140,0xF100,0xF0E0,0xF0C0,0xF0A0,
0xF080,0xF060,0xF040,0xF020,0xF800,};

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);

Adafruit_AMG88xx amg;
unsigned long delayTime;
float pixels[AMG88xx_PIXEL_ARRAY_SIZE];
uint16_t displayPixelWidth, displayPixelHeight;

int randomno;
void setup() {
  Serial.begin(9600);
  Serial1.begin(115200);
    SUART.begin(115200); //enable SUART Port
    Serial.println(F("AMG88xx thermal camera!"));

    tft.initR(INITR_144GREENTAB);   // initialize a ST7735S chip, black tab
    tft.fillScreen(ST7735_BLACK);

    displayPixelWidth = tft.width() / 8;
    displayPixelHeight = tft.height() / 8;

    //tft.setRotation(3);
    
    bool status;
    
    // default settings
    status = amg.begin();
    if (!status) {
        Serial.println("Could not find a valid AMG88xx sensor, check wiring!");
        while (1);
    }
    
    Serial.println("-- Thermal Camera Test --");
    lcd.begin(16, 2);
    delay(100); // let sensor boot up
    pinMode(cardpin, OUTPUT); 
    if (!SD.begin(cardpin)) { 
      Serial.println("SD card initialization failed");
    }
  
    if (SD.exists("data.txt")) {
      Serial.println("data.txt exists.");
      if (SD.remove("data.txt")) {
        Serial.println("Successfully removed file.");
      } else {
        Serial.println("Could not remove data file.");
      }
    }

}

void loop() {
  String pixelarray;
  //read all the pixels
  amg.readPixels(pixels);
float maxx = -99.9;
  for(int i=0; i<AMG88xx_PIXEL_ARRAY_SIZE; i++){
    uint8_t colorIndex = map(pixels[i], MINTEMP, MAXTEMP, 0, 255);
    colorIndex = constrain(colorIndex, 0, 255);

    //draw the pixels!
    tft.fillRect(displayPixelHeight * floor(i / 8), displayPixelWidth * (i % 8),
        displayPixelHeight, displayPixelWidth, camColors[colorIndex]);
        if(pixels[i]>maxx){
  maxx = pixels[i];
}
  }
  
  dist=sr04.Distance();
if(dist>40){
  Serial.println("Come closer");
  lcd.print("Come closer");

}
else{
  if(dist<6){
    Serial.println("Go farther");
    lcd.print("Go farther");
  }
  else{
      Serial.print(dist);
      Serial.println(" cm");
      float newmax = round((maxx+5.25+(dist/9))*100);
      float degreesF = newmax/100;
      String temperaturetext = String(degreesF);
      Serial.println(temperaturetext);
      lcd.setCursor(0, 0);
      lcd.print("Temp: "+String(degreesF));
      Serial1.println(degreesF);
      lcd.setCursor(0,1);
      if(degreesF < 37){
        lcd.println("Person is safe");} 
      else{
        lcd.println("Person go to ER");}
        delay(3000);
        sendtofirebase(true); 
    }
     
}
delay(1000);
lcd.clear();
}
void sendtofirebase(bool checkdetect){
   DynamicJsonDocument doc(1024);
   if (checkdetect == true)
   {
      for(int i=1; i<=AMG88xx_PIXEL_ARRAY_SIZE; i++){
      float temppixel = pixels[i-1];
      float newtemppixel = ((temppixel+5.25+(dist/9)));
      doc[i-1] = newtemppixel;
      }
   }
   serializeJson(doc, SUART);
   //end send to nodemcu(firebase)
   delay(3000);
  }
