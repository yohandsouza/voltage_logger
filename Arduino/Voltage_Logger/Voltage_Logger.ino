/*
  SD card datalogger

  This example shows how to log data from three analog sensors
  to an SD card using the SD library.

  The circuit:
   analog sensors on analog ins 0, 1, and 2
   SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 4 (for MKRZero SD: SDCARD_SS_PIN)

  created  24 Nov 2010
  modified 9 Apr 2012
  by Tom Igoe

  This example code is in the public domain.

*/

//#include <SPI.h>
//#include <SD.h>
#include "SdFat.h"
#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <TimeLib.h>
#include <DS1307RTC.h>
#include "oled_lib.h"
#include <EEPROM.h>

SdFat SD;

//#include "font1.h"

Adafruit_ADS1115 ads;

//#define dbg

//--------modifiable parameters-----------//

const uint8_t chipSelect = 4;
const uint8_t samples = 5;              //no of samples per read
const uint8_t sample_delay = 5;       //time duration between samples
uint16_t log_interval = 10 * 1000;  //file voltage log interval
char filename[10] ;//= "vtlog.csv";
const uint16_t r1_k = 100;
const uint16_t r2_k = 100;
//--------modifiable parameters end-----------//

unsigned long int start_time = 0;
const char eeprom_addr = 0;
uint16_t file_num = 0;
const char comma_char = ',';
const char null_char = '\0';
uint32_t  timestamp = 0;
unsigned long prev_log_instance;
//unsigned long prev_led_on_instance;
uint32_t raw_value = 0;
uint16_t vtg_1;
uint16_t vtg_2;
uint16_t vtg_3;
//uint16_t actual_vtg;
String dataString = "";
const uint8_t resistor_ratio =  (r1_k + r2_k) / r2_k;
//const uint16_t OPERATING_VTG = 5000;
//bool led_state = false;
//String start_time_date = "";
void setup() {
  // Open serial communications and wait for port to open:
#ifdef dbg
  Serial.begin(9600);
#endif
  oled_begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  oled_clearDisplay();
  oled_setTextColor(SSD1306_WHITE);
  oled_setCursor(0, 0);
  oled_print_f(F("Initializing SD card"));
  oled_display();
#ifdef dbg
  Serial.print(F("Initializing SD card..."));
#endif
  // see if the card is present and can be initialized:
  //if (true){
  if (!SD.begin(chipSelect)) {
#ifdef dbg
    Serial.println(F("Card failed, or not present"));
#endif
    oled_print_f(F("\nCard failed, or not present\nCheck SD card and then reset"));
    oled_display();
    while (1)
    {}
    // don't do anything more:
    //error_blink_pattern();
  }
  //tmElements_t tm;
  //RTC.read(tm);
  //start_time_date = (String)tm.Day + "-" + (String)tm.Month + "-" + (String)tm.Year + " " + (String)tm.Hour + ":" + (String)tm.Minute + ":" + (String)tm.Second;
  //start_time_date = tm.Day + "-" + tm.Month + "-" +tm.Year + " " + tm.Hour + ":" + tm.Minute + ":" + tm.Second;

  //  EEPROM.write(eeprom_addr, 0);
  //  EEPROM.write(eeprom_addr + 1, 0);
  file_num =  EEPROM.read(eeprom_addr + 1);
  file_num = (file_num << 8) | EEPROM.read(eeprom_addr);
  sprintf(filename, "%05d", file_num);
  //filename+=".csv";
  strcat(filename, ".csv");
#ifdef dbg
  Serial.println("card initialized.");
#endif
  oled_print_f(F("\nCard initialized."));
  oled_display();
  delay(1000);
  //oled_clearDisplay();
  //get_vtg();
  //ads.begin();
  File dataFile = SD.open(filename, FILE_WRITE);
  if (dataFile) {
    //dataFile.println("Time(mins),Pin Vtg,Actual Vtg,,R1(K)=" + (String)r1_k + ",R2(K)=" +  (String)r2_k + ",Log interval=" +  (String)log_interval + "ms");
    //dataFile.println(start_time_date);
    dataFile.print(F("Time(mins),Vtg1,Vtg 2, Vtg 3 ,,R1(K)=" ));
    dataFile.print(r1_k );
    dataFile.print(F(",R2(K)="));
    dataFile.print(r2_k);
    dataFile.print(F(",Log interval="));
    dataFile.print(log_interval );
    dataFile.print(F( "ms\n"));
    dataFile.close();
    oled_clearDisplay();
    oled_setCursor(0, 0);
    oled_print_f(F("File opened"));
    oled_display();
    file_num++;
    EEPROM.write(eeprom_addr, (file_num & 0xFF));
    EEPROM.write(eeprom_addr + 1, ((file_num & 0xFF00) >> 8));
  }
  else
  {

    log_data();
    oled_clearDisplay();
    oled_setCursor(0, 0);
    oled_print_f(F("Could not open file"));
    oled_display();
    while (1) {}
  }
  //oled_clearDisplay();
  oled_print_f(F("\nReady"));
  oled_display();
  delay(1000);
  prev_log_instance = millis();
  oled_clearDisplay();
  oled_setCursor(0, 0);
  oled_print_f(F("Logging:- "));
  oled_print_c_str(filename);
  //oled_print_f(F("\nStart:- "));
  //oled_print_str(start_time_date.c_str(), sizeof(start_time_date));
  oled_print_f(F("\n\n"));
  get_vtg_1();
  get_vtg_2();
  get_vtg_3();
  log_data();
}



void loop() {
  //blink_status_led();
  if (millis() - prev_log_instance >= log_interval)
  {
    prev_log_instance = millis();
    oled_clearDisplay();
    oled_setCursor(0, 0);
    oled_print_f(F("Logging:- "));
    oled_print_c_str(filename);
    //oled_print_f(F("\nStart:- "));
    //oled_print_str(start_time_date.c_str(), sizeof(start_time_date));
    oled_print_f(F("\n\n"));
    get_vtg_1();
    get_vtg_2();
    get_vtg_3();
    //bool log_success = log_data();
    timestamp = prev_log_instance - start_time;
    if (!log_data()) { //if (!log_success) {
#ifdef dbg
      Serial.println(F("error opening datalog.txt"));
#endif
      oled_clearDisplay();
      oled_setCursor(0, 0);
      oled_print_f(F("error opening "));
      oled_print_c_str(filename);
      oled_display();
      error_blink_pattern();
    }
    else
    {
      /*oled_clearDisplay();
        oled_setCursor(0, 0);
        oled_print_f(F("Log"));
        oled_display();*/
    }
  }
}

bool log_data()
{
  File dataFile = SD.open(filename, FILE_WRITE);
  dataString = null_char;
  if (dataFile) {
    dataString += timestamp;
    dataString += comma_char;
    dataString += vtg_1;
    dataString += comma_char;
    dataString += vtg_2;
    dataString += comma_char;
    dataString += vtg_3;
    //dataString += comma_char;
    //dataString += actual_vtg;
    dataString += null_char;
    dataFile.println(dataString);
    dataFile.close();
    return true;
  }
  else
  {
    return false;
  }
}

void get_vtg_1()
{
  raw_value = 0;
  float multiplier = 0.1875F;
  for (uint8_t i = 0; i < samples; i++)
  {
    //raw_value += analogRead(sense_pin);
    raw_value += ads.readADC_SingleEnded(0);
    delay(sample_delay);
  }
  raw_value = raw_value / samples;
  vtg_1 = raw_value * multiplier; //map(raw_value, 0, 65536 , 0, 5000);
  //oled_clearDisplay();
  //oled_setCursor(0, 0);
  oled_print_f(F("A0 "));
  oled_print_int(vtg_1);
  oled_print_f(F(" mV"));
  //oled_display();
  //actual_vtg = pin_vtg * resistor_ratio;
}

void get_vtg_2()
{
  raw_value = 0;
  float multiplier = 0.1875F;
  for (uint8_t i = 0; i < samples; i++)
  {
    //raw_value += analogRead(sense_pin);
    raw_value += ads.readADC_SingleEnded(1);
    delay(sample_delay);
  }
  raw_value = raw_value / samples;
  vtg_2 = raw_value * multiplier; //map(raw_value, 0, 65536 , 0, 5000);
  //oled_clearDisplay();
  //oled_setCursor(0, 0);
  oled_print_f(F("\nA1 "));
  oled_print_int(vtg_2);
  oled_print_f(F(" mV"));
  //oled_display();
  //actual_vtg = pin_vtg * resistor_ratio;
}
void get_vtg_3()
{
  raw_value = 0;
  float multiplier = 0.1875F;
  for (uint8_t i = 0; i < samples; i++)
  {
    //raw_value += analogRead(sense_pin);
    raw_value += ads.readADC_SingleEnded(2);
    delay(sample_delay);
  }
  raw_value = raw_value / samples;
  vtg_3 = raw_value * multiplier; //map(raw_value, 0, 65536 , 0, 5000);
  //oled_clearDisplay();
  //oled_setCursor(0, 0);
  oled_print_f(F("\nA2 "));
  oled_print_int(vtg_3);
  oled_print_f(F(" mV"));
  oled_display();
  //actual_vtg = pin_vtg * resistor_ratio;
}
void blink_status_led()
{
  /*if ((millis() - prev_led_on_instance >= 1000) && (led_state == false))
    {
    prev_led_on_instance = millis();
    turn_on_led();
    }
    if ((led_state == true) && (millis() - prev_led_on_instance >= 100))
    {
    turn_off_led();
    }*/
}

void error_blink_pattern()
{
  /*while (1)
    {
    turn_on_led();
    delay(250);
    turn_off_led();
    delay(250);
    }*/
}


void turn_on_led()
{
  //digitalWrite(led_pin, HIGH);
  //led_state = true;
}

void turn_off_led()
{
  //digitalWrite(led_pin, LOW);
  //led_state = false;
}
