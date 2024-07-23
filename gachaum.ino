#include "PN532.h"
#include <ESP8266WiFi.h>
#include <PN532_I2C.h>
#include <PubSubClient.h>
#include <Wire.h>

#include "config.h"

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
char message [50];
int value = 0;

PN532_I2C pn532_i2c(Wire);
PN532 nfc(pn532_i2c);

void setup(void) {
  Serial.begin(115200);
  Serial.println("Hello!");

  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqtt_callback);

  Wire.begin();
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1)
      ; // halt
  }

  // Got ok data, print it out!
  Serial.print("Found chip PN5");
  Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. ");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((versiondata >> 8) & 0xFF, DEC);

  // Set the max number of retry attempts to read from a card
  // This prevents us from waiting forever for a card, which is
  // the default behaviour of the PN532.
  nfc.setPassiveActivationRetries(0xFF);

  // configure board to read RFID tags
  nfc.SAMConfig();

  pinMode(D3, OUTPUT);
  pinMode(D5, OUTPUT);
  digitalWrite(D3, LOW);
  digitalWrite(D5, LOW);
  Serial.println("Waiting for an ISO14443A card");
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);
  
  uint8_t macAddr[6];
  char hostname [32];
  WiFi.macAddress(macAddr);
  sprintf (hostname, READER_NAME);
  Serial.println();
  Serial.print("new hostname -> : ");
  Serial.print(hostname);
  Serial.println();
  WiFi.hostname(hostname);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());


}

void mqtt_callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    message[i]=(char)payload[i];
    message[i+1]=0;
  }
  Serial.println();

  if (strcmp(topic, "haum/laser/power") == 0) {
    Serial.println("On topic");
    if (strcmp(message, "on")==0){
      Serial.println("On");
      digitalWrite(D3, HIGH);
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect( READER_NAME, mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      // client.publish("haum/gachaum/announce", "hello world");
      client.subscribe("haum/laser/power");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

uint8_t _modulation = PN532_MIFARE_ISO14443B;

void loop(void) {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  _modulation = (_modulation == PN532_MIFARE_ISO14443A)
                    ? PN532_MIFARE_ISO14443B
                    : PN532_MIFARE_ISO14443A;
  boolean success;
  uint8_t uid[12];   // Buffer to store the returned UID
  uint8_t uidLength; // Length of the UID (4 or 7 bytes depending on ISO14443A
                     // card type)

  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(_modulation, &uid[0], &uidLength);

  if (success) {
    Serial.println("Found a card!");
    Serial.print("UID Length: ");
    Serial.print(uidLength, DEC);
    Serial.println(" bytes");
    Serial.print("UID Value: ");
    String uidString = "0x";
    for (uint8_t i = 0; i < uidLength; i++) {
      uidString += String(uid[i], HEX);
    }
    Serial.println(uidString);
    client.publish("haum/laser/tag/uid", uidString.c_str());
    digitalWrite(D5, HIGH);
    // wait until the card is taken away
    nfc.setPassiveActivationRetries(0x01);
    while (nfc.isTargetPresent(_modulation)) {
      client.loop();
    }
    digitalWrite(D5, LOW);
    nfc.setPassiveActivationRetries(0xFF);
  } else {
    // PN532 probably timed out waiting for a card
    Serial.println("Timed out waiting for a card");
    digitalWrite(D3, LOW);
  }
}
