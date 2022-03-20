/**
d1 mini rc522 wiring
RST  ----- D3
MISO ----- D6
MOSI ----- D7
SCK ------ D5
SDA/SSN -- D8
*/

/**
 * import secrets
 */
#include "arduino_secrets.h"


/**
 * import libraries
 */
#include "ESP8266WiFi.h"
#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>

WiFiClient wifiClient;

constexpr uint8_t RST_PIN = 0;
constexpr uint8_t SS_PIN = 15;

/**
 * WiFi config
 */
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;

/**
 * create MFRC522 instance
 */
MFRC522 mfrc522(SS_PIN, RST_PIN);

void setup() {
  /**
   * set baud
   */
  Serial.begin(74880);

  delay(1000);

  while (!Serial);

  
  SPI.begin();
  mfrc522.PCD_Init();
  mfrc522.PCD_DumpVersionToSerial();

  /**
   * connect to WiFi
   */
  WiFi.begin(ssid, password);

  /**
   * wait for the WiFI connection
   */
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("connecting to WiFi...");
  }
}

bool locked = false;

void loop() {
    /**
     * check WiFi connection again
     */
    if (WiFi.status() == WL_CONNECTED) {        
        bool chipPresent = chipIsPresent();

        /**
         * reset if no chip was locked an no chip is present
         */
        if (! locked && ! chipPresent) {
            return;
        }

        MFRC522::StatusCode result = mfrc522.PICC_Select(&mfrc522.uid,8*mfrc522.uid.size);

        if(!locked && result == MFRC522::STATUS_OK)
        {
            /**
             * if a chip has been added
             * lock chip
             */
            locked = true;

            /**
             * open/prepare connection
             */
            HTTPClient http;
            http.begin(wifiClient, API_URL);
            http.addHeader("Content-Type", "application/json");
            http.addHeader("token", SECRET_TOKEN);   

            /**
             * make http request
             */         
            int httpCode = http.POST("{\"data\": {\"attributes\":{\"rfid\": \""+getUID()+"\",\"status\": \"1\"},\"type\": \"Projects\"}}");

            /**
             * get the request payload
             */
            String payload = http.getString();
            Serial.println(payload);

            /**
             * close connection
             */
            http.end();
           
            Serial.println("added: " + getUID());
        }
        else if(locked && result != MFRC522::STATUS_OK) {
            /**
             * if a chip has been removed
             */
             
            /**
             * open/prepare connection
             */
            HTTPClient http;
            http.begin(wifiClient, API_URL);
            http.addHeader("Content-Type", "application/json");
            http.addHeader("token", SECRET_TOKEN);

            /**
             * make http request
             */    
            int httpCode = http.POST("{\"data\": {\"attributes\":{\"rfid\": \""+getUID()+"\",\"status\": \"0\"},\"type\": \"Projects\"}}");

            /**
             * get the request payload
             */
            String payload = http.getString();
            Serial.println(payload);

            /**
             * close connection
             */
            http.end();
            
            Serial.println("removed: " + getUID());

            /**
             * unlock chip
             */
            locked = false;
            mfrc522.uid.size = 0;
        }
        else if(locked && result == MFRC522::STATUS_OK) {
          /**            
           * TODO
           */
        }
        else if(!locked && result != MFRC522::STATUS_OK) {
          /**
           * unlock chip if an error has occurred
           */
            mfrc522.uid.size = 0;
        }
    }

    mfrc522.PICC_HaltA();
}

/**
 * helper to dump a byte array as hex values
 */
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(((buffer[i])>>4)&0x0F,  HEX);
    Serial.print(buffer[i]&0x0F, HEX);
    Serial.print(" ");
  }
}

/**
 * get the card's uID
 */
String getUID() {
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uid += String(mfrc522.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  return uid;
}

/**
 * return true if a chip is present
 * @return bool
 */
bool chipIsPresent() {
  byte bufferATQA[2];
  byte bufferSize = sizeof(bufferATQA);

  mfrc522.PCD_WriteRegister(mfrc522.TxModeReg, 0x00);
  mfrc522.PCD_WriteRegister(mfrc522.RxModeReg, 0x00);
  mfrc522.PCD_WriteRegister(mfrc522.ModWidthReg, 0x26);

  MFRC522::StatusCode result = mfrc522.PICC_WakeupA(bufferATQA, &bufferSize);
  return (result == MFRC522::STATUS_OK || result == MFRC522::STATUS_COLLISION);
}
