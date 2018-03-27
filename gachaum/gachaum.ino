#include "PN532.h"
#include <ESP8266WiFi.h>
#include <PN532_SPI.h>
#include <PubSubClient.h>
#include <SPI.h>

#include "config.h"

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

PN532_SPI pn532spi(SPI, D8);
PN532 nfc(pn532spi);

void setup(void) {
  Serial.begin(115200);
  Serial.println("Hello!");

  pinMode(BUILTIN_LED, OUTPUT);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqtt_callback);

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

  pinMode(D1, OUTPUT);
  Serial.println("Waiting for an ISO14443A card");
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

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
  }
  Serial.println();

  if (strcmp(topic, "haum/gachaum/strike") == 0) {
    const char *strike_open = "open";
    if (memcmp(payload, strike_open, sizeof(strike_open)) == 0) {
      digitalWrite(D1, HIGH);
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("gachaum")) {
      Serial.println("connected");
      client.publish("haum/gachaum/announce", "hello world");
      client.subscribe("haum/gachaum/strike");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop(void) {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  boolean success;
  uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // Buffer to store the returned UID
  uint8_t uidLength; // Length of the UID (4 or 7 bytes depending on ISO14443A
                     // card type)

  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success =
      nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);

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
    client.publish("haum/gachaum/tag/uid", uidString.c_str());

    // wait until the card is taken away
    nfc.setPassiveActivationRetries(0x01);
    while (
        nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength)) {
      client.loop();
    }
    nfc.setPassiveActivationRetries(0xFF);
  } else {
    // PN532 probably timed out waiting for a card
    Serial.println("Timed out waiting for a card");
    digitalWrite(D1, LOW);
  }
}
