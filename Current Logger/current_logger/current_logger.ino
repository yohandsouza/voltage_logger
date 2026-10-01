
#include "SdFat.h"
#include <Wire.h>
#include "ina219.h"
//#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1306.h>
#include "oled_lib.h"
#include <EEPROM.h>

SdFat SD;
#define ina219Wireaddr INA219_ADDRESS



//--------modifiable parameters-----------//

const uint8_t chipSelect = 4;
const uint8_t samples = 5;              //no of samples per read
const uint8_t sample_delay = 5;       //time duration between samples
const uint8_t log_interval = 10;  //file voltage log interval
char filename[10] ;//= "vtlog.csv";
//--------modifiable parameters end-----------//


unsigned long int start_time = 0;
const char eeprom_addr = 0;
const char comma_char = ',';
const char null_char = '\0';
uint32_t  timestamp = 0;
//unsigned long prev_log_instance;
String dataString = "";

unsigned long int total_mA = 0;
unsigned long int total_mW = 0;

int shuntvoltage = 0;
float busvoltage = 0;
int current_mA = 0;
float loadvoltage = 0;
//int power_mW = 0;

unsigned long int prev_calc_time;
uint8_t count = 0;
void setup(void)
{
#ifdef dbg
  Serial.begin(115200);
#endif
  oled_begin(SSD1306_SWITCHCAPVCC);
  oled_clearDisplay();
  //oled_setTextColor(SSD1306_WHITE);
  oled_setCursor(0, 0);
  oled_print_f(F("Current Logger\n\nInitializing SD card"));
  oled_display();
  //delay(2000);
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
  // Initialize the INA219.
  // By default the initialization will use the largest range (32V, 2A).  However
  // you can call a setCalibration function to change this range (see comments).
  //  if (! ina219_begin()) {
  //    Serial.println("Failed to find INA219 chip");
  //    while (1) { delay(10); }
  //  }
  ina219_begin();//
  uint16_t file_num = 0;
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
  // To use a slightly lower 32V, 1A range (higher precision on amps):
  //ina219.setCalibration_32V_1A();
  // Or to use a lower 16V, 400mA range (higher precision on volts and amps):
  //ina219.setCalibration_16V_400mA();
  File dataFile = SD.open(filename, FILE_WRITE);
  if (dataFile) {
    //dataFile.println("Time(mins),Pin Vtg,Actual Vtg,,R1(K)=" + (String)r1_k + ",R2(K)=" +  (String)r2_k + ",Log interval=" +  (String)log_interval + "ms");
    //dataFile.println(start_time_date);
    dataFile.print(F("Time(mins),Load Vtg(V),Current(mA)"));
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
  oled_print_f(F("\nReady"));
  oled_display();
  delay(1000);
  //prev_log_instance = millis();
  prev_calc_time = millis();
  oled_clearDisplay();
  oled_setCursor(0, 0);
  oled_print_f(F("Logging:- "));
  oled_print_c_str(filename);
  //oled_print_f(F("\nStart:- "));
  //oled_print_str(start_time_date.c_str(), sizeof(start_time_date));
  oled_print_f(F("\n\n"));
  get_shunt_vtg();
  get_bus_vtg();
  get_current();
  //get_power();
  get_load_voltage();
  log_data();
}

void loop(void)
{


  //  shuntvoltage = ina219_getShuntVoltage_mV();
  //  busvoltage = ina219_getBusVoltage_V();
  //  current_mA = ina219_getCurrent_mA();
  //  power_mW = ina219_getPower_mW();
  //  loadvoltage = busvoltage + (shuntvoltage / 1000);
  if (millis() - prev_calc_time >= 1000)
  {
    prev_calc_time = millis();
    oled_clearDisplay();
    oled_setCursor(0, 0);
    //File dataFile = SD.open(filename, FILE_WRITE);
    //if (dataFile) {
    //dataFile.close();
    oled_print_f(F("Logging:- "));
    oled_print_c_str(filename);
    update_power_display();
    count++;


    if (count >= log_interval )
    {
      count = 0;
      //prev_log_instance = millis();

      //oled_print_f(F("\nStart:- "));
      //oled_print_str(start_time_date.c_str(), sizeof(start_time_date));
      oled_print_f(F("\n\n"));
      //bool log_success = log_data();
      timestamp = prev_calc_time - start_time;
      if (!log_data()) { //if (!log_success) {
#ifdef dbg
        Serial.println(F("error opening datalog.txt"));
#endif
        show_error_message();
      }
    }
    //    }
    //    else
    //    {
    //       show_error_message();
    //    }
  }
  /*Serial.print(F("Bus Voltage:   ")); Serial.print(busvoltage); Serial.println(F(" V"));
    Serial.print(F("Shunt Voltage: ")); Serial.print(shuntvoltage); Serial.println(F(" mV"));
    Serial.print(F("Load Voltage:  ")); Serial.print(loadvoltage); Serial.println(F(" V"));
    Serial.print(F("Current:       ")); Serial.print(current_mA); Serial.println(F(" mA"));
    Serial.print(F("Power:         ")); Serial.print(power_mW); Serial.println(F(" mW"));
    Serial.println(buff);
    Serial.println("");*/

  //delay(1000);
}

void show_error_message()
{
  oled_clearDisplay();
  oled_setCursor(0, 0);
  oled_print_f(F("error opening "));
  oled_print_c_str(filename);
  oled_display();
}
void update_power_display() {
  // Read voltage and current from INA219.
  //  float shuntvoltage = ina219.getShuntVoltage_mV();
  //  float busvoltage = ina219.getBusVoltage_V();
  //  float current_mA = ina219.getCurrent_mA();

  /*shuntvoltage = ina219_getShuntVoltage_mV();
    busvoltage = ina219_getBusVoltage_V();
    current_mA = ina219_getCurrent_mA();
    power_mW = ina219_getPower_mW();
    loadvoltage = (busvoltage) + (shuntvoltage / 1000);*/
  get_shunt_vtg();
  get_bus_vtg();
  get_current();
  //get_power();
  get_load_voltage();
  //total_mW += power_mW;
  total_mA += current_mA;
  //total_sec += 1;
  float total_mAh = total_mA / 3600.0;
  //float total_mWh = total_mW / 3600.0;
  /*oled_clearDisplay();
    oled_setCursor(0, 0);
    //oled_print_f(F("Bus Vtg:  ")); oled_print_int(busvoltage);oled_print_f(F("."));oled_print_int((busvoltage-((int)busvoltage))*1000); oled_print_f(F("V\n"));
    oled_print_f(F("Bus Vtg:  ")); oled_print_float( busvoltage, 3); oled_print_f(F("V\n"));
    //display.print(F("Shunt Voltage: ")); display.print(shuntvoltage); display.println(F("mV"));
    //display.print(F("Load Voltage:  ")); display.print(loadvoltage); display.println(F("V"));
    oled_print_f(F("Curr:      ")); oled_print_float(current_mA, 3); oled_print_f(F("mA\n"));
    oled_print_f(F("Amp-Hr:      ")); oled_print_float(total_mAH, 1); oled_print_f(F("mAh\n"));
    oled_print_f(F("Pwr:        ")); oled_print_float(power_mW, 2); oled_print_f(F("mW\n"));
    oled_print_f(F("Engy:        ")); oled_print_float(total_mW, 2); oled_print_f(F("mWh\n"));
  */
  // Update display.
  //oled_clearDisplay();
  //oled_setCursor(0, 0);

  //  // Mode 0, display volts and amps.


  uint8_t const max_width = 19; //max no of characterrs that be displayed on 1 line.
  printSIValue(busvoltage, "\nBus VTG(", "V):", 3, max_width);
  //oled_setCursor(0, 16);
  printSIValue(current_mA / 1000.0, "\nCurr(", "A):", 3, max_width);
  printSIValue(total_mAh / 1000.0, "\nAmp-Hr(", "Ah):", 3, max_width);
  //printSIValue(power_mW / 1000.0, "\nPwr(", "W):", 3, max_width);
  //printSIValue(total_mWh / 1000.0, "\nEngy(", "Wh):", 3, max_width);

  oled_display();
}



void get_bus_vtg()
{
  busvoltage = 0;
  for (uint8_t i = 0; i < samples; i++)
  {
    busvoltage += ina219_getBusVoltage_V();
    delay(sample_delay);
  }
  busvoltage = busvoltage / samples;
}

void get_shunt_vtg()
{
  shuntvoltage = 0;
  for (uint8_t i = 0; i < samples; i++)
  {
    shuntvoltage += ina219_getShuntVoltage_mV();
    delay(sample_delay);
  }
  shuntvoltage = shuntvoltage / samples;
}

void get_current()
{
  current_mA = 0;
  for (uint8_t i = 0; i < samples; i++)
  {
    current_mA += ina219_getCurrent_mA();
    delay(sample_delay);
  }
  current_mA = current_mA / samples;
}

/*void get_power()
  {
  power_mW = 0;
  for (uint8_t i = 0; i < samples; i++)
  {
    power_mW = ina219_getPower_mW();
    delay(sample_delay);
  }
  power_mW = power_mW / samples;
  }
*/
void get_load_voltage()
{
  loadvoltage = (busvoltage) + (shuntvoltage / 1000);
}
void printSIValue(float value, char* label, char* units, int precision, int maxWidth) {
  // Print a value in SI units with the units left justified and value right justified.
  // Will switch to milli prefix if value is below 1.

  // Add milli prefix if low value.
  oled_print_c_str(label);
  maxWidth -= strlen(label);

  if (fabs(value) < 1.0) {
    oled_print_char('m');
    maxWidth -= 1;
    value *= 1000.0;
    precision = max(0, precision - 3);
  }

  // Print units.
  oled_print_c_str(units);
  maxWidth -= strlen(units);

  // Leave room for negative sign if value is negative.
  if (value < 0.0) {
    maxWidth -= 1;
  }

  // Find how many digits are in value.
  uint8_t digits = ceil(log10(fabs(value)));
  if (fabs(value) < 1.0) {
    digits = 1; // Leave room for 0 when value is below 0.
  }

  // Handle if not enough width to display value, just print dashes.
  if (digits > maxWidth) {
    // Fill width with dashes (and add extra dash for negative values);
    for (uint8_t i = 0; i < maxWidth; ++i) {
      oled_print_char('-');
    }
    if (value < 0.0) {
      oled_print_char('-');
    }
    return;
  }

  // Compute actual precision for printed value based on space left after
  // printing digits and decimal point.  Clamp within 0 to desired precision.
  uint8_t actualPrecision = constrain(maxWidth - digits - 1, 0, precision);

  // Compute how much padding to add to right justify.
  uint8_t padding = maxWidth - digits - 1 - actualPrecision;
  for (uint8_t i = 0; i < padding; ++i) {
    oled_print_char(' ');
  }

  // Finally, print the value!
  oled_print_float(value, actualPrecision);

}
// prints a number with 2 digits following the decimal place
// creates the string backwards, before printing it character-by-character from
// the end to the start
//
// Usage: myPrintf(270.458)
//  Output: 270.45
/*void myPrintf(float fVal)
  {
  char result[100];
  int dVal, dec, i;

  fVal += 0.005;   // added after a comment from Matt McNabb, see below.

  dVal = fVal;
  dec = (int)(fVal * 100) % 100;

  memset(result, 0, 100);
  result[0] = (dec % 10) + '0';
  result[1] = (dec / 10) + '0';
  result[2] = '.';

  i = 3;
  while (dVal > 0)
  {
    result[i] = (dVal % 10) + '0';
    dVal /= 10;
    i++;
  }
  Serial.println(result);
  //for (i=strlen(result)-1; i>=0; i--)
  //    putc(result[i], stdout);
  }*/

//https://stackoverflow.com/questions/23191203/convert-float-to-string-without-sprintf
/*static char * _float_to_char(float x, char *p) {
  char *s = p + CHAR_BUFF_SIZE; // go to end of buffer
  uint16_t decimals;  // variable to store the decimals
  int units;  // variable to store the units (part to left of decimal place)
  if (x < 0) { // take care of negative numbers
    decimals = (int)(x * -100) % 100; // make 1000 for 3 decimals etc.
    units = (int)(-1 * x);
  } else { // positive numbers
    decimals = (int)(x * 100) % 100;
    units = (int)x;
  }

   --s = (decimals % 10) + '0';
  decimals /= 10; // repeat for as many decimal places as you need
   --s = (decimals % 10) + '0';
   --s = '.';

  while (units > 0) {
     --s = (units % 10) + '0';
    units /= 10;
  }
  if (x < 0) *--s = '-'; // unary minus sign for negative numbers
  return s;
  }*/
bool log_data()
{
  File dataFile = SD.open(filename, FILE_WRITE);
  dataString = null_char;
  if (dataFile) {
    dataString = timestamp;
    dataString += comma_char;
    //dataString += busvoltage;
    //dataString += comma_char;
    dataString += loadvoltage;
    dataString += comma_char;
    dataString += current_mA;
    dataString += comma_char;
    //dataString += power_mW;
    //dataString += null_char;
    dataFile.println(dataString);
    dataFile.close();
    return true;
  }
  else
  {
    return false;
  }
}
/*Sketch uses 9162 bytes (29%) of program storage space. Maximum is 30720 bytes.
  Global variables use 617 bytes (30%) of dynamic memory, leaving 1431 bytes for local variables. Maximum is 2048 bytes.*/
