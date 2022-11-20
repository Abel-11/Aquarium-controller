 
#include <OneWire.h> 
#include <DallasTemperature.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include "RTClib.h"

#define LIGHT                    4

#define ONE_WIRE_BUS            5 

#define feed_button             3
#define not_fed_indicator       7
#define fed_indicator           8
#define temperature_light       2
#define temperature_sensor      A0
#define SDA                     A4               //SDA -> A4
#define SCL                     A5               //SCL -> A5

#define button_up                9
#define button_down              10
#define button_right             11
#define button_left              12

#define address_hour_on           5
#define address_minute_on         6
#define address_hour_off          7
#define address_minute_off        8
#define address_day_clean         9
#define address_month_clean       15
#define address_year_clean        13



#define REGISTER_MAX    1023
#define VOLTAGE_MAX     5
#define CONVERSION_REGISTER_VOLTAGE VOLTAGE_MAX/REGISTER_MAX

const int eepromAddress = 0x50;

OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);
RTC_DS3231 rtc;

LiquidCrystal_I2C lcd(0x27, 16, 2);

int adc_register = 0;
float voltaje_pin = 0;
//468000 counts in 100ms => 13h
//36000 counts in 100ms => 1h

unsigned long int count_to_turn_off_lights = 0; //initialize with light off
unsigned long int count_to_feed = 68450;            //initialize with feeding on

unsigned long previous_time_ms = 0;
unsigned long previous_update_temperature = 0;
 byte day_last_cleanup = 7;
 byte month_last_cleanup = 2;
byte year_last_cleanup = 22;

 byte hour_turn_on = 10;
 byte hour_turn_off = 21;
 byte minute_turn_on = 0;
 byte minute_turn_off;
 

DateTime date;


byte i2c_eeprom_read_byte(int deviceaddress, unsigned int eeaddress);
void i2c_eeprom_write_byte(int deviceaddress, unsigned int eeaddress, byte data);
void printDate(DateTime date);
void set_hour(bool reset_flag);



void setup() {
  Serial.begin(9600);
  Wire.begin();
  delay(1000);
  
  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.println("Initializing...");

  
  if (!rtc.begin()) {
    lcd.setCursor(0,0);
    lcd.println("Couldn't find RTC");
    while (1);
  }
  if (rtc.lostPower()||!digitalRead(button_up)) {
      // Fijar a fecha y hora de compilacion
     // rtc.adjust(DateTime(2012, 2, 5, 17, 29, 0));
      lcd.setCursor(0,0);
      lcd.println("Power lost");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      delay(5000);
      // Fijar a fecha y hora específica. En el ejemplo, 21 de Enero de 2016 a las 03:00:00
      // rtc.adjust(DateTime(2016, 1, 21, 3, 0, 0));
   }
   
  pinMode(LIGHT, OUTPUT);
  pinMode(not_fed_indicator, OUTPUT);
  pinMode(fed_indicator, OUTPUT);
  pinMode(temperature_light, OUTPUT);
  
  pinMode(temperature_sensor, INPUT);
  pinMode(feed_button, INPUT_PULLUP);
  pinMode(button_up, INPUT_PULLUP);
  pinMode(button_down, INPUT_PULLUP);
  pinMode(button_right, INPUT_PULLUP);
  pinMode(button_left, INPUT_PULLUP);

  sensors.begin(); 

  
  lcd.clear();
 // set_parameters();
  if(digitalRead(button_down) && digitalRead(button_left) && digitalRead(button_right))
  {
    
    hour_turn_on = i2c_eeprom_read_byte(eepromAddress,address_hour_on);   
    delay(50); 
    minute_turn_on = i2c_eeprom_read_byte(eepromAddress,address_minute_on);
    delay(50);
    hour_turn_off = i2c_eeprom_read_byte(eepromAddress,address_hour_off);
    delay(50);
    minute_turn_off = i2c_eeprom_read_byte(eepromAddress,address_minute_off);
    delay(50);
    day_last_cleanup = i2c_eeprom_read_byte(eepromAddress,address_day_clean);
    delay(50);
    month_last_cleanup = i2c_eeprom_read_byte(eepromAddress,address_month_clean);
    delay(50);
    year_last_cleanup = i2c_eeprom_read_byte(eepromAddress,address_year_clean);
    delay(50);
    

    
    lcd.setCursor(0,0);
    lcd.print("Memory accesed");
    lcd.setCursor(0,1);
    lcd.print("timer values");
  }else
  {
    lcd.setCursor(0,0);
    lcd.print("Compiler accesed");
    lcd.setCursor(0,1);
    lcd.print("timer values");
  }
  
  delay(3000);
//rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

}


void loop() {
   DateTime now = rtc.now();
   printDate(now);
   
  
  if(previous_update_temperature + 6000 < millis())
  {
    
    sensors.requestTemperatures();
    
    lcd.setCursor(0,1);
    lcd.print("Temp = ");
    lcd.print(sensors.getTempCByIndex(0));
    lcd.print("        ");
    previous_update_temperature = millis();
  }
  
  if(!digitalRead(button_down) && !digitalRead(button_left))
  {
    
    set_hour(1);
    lcd.backlight();
    
    while(1)
    {
        set_hour(0);
        if(!digitalRead(button_up) && !digitalRead(button_right))
            break;
    }
    i2c_eeprom_write_byte(eepromAddress, address_hour_on, hour_turn_on);
    delay(50);
    i2c_eeprom_write_byte(eepromAddress, address_minute_on, minute_turn_on);
    delay(50);
    i2c_eeprom_write_byte(eepromAddress, address_hour_off, hour_turn_off); 
    delay(50);
    i2c_eeprom_write_byte(eepromAddress, address_minute_off, minute_turn_off); 
    delay(50);
    
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Hour was set");
    lcd.setCursor(0,1);
    lcd.print("correctly");
    
    delay(3000);
    lcd.noBacklight();
  }
  
    
}


void printDate(DateTime date)
{
  if((hour_turn_on * 60 + minute_turn_on) < (hour_turn_off * 60 + minute_turn_off))
  {   
      if((date.hour()* 60 + date.minute()) >= (hour_turn_on * 60 + minute_turn_on) && (date.hour()* 60 + date.minute() )< (hour_turn_off * 60 + minute_turn_off))
      {
      
        digitalWrite(LIGHT,1);
           
        lcd.backlight();
        
       
      }else
      {
        digitalWrite(LIGHT,0);
        
        if(!digitalRead(button_up))
          lcd.backlight();
        else
          lcd.noBacklight();
      }
  }else
  {
      if((date.hour()* 60 + date.minute()) < (hour_turn_on * 60 + minute_turn_on) && (date.hour()* 60 + date.minute() ) >= (hour_turn_off * 60 + minute_turn_off))
      {

        digitalWrite(LIGHT,0);
        
        if(!digitalRead(button_up))
          lcd.backlight();
        else
          lcd.noBacklight();
       
        
       
      }else
      {
         digitalWrite(LIGHT,1);
           
          lcd.backlight();
      }
  }
  
    if(!digitalRead(button_up) && !digitalRead(button_right))
      {
        lcd.backlight();
        delay(500);
        while(1)
        {
            lcd.setCursor(0,0);
            lcd.print("change date clean?       ");
            lcd.setCursor(0,1);
            lcd.print("y:right   n:left   ");
            if(!digitalRead(button_right))
            {
                day_last_cleanup = date.day();
                month_last_cleanup = date.month();
                year_last_cleanup = date.year()-2000;
                i2c_eeprom_write_byte(eepromAddress, address_day_clean, day_last_cleanup);
                delay(50);
                i2c_eeprom_write_byte(eepromAddress, address_month_clean, month_last_cleanup);
                delay(50);
                i2c_eeprom_write_byte(eepromAddress, address_year_clean, year_last_cleanup);
                delay(50);
                
                break;
            }else if(!digitalRead(button_left))
                break;  
        }
        
      }
   

      
    lcd.setCursor(0,0);
    if(date.hour() < 10)
      lcd.print("0");
    lcd.print(date.hour());
    lcd.print(':');
    if(date.minute() < 10)
      lcd.print("0");
    lcd.print(date.minute());
    lcd.print(':');
    if(date.second() < 10)
      lcd.print("0");
    lcd.print(date.second());
    lcd.print(" ");
    lcd.print(day_last_cleanup);
    lcd.print('/');
    lcd.print(month_last_cleanup);
    lcd.print('/');
    lcd.print(year_last_cleanup);
    lcd.print("   ");
}

void set_hour(bool reset_flag)
{
    static char option = 0;
    static char blink = 0;
    if(reset_flag)
        option = 0;
   
    if(!option && read_buton_up() && !read_buton_left() && !read_buton_right())
    {
        hour_turn_on++;
        if(hour_turn_on >= 24)
          hour_turn_on = 0;
    }else if(!option && read_buton_down() && !read_buton_left() && !read_buton_right())
    {
        if(hour_turn_on)
          hour_turn_on--;
        else
            hour_turn_on = 23;
    }else if(option == 1 && read_buton_up() && !read_buton_left() && !read_buton_right())
    {
        minute_turn_on++;
        if(minute_turn_on >= 60)
          minute_turn_on = 0;
    }else if(option == 1 && read_buton_down() && !read_buton_left() && !read_buton_right())
    {
        if(minute_turn_on)
            minute_turn_on--;
        else
            minute_turn_on = 59;
        
    }else if(option == 2 && read_buton_up() && !read_buton_left() && !read_buton_right())
    {
        hour_turn_off++;
        if(hour_turn_off >= 24)
          hour_turn_off = 0;
    }else if(option == 2 && read_buton_down() && !read_buton_left() && !read_buton_right())
    {
        if(hour_turn_off)
            hour_turn_off--;
        else
            hour_turn_off = 23;
        
    }else if(option == 3 && read_buton_up() && !read_buton_left() && !read_buton_right())
    {
        minute_turn_off++;
        if(minute_turn_off >= 60)
          minute_turn_off = 0;
    }else if(option == 3 && read_buton_down() && !read_buton_left() && !read_buton_right())
    {
        if(minute_turn_off)
            minute_turn_off--;
        else
            minute_turn_off = 59;
        
    }
    if(!option)
    {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Set turn on hour");
        lcd.print("                ");
        lcd.setCursor(0,1);
        if(blink < 4)
        {
           if(hour_turn_on < 10)
             lcd.print("0");
           lcd.print(hour_turn_on);
           blink++;
        }else 
        {
            lcd.print("  ");
            blink++;
            if(blink > 8)
              blink = 0;
        }
        lcd.print(":");
        if(minute_turn_on < 10)
             lcd.print("0");
        lcd.print(minute_turn_on);
        lcd.print("                   ");
    }
    else if(option == 1)
    {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Set turn on hour");
        lcd.print("                ");
        lcd.setCursor(0,1);
        if(hour_turn_on < 10)
           lcd.print("0");
        lcd.print(hour_turn_on);
        lcd.print(":");
        if(blink < 4)
        {
           if(minute_turn_on < 10)
             lcd.print("0");
           lcd.print(minute_turn_on);
           blink++;
        }else 
        {
            lcd.print("  ");
            blink++;
            if(blink > 8)
              blink = 0;
        }       
        lcd.print("                   ");
    }
    else if(option == 2)
    {
        lcd.setCursor(0,0);
        lcd.print("Set turn off hour");
        lcd.print("                ");
        lcd.setCursor(0,1);
        if(blink < 4)
        {
           if(hour_turn_off < 10)
             lcd.print("0");
           lcd.print(hour_turn_off);
           blink++;
        }else 
        {
            lcd.print("  ");
            blink++;
            if(blink > 8)
              blink = 0;
        }
        lcd.print(":");
        if(minute_turn_off < 10)
             lcd.print("0");
        lcd.print(minute_turn_off);
        lcd.print("                   ");
    }else if(option == 3)
    {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Set turn on hour");
        lcd.print("                ");
        lcd.setCursor(0,1);
        if(hour_turn_off < 10)
           lcd.print("0");
        lcd.print(hour_turn_off);
        lcd.print(":");
        if(blink < 4)
        {
           if(minute_turn_off < 10)
             lcd.print("0");
           lcd.print(minute_turn_off);
           blink++;
        }else 
        {
            lcd.print("  ");
            blink++;
            if(blink > 8)
              blink = 0;
        }       
        lcd.print("                   ");
    
    }
    else
    {
        lcd.setCursor(0,0);
        lcd.print("For exit press ");
        lcd.print("                ");
        lcd.setCursor(0,1);
        lcd.print("keys up + right");
        lcd.print("                ");
    }
      
    
    if( !read_buton_left() && read_buton_right() && !read_buton_up())
        option++;
    
}

bool read_buton_up()
{    
    static bool previous_state = 1;
    if(!digitalRead(button_up) && previous_state)
    {    
        previous_state = 0;
        delay(20);
        return 1;
    }else if(digitalRead(button_up) && !previous_state)
    {
        previous_state = 1;
    }
    
    return 0;
}

bool read_buton_down()
{    
    static bool previous_state = 1;
    if(!digitalRead(button_down) && previous_state)
    {    
        previous_state = 0;
     
        delay(20);
        return 1;
    }else if(digitalRead(button_down) && !previous_state)
    {
      
        previous_state = 1;
    }
    
    
    return 0;
}

bool read_buton_left()
{    
    static bool previous_state = 1;
    if(!digitalRead(button_left) && previous_state)
    {    
        previous_state = 0;
        delay(20);
        return 1;
    }else if(digitalRead(button_left) && !previous_state)
    {
        previous_state = 1;
    }
    
    return 0;
}

bool read_buton_right()
{    
    static bool previous_state = 1;
    if(!digitalRead(button_right) && previous_state)
    {    
        previous_state = 0;
        delay(20);
        return 1;
    }else if(digitalRead(button_right) && !previous_state)
    {
        previous_state = 1;
    }
    
    return 0;
}

void i2c_eeprom_write_byte(int deviceaddress, unsigned int eeaddress, byte data) {
   int rdata = data;
   Wire.beginTransmission(deviceaddress);
   Wire.write((int)(eeaddress >> 8)); // MSB
   Wire.write((int)(eeaddress & 0xFF)); // LSB
   Wire.write(rdata);
   Wire.endTransmission();
}

void i2c_eeprom_write_page(int deviceaddress, unsigned int eeaddresspage, byte* data, byte length) {
   Wire.beginTransmission(deviceaddress);
   Wire.write((int)(eeaddresspage >> 8)); // MSB
   Wire.write((int)(eeaddresspage & 0xFF)); // LSB
   byte c;
   for (c = 0; c < length; c++)
      Wire.write(data[c]);
   Wire.endTransmission();
}
byte i2c_eeprom_read_byte(int deviceaddress, unsigned int eeaddress) {
   byte rdata = 0xFF;
   Wire.beginTransmission(deviceaddress);
   Wire.write((int)(eeaddress >> 8)); // MSB
   Wire.write((int)(eeaddress & 0xFF)); // LSB
   Wire.endTransmission();
   Wire.requestFrom(deviceaddress, 1);
   if (Wire.available()) rdata = Wire.read();
   return rdata;
}

void i2c_eeprom_read_buffer(int deviceaddress, unsigned int eeaddress, byte *buffer, int length) {
   Wire.beginTransmission(deviceaddress);
   Wire.write((int)(eeaddress >> 8)); // MSB
   Wire.write((int)(eeaddress & 0xFF)); // LSB
   Wire.endTransmission();
   Wire.requestFrom(deviceaddress, length);
   int c = 0;
   for (c = 0; c < length; c++)
      if (Wire.available()) buffer[c] = Wire.read();
}
void set_parameters()
{
  i2c_eeprom_write_byte(eepromAddress, address_hour_on, hour_turn_on);
  delay(50);
  i2c_eeprom_write_byte(eepromAddress, address_hour_off, hour_turn_off); 
  delay(50);
  i2c_eeprom_write_byte(eepromAddress, address_day_clean, day_last_cleanup);
  delay(50);
  i2c_eeprom_write_byte(eepromAddress, address_month_clean, month_last_cleanup);
  delay(50);
  i2c_eeprom_write_byte(eepromAddress, address_year_clean, year_last_cleanup);
  delay(50);
}

void set_hour_RTC(bool reset_flag)
{
    static char option = 0;
    static char blink = 0;
    static int hour_RTC = 0;
    static int min_RTC = 0;
    if(reset_flag)
        option = 0;
   
    if(!option && read_buton_up() && !read_buton_left() && !read_buton_right())
    {
        hour_RTC++;
        if(hour_RTC >= 24)
          hour_RTC = 0;
    }else if(!option && read_buton_down() && !read_buton_left() && !read_buton_right())
    {
        if(hour_RTC)
          hour_RTC--;
        else
            hour_RTC = 23;
    }else if(option == 1 && read_buton_up() && !read_buton_left() && !read_buton_right())
    {
        min_RTC++;
        if(min_RTC >= 60)
          min_RTC = 0;
    }else if(option == 1 && read_buton_down() && !read_buton_left() && !read_buton_right())
    {
        if(min_RTC)
            min_RTC--;
        else
            min_RTC = 59;
        
    }
    if(!option)
    {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Set turn on hour");
        lcd.print("                ");
        lcd.setCursor(0,1);
        if(blink < 4)
        {
           if(hour_RTC < 10)
             lcd.print("0");
           lcd.print(hour_RTC);
           blink++;
        }else 
        {
            lcd.print("  ");
            blink++;
            if(blink > 8)
              blink = 0;
        }
        lcd.print(":");
        if(min_RTC < 10)
             lcd.print("0");
        lcd.print(min_RTC);
        lcd.print("                   ");
    }
    else if(option == 1)
    {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Set turn on hour");
        lcd.print("                ");
        lcd.setCursor(0,1);
        if(hour_RTC < 10)
           lcd.print("0");
        lcd.print(hour_RTC);
        lcd.print(":");
        if(blink < 4)
        {
           if(min_RTC < 10)
             lcd.print("0");
           lcd.print(min_RTC);
           blink++;
        }else 
        {
            lcd.print("  ");
            blink++;
            if(blink > 8)
              blink = 0;
        }       
        lcd.print("                   ");
    }else
    {
        lcd.setCursor(0,0);
        lcd.print("For exit press ");
        lcd.print("                ");
        lcd.setCursor(0,1);
        lcd.print("keys up + right");
        lcd.print("                ");
    }
      
    
    if( !read_buton_left() && read_buton_right() && !read_buton_up())
        option++;
    
}
