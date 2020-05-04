/*
    Name:           M5-Lightning
    Created:        2020
    Author:         @MuttleyNakamoto
    Based on:       https://github.com/ropg/M5ez [@M5ez2]
                    https://github.com/arcbtc/M5StackSats [@BTCSocialist]
*/

#include <M5Stack.h>
#include <M5ez.h>
#include <ezTime.h>
#include "Free_Fonts.h"
#include <WiFiClientSecure.h>
#include <ArduinoJson.h> 
#include "M5lightning.c" //calls LightningLogo

#define MAIN_DECLARED


//Wifi credentials
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

char wifiSSID[] = "YOUR WiFi SSID";
char wifiPASS[] = "YOUR WiFi PASSWORD";


//OpenNode API
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* host = "api.opennode.co";
String apikey = "YOUR OPENNODE API KEY"; 
const int httpsPort = 443;


//Variables
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t _time;
uint8_t seconds_per_sat = 10;
uint8_t time_multiplier = 10;
uint16_t sats_to_pay;
String disp_val;

String data_lightning_invoice_payreq;
String data_id;
String data_status = "unpaid";
int counta = 0;
String hints = "false"; 
String amount;
String description = "M5-Lightning";

//Setup
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {

  M5.begin();
  
  pinMode(21, OUTPUT);
  digitalWrite(21, LOW);

  //shows splash screen
  M5.Lcd.drawBitmap(0, 0, 320, 240, (uint8_t *)m5lightning_map);
  delay(2000); 
  
//connect to local Wifi    
  WiFi.begin(wifiSSID, wifiPASS);
       
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    if(i >= 10){
      M5.Lcd.setTextDatum(MC_DATUM);
      M5.Lcd.setTextColor(TFT_BLACK, TFT_WHITE); 
      M5.Lcd.setFreeFont(FF18);
      M5.Lcd.fillScreen(TFT_WHITE);                             
      M5.Lcd.drawString("CONNECTING WiFi", 160, 120, GFXFF);
    }
    delay(1000);
    i++;
    }

  ezt::setDebug(INFO);
  ez.begin();
}


//Main Loop
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void loop() {
  ezMenu mainmenu("M5-LIGHTNING");
  mainmenu.txtBig();
  mainmenu.addItem("TURN ON THE LIGHT", mainmenu_choose);
  mainmenu.addItem("SETTINGS", mainmenu_settings);
  mainmenu.addItem("ABOUT", mainmenu_about);
  mainmenu.run();
}


//Choose time and pay
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void mainmenu_choose(){ 
  ezMenu submenu("M5-LIGHTNING");
  submenu.txtBig();
  submenu.buttons("up#Back#select##down#");
  submenu.addItem("CHOOSE DURATION", choose_duration);
  submenu.addItem("PAY", pay_sats);  
  submenu.addItem("Exit | BACK");
  submenu.run();
}


//Config menu
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void mainmenu_settings(){ 
  ezMenu submenu("SETTINGS");
  submenu.txtBig();
  submenu.buttons("up#Back#select##down#");
  submenu.addItem("CONFIG PRICE", config_tariff);
  submenu.addItem("CONFIG TIME", config_time);
  submenu.addItem("POWER OFF", power_off);
  submenu.addItem("Exit | BACK");   
  submenu.run();
}


//Choose how much time the light is ON
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void choose_duration() {
    _time = 0; 
    while(true) {
             
           while (true) {
              //if (!_time) {
                if (_time == 0) {
                disp_val = "PRESS ++ & -- TO CHOOSE THE DURATION. | TARIFF: " + String(seconds_per_sat) + " seconds per SAT ";
              }
                
              else {
                sats_to_pay = ceil(_time * time_multiplier / seconds_per_sat);
                
                if (sats_to_pay < 1) {
                  sats_to_pay = 1;
                  }
                  
                disp_val = "Light will be ON for " + String(_time * time_multiplier) + " s | SATS to pay: " + String(sats_to_pay);                 
              }
              
              ez.msgBox("DURATION", disp_val, "-#--#OK##+#++", false);
              
              String b = ez.buttons.wait();
              
              if (b == "-" && _time) _time--;
              if (b == "+" && _time < 100) _time++;
              if (b == "--") {
                if (_time < 10) {
                  _time = 0;
                } else {
                  _time -= 10;
                }
              }
              if (b == "++") {
                if (_time > 90) {
                  _time = 100;
                } else {
                  _time += 10;
                }
              }
              if (b == "OK") break;
            }
        break;         
    }
}

//Disply and pay sats
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void pay_sats(){

    amount = String(sats_to_pay);
      
    String cr = (String)char(13);
    if(ez.msgBox("PAYMENT INFO", "You will pay: " + String(sats_to_pay) + " SATS","Cancel#OK#") =="OK") {

    fetchpayment();
    delay(100);
    const char *payreq = String(data_lightning_invoice_payreq).c_str();
    M5.Lcd.fillScreen(BLACK); 
    M5.Lcd.qrcode(payreq,45,0,240,10);
    delay(100);

    checkpayment(data_id);
    while (counta < 1000){
        if (data_status == "unpaid"){
        delay(300);
        checkpayment(data_id);
        counta++;
    }
    
    else{     
        payment_done();
        processing();
        delay(1000);
        counta = 1000;
    }  
  }
  counta = 0;

  }
}


//Payment Done
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void payment_done()
{ 
  M5.Lcd.drawBitmap(0, 0, 320, 240, (uint8_t *)m5lightning_map);
  delay(2000); 
}


//Processing
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void processing()
{ 
  //Configures font
  M5.Lcd.setTextDatum(MC_DATUM);
  M5.Lcd.setTextColor(TFT_BLACK, TFT_WHITE); 
  M5.Lcd.setFreeFont(FF18);

  //write message                 
  M5.Lcd.fillScreen(TFT_WHITE);
  M5.Lcd.drawString("PLEASE WAIT", 160, 120, GFXFF);

  //change state, wait
  digitalWrite(21, HIGH);   
  delay(_time * time_multiplier*1000);
  digitalWrite(21, LOW);

  //write message    
  M5.Lcd.fillScreen(TFT_WHITE);
  M5.Lcd.drawString("TASK COMPLETE", 160, 120, GFXFF);
  
  delay(1000);
}


//Fetch payment
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void fetchpayment(){

  WiFiClientSecure client;

    if (!client.connect(host, httpsPort)) {
      return;
    }

    String topost = "{  \"amount\": \""+ amount  +"\", \"description\": \""+ description  +"\", \"route_hints\": \""+ hints  +"\"}";
    String url = "/v1/charges";

    client.print(String("POST ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "User-Agent: ESP32\r\n" +
                 "Authorization: " + apikey + "\r\n" +
                 "Content-Type: application/json\r\n" +
                 "Connection: close\r\n" +
                 "Content-Length: " + topost.length() + "\r\n" +
                 "\r\n" + 
                 topost + "\n");

     while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
          break;
        }
    }
  
    String line = client.readStringUntil('\n');
    Serial.println(line);
    const size_t capacity = 169*JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(168) + 3800;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, line);

    String data_idd = doc["data"]["id"]; 
    data_id = data_idd;
    String data_lightning_invoice_payreqq = doc["data"]["lightning_invoice"]["payreq"];
    data_lightning_invoice_payreq = data_lightning_invoice_payreqq;
}


//Check payment
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void checkpayment(String PAYID){

    WiFiClientSecure client;  
    if (!client.connect(host, httpsPort)) {
      return;
    }

    String url = "/v1/charge/" + PAYID;

    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
    "Host: " + host + "\r\n" +
    "Authorization: " + apikey + "\r\n" +
    "User-Agent: ESP32\r\n" +
    "Connection: close\r\n\r\n");

    while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
          break;
        }
    }
    
    String line = client.readStringUntil('\n');
    const size_t capacity = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(14) + 650;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, line);

    String data_statuss = doc["data"]["status"]; 
    data_status = data_statuss;
    Serial.println(data_status);
}


//About M5-LIGHTNING
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void mainmenu_about(){  
    String cr = (String)char(13);
    ez.msgBox("ABOUT", "M5-LIGHTNING by @MuttleyNakamoto | Hardware: M5Stack | Special tanks to: | Rop Gonggrijp and | @BTCSocialist");
}


//Config time: the number of seconds of light per SAT paid
///////////////////////////////////////////////////////////////////////////////////////////////////////////////


void config_tariff(){  
    String cr = (String)char(13);
    ez.msgBox("CONFIG TARIFF", "Use the keypad to adjust the tariff in seconds per SAT ");

while(true) {
             
           while (true) {
                if (seconds_per_sat == 0) {
                disp_val = "Tariff invalid. | Must be > 0! ";
              }
                
              else {
                disp_val = "Tariff = " + String(seconds_per_sat) + "| seconds per SAT ";
              }
              
              ez.msgBox("CONFIG TARIFF", disp_val, "-#--#OK##+#++", false);
              
              String b = ez.buttons.wait();
              
              if (b == "-" && seconds_per_sat) seconds_per_sat--;
              if (b == "+" && seconds_per_sat < 60) seconds_per_sat++;
              if (b == "--") {
                if (seconds_per_sat < 10) {
                  seconds_per_sat = 1;
                } 
                else {
                  seconds_per_sat -= 10;
                }
              }
              
              if (b == "++") {
                if (seconds_per_sat > 50) {
                  seconds_per_sat = 60;
                } else {
                  seconds_per_sat += 10;
                }
              }
              if (b == "OK") break;
            }
        break;         
    }

}


//Config time
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void config_time(){  
    String cr = (String)char(13);
    ez.msgBox("CONFIG TIME", "Use the keypad to adjust the step when selecting the time ");

while(true) {
             
           while (true) {
                if (time_multiplier == 0) {
                disp_val = "Delta time invalid. | Must be > 0! ";
              }
                
              else {
                disp_val = "Delta time = " + String(time_multiplier) + "| seconds";
              }
              
              ez.msgBox("CONFIG TIME", disp_val, "-#--#OK##+#++", false);
              
              String b = ez.buttons.wait();
              
              if (b == "-" && time_multiplier) time_multiplier--;
              if (b == "+" && time_multiplier < 60) time_multiplier++;
              if (b == "--") {
                if (time_multiplier < 10) {
                  time_multiplier = 1;
                } 
                else {
                  time_multiplier -= 10;
                }
              }
              
              if (b == "++") {
                if (time_multiplier > 50) {
                  time_multiplier = 60;
                } else {
                  time_multiplier += 10;
                }
              }
              if (b == "OK") break;
            }
        break;         
    }

}



//Power off M5-LIGHTNING
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void power_off() {
  m5.powerOFF(); 
 }

 
