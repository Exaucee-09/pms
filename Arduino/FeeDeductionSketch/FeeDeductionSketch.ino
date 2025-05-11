#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN 9
#define SS_PIN 10

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

void setup() {
  Serial.begin(9600);
  while (!Serial);
  SPI.begin();
  rfid.PCD_Init();
  Serial.println(F("================================"));
  Serial.println(F("      Parking Fee System"));
  Serial.println(F("================================"));
  Serial.println(F("Waiting for RFID card scan..."));
  Serial.println();

  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  // Authenticate block 4 (license plate)
  byte buffer[18];
  byte size = 18;
  byte blockAddr = 4;
  MFRC522::StatusCode status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, &key, &(rfid.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("[ERROR] Authentication failed: "));
    Serial.println(rfid.GetStatusCodeName(status));
    halt();
    return;
  }

  // Read license plate
  status = rfid.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("[ERROR] Read failed (block 4): "));
    Serial.println(rfid.GetStatusCodeName(status));
    halt();
    return;
  }
  String licensePlate = "";
  for (byte i = 0; i < 16; i++) {
    if (buffer[i] != ' ') licensePlate += (char)buffer[i];
  }

  // Read current cash (block 5)
  blockAddr = 5;
  status = rfid.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("[ERROR] Read failed (block 5): "));
    Serial.println(rfid.GetStatusCodeName(status));
    halt();
    return;
  }
  String cashStr = "";
  for (byte i = 0; i < 16; i++) {
    if (buffer[i] != ' ') cashStr += (char)buffer[i];
  }
  long currentCash = cashStr.toInt();

  // Send data to Python
  Serial.print(F("DATA:"));
  Serial.print(licensePlate);
  Serial.print(F(","));
  Serial.println(currentCash);

  // Wait for charge input
  while (Serial.available() == 0) {}
  String chargeStr = Serial.readStringUntil('\n');
  if (!chargeStr.startsWith("CHARGE:")) {
    Serial.println(F("[ERROR] Invalid charge format."));
    halt();
    return;
  }

  long charge = chargeStr.substring(7).toInt();
  long newCash = currentCash - charge;

  if (newCash < 0) {
    Serial.println(F("[ERROR] Insufficient funds."));
    halt();
    return;
  }

  // Write new balance
  String newCashStr = String(newCash);
  for (byte i = 0; i < 16; i++) {
    buffer[i] = (i < newCashStr.length()) ? newCashStr[i] : ' ';
  }
  blockAddr = 5;
  status = rfid.MIFARE_Write(blockAddr, buffer, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("[ERROR] Write failed: "));
    Serial.println(rfid.GetStatusCodeName(status));
    halt();
    return;
  }

  // Notify completion
  Serial.println("DONE");

  // Print transaction summary
  Serial.println(F("\n----------- Transaction Summary -----------"));
  Serial.print(F("License Plate: "));
  Serial.println(licensePlate);
  Serial.print(F("Charge: "));
  Serial.println(charge);
  Serial.print(F("Previous Balance: "));
  Serial.println(currentCash);
  Serial.print(F("New Balance: "));
  Serial.println(newCash);
  Serial.println(F("-------------------------------------------\n"));

  halt();
  delay(1000);
}

void halt() {
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}