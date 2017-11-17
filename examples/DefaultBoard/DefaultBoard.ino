#include <DSPI.h>
#include <OBCI32_SD.h>
#include <EEPROM.h>
#include <OpenBCI_Wifi_Master_Definitions.h>
#include <OpenBCI_Wifi_Master.h>
#include <OpenBCI_32bit_Library.h>
#include <OpenBCI_32bit_Library_Definitions.h>

// Booleans Required for SD_Card_Stuff.ino
boolean addAccelToSD = false; // On writeDataToSDcard() call adds Accel data to SD card write
boolean addAuxToSD = false; // On writeDataToSDCard() call adds Aux data to SD card write
boolean SDfileOpen = false; // Set true by SD_Card_Stuff.ino on successful file open

// DigitalRead External Trigger Button
int pushButton = 17;
int pushButtonValue;

void setup() {
  // Bring up the OpenBCI Board
  board.begin();
  // Bring up wifi
  wifi.begin(true, true);

  // Set pin to input
  pinMode(pushButton, INPUT);
  pushButtonValue = digitalRead(pushButton);
}

void loop() {
  bool storing = false;

  if (board.streaming) {
    if (board.channelDataAvailable) {
      // Read from the ADS(s), store data, set channelDataAvailable flag to false
      board.updateChannelData();

      // Check to see if button is pressed
      int value = digitalRead(pushButton);
      if (value != pushButtonValue) {
        // Store button state
        pushButtonValue = value;
        board.auxData[0] = value ? 0x6220 : 0;

        // Show the user
        digitalWrite(OPENBCI_PIN_LED, value);

        // Write button state
        if (board.curAccelMode != board.ACCEL_MODE_OFF)
          board.useAccel(false);
        addAuxToSD = true;
      }
      // Check to see if accel has new data
      else  {
        if(board.accelHasNewData()) {
          // Get new accel data
          board.accelUpdateAxisData();

          if (board.curAccelMode != board.ACCEL_MODE_ON)
            board.useAccel(true);

          // Tell the SD_Card_Stuff.ino to add accel data in the next write to SD
          addAccelToSD = true; // Set false after writeDataToSDcard()
        }
      }

      // Verify the SD file is open
      if(SDfileOpen) {
        // Write to the SD card, writes aux data
        writeDataToSDcard(board.sampleCounter);
        storing = true;
      }

      board.sendChannelData();
    }
  }

  if (!storing) {
    digitalWrite(OPENBCI_PIN_LED, (millis() & 0x100) ? HIGH : LOW);
  }

  // Check serial 0 for new data
  if (board.hasDataSerial0()) {
    // Read one char from the serial 0 port
    char newChar = board.getCharSerial0();

    // Send to the sd library for processing
    sdProcessChar(newChar);

    // Send to the board library
    board.processChar(newChar);
  }

  if (board.hasDataSerial1()) {
    // Read one char from the serial 1 port
    char newChar = board.getCharSerial1();

    // Send to the sd library for processing
    sdProcessChar(newChar);

    // Read one char and process it
    board.processChar(newChar);
  }

  // Call the loop function on the board
  board.loop();

  // Call to wifi loop
  wifi.loop();

  if (wifi.hasData()) {
    // Read one char from the wifi shield
    char newChar = wifi.getChar();

    // Send to the sd library for processing
    sdProcessChar(newChar);

    // Send to the board library
    board.processCharWifi(newChar);
  }

  if (!wifi.sentGains) {
    if(wifi.present && wifi.tx) {
      wifi.sendGains(board.numChannels, board.getGains());
    }
  }
}
