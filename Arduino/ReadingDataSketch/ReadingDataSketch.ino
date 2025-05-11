#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN 9
#define SS_PIN 10

MFRC522 rfid(SS_PIN, RST_PIN); // Create MFRC522 instance
MFRC522::MIFARE_Key key;

void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();
  Serial.println("Scan the RFID card to read data...");

  // Prepare the default key (0xFF 0xFF 0xFF 0xFF 0xFF 0xFF)
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
}

void loop() {
  // Check for a new card
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  Serial.print("Card detected. UID: ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // Authenticate for sector 1 (blocks 4-7) using key A
  byte sector = 1;
  byte blockAddr = 4; // Starting block for sector 1
  MFRC522::StatusCode status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, &key, &(rfid.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Authentication failed: ");
    Serial.println(rfid.GetStatusCodeName(status));
    return;
  }

  // Read block 4 (license plate)
  byte buffer[18]; // 16 bytes data + 2 bytes CRC
  byte size = sizeof(buffer);
  Serial.print("Reading license plate from block 4: ");
  status = rfid.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Read failed: ");
    Serial.println(rfid.GetStatusCodeName(status));
    return;
  }
  for (byte i = 0; i < 16; i++) {
    Serial.print((char)buffer[i]);
  }
  Serial.println();

  // Read block 5 (cash amount)
  blockAddr = 5;
  Serial.print("Reading cash amount from block 5: ");
  status = rfid.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Read failed: ");
    Serial.println(rfid.GetStatusCodeName(status));
    return;
  }
  for (byte i = 0; i < 16; i++) {
    Serial.print((char)buffer[i]);
  }
  Serial.println();

  // Halt card and stop encryption
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  delay(1500);
}