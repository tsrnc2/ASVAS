#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Wire.h>
#include <EEPROM.h>

// LCD and Keypad Setup
LiquidCrystal_I2C lcd(0x27, 16, 2);
const int relayPin = 12;  // Relay pin
const String salt = "s@1tValue"; 
const int initFlagAddress = 0;
bool settingGuestPassword = false;

unsigned long storedMasterHash = 0;
unsigned long storedGuestHash = 0;

char keys[4][4] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[4] = {11, 10, 9, 8};
byte colPins[4] = {7, 6, 5, 4};
Keypad myKeypad = Keypad(makeKeymap(keys), rowPins, colPins, 4, 4);

void setup() {
  Wire.begin();
  lcd.init();
  lcd.backlight();
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  bool isInitialized;
  EEPROM.get(initFlagAddress, isInitialized);
  if (!isInitialized) {
    performInitialSetup();
    EEPROM.put(initFlagAddress, true);
  } else {
    loadStoredHashes();
  }
  lcd.clear();
  lcd.print("Enter Password");
}

void loop() {
  static char checkpassword[8] = {'\0'};
  static int currentdigit = 0;
  static unsigned int failedAttempts = 0;

  lcd.setCursor(currentdigit, 1);
  char keyPressed = myKeypad.getKey();
  if (keyPressed) {
    checkpassword[currentdigit] = keyPressed;
    lcd.print("*");
    currentdigit = (currentdigit + 1) % 7;
    if (currentdigit == 0) {
      String enteredPassword = String(checkpassword);
      if (settingGuestPassword) {
        setGuestPassword(enteredPassword);
      } else if (comparePasswords(enteredPassword, storedMasterHash)) {
        masterMenu();
        failedAttempts = 0;
      } else if (comparePasswords(enteredPassword, storedGuestHash)) {
        startVehicle();
      } else {
        failedAttempts++;
        displayFailedAttempt(failedAttempts);
      }
      memset(checkpassword, 0, 8);
      currentdigit = 0;
      lcd.clear();
      lcd.print("Enter Password");
    }
  }
}
void displayFailedAttempt(unsigned int failedAttempts) {
    lcd.clear();
    lcd.print("Access Denied");
    lcd.setCursor(0, 1);
    lcd.print("Failed: " + String(failedAttempts));
    delay(2000); // Display the message for 2 seconds
}


bool i2CAddrTest(uint8_t addr) {
    Wire.beginTransmission(addr);
    return Wire.endTransmission() == 0;
}

void performInitialSetup() {
    lcd.clear();
    lcd.print("Set Master Pwd");
    String masterPassword = inputPassword();
    storePassword(masterPassword, true);
    lcd.clear();
    lcd.print("Set Guest Pwd");
    delay(2000);
    String guestPassword = inputPassword();
    storePassword(guestPassword, false);
    lcd.clear();
    lcd.print("Setup Complete");
    delay(2000);
}

String inputPassword() {
    char password[8] = {'\0'};
    int currentDigit = 0;
    
    while (currentDigit < 7) {
        lcd.setCursor(currentDigit, 1);
        char key = myKeypad.getKey();
        if (key) {
            password[currentDigit] = key;
            lcd.print("*");
            currentDigit++;
            delay(500);
        }
    }

    password[currentDigit] = '\0';
    return String(password);
}

void storePassword(String password, bool isMaster) {
    unsigned long hash = hashPassword(password + salt);
    if (isMaster) {
        EEPROM.put(sizeof(bool), hash); 
        storedMasterHash = hash;
    } else {
        EEPROM.put(sizeof(bool) + sizeof(unsigned long), hash);
        storedGuestHash = hash;
    }
}

void loadStoredHashes() {
    EEPROM.get(sizeof(bool), storedMasterHash);
    EEPROM.get(sizeof(bool) + sizeof(unsigned long), storedGuestHash);
}

unsigned long hashPassword(String password) {
    unsigned long hash = 5381;
    for (int i = 0; i < password.length(); i++) {
        hash = ((hash << 5) + hash) + password[i];
    }
    return hash;
}

bool comparePasswords(String enteredPassword, unsigned long storedHash) {
    String saltedPassword = enteredPassword + salt;
    unsigned long hash = hashPassword(saltedPassword);
    return hash == storedHash;
}

void masterMenu() {
    int page = 1;
    const int maxPage = 3; // Total number of pages
    displayMasterMenuPage(page);

    while (true) {
        char key = myKeypad.getKey();
        if (key) {
            if (key == '#') {
                page = page % maxPage + 1; // Cycle through pages
                displayMasterMenuPage(page);
            } else if (key >= '1' && key <= '4') {
                executeMasterMenuOption(key, page);
                break;
            }
            delay(100); // Debounce delay
        }
    }
}

void displayMasterMenuPage(int page) {
    lcd.clear();
    if (page == 1) {
        lcd.print("1:Clr EEPROM");
        lcd.setCursor(0, 1);
        lcd.print("2:New Gst Pwd");
    } else if (page == 2) {
        lcd.print("3:Test LCD");
        lcd.setCursor(0, 1);
        lcd.print("4:Test Keypad");
    } else if (page == 3) {
        lcd.print("5:Test Relay");
        lcd.setCursor(0, 1);
        lcd.print("6:Test EEPROM");
    }
}

void executeMasterMenuOption(char option, int page) {
    if (page == 1) {
        // Page 1 options
    } else if (page == 2) {
        // Page 2 options
        switch (option) {
            case '3':
                testLCD();
                break;
            case '4':
                testKeypad();
                break;
        }
    } else if (page == 3) {
        // Page 3 options
        switch (option) {
            case '5':
                testRelay();
                break;
            case '6':
                testEEPROM();
                break;
        }
    }
}

void setGuestPassword(String newGuestPassword) {
    storePassword(newGuestPassword, false);
    lcd.clear();
    lcd.print("Guest Pwd Set");
    delay(2000);
}

void clearEEPROM() {
    lcd.clear();
    lcd.print("Clearing EEPROM");
    for (int i = 0; i < EEPROM.length(); i++) {
        EEPROM.write(i, 0);
    }
    delay(2000);
    lcd.clear();
    lcd.print("EEPROM Cleared");
    delay(2000);
}

void startVehicle() {
    lcd.clear();
    lcd.print("Starting Car...");
    digitalWrite(relayPin, HIGH);  // Turn on the relay

    // Maintain the message for 45 seconds
    long startTime = millis();
    while (millis() - startTime < 45000) {
        // Keep the loop running to maintain the display
    }

    digitalWrite(relayPin, LOW); // Turn off the relay
    lcd.clear();
    lcd.print("Car Started");
    delay(2000); // Display message for 2 seconds before clearing
}


void testLCD() {
    lcd.init();
    lcd.backlight();
    lcd.print("Testing LCD...");
    delay(2000);
    lcd.clear();
    // Check if the text is correctly displayed on the LCD.
}


void testKeypad() {
    lcd.print("Press Keys");
    while (true) {
        char key = myKeypad.getKey();
        if (key) {
            lcd.clear();
            lcd.print("Key Pressed: ");
            lcd.print(key);
            delay(1000);
            lcd.clear();
        }
    }
    // Check if the correct keys are displayed on the LCD.
}


void testRelay() {
    digitalWrite(relayPin, HIGH); // Turn on the relay
    lcd.print("Relay ON");
    delay(5000); // Wait for 5 seconds
    digitalWrite(relayPin, LOW); // Turn off the relay
    lcd.clear();
    lcd.print("Relay OFF");
    delay(2000);
    // Manually check if the relay is functioning.
}


void testEEPROM() {
    // Write a test value
    int testAddress = 10; // Choose an address that doesn't interfere with your program data
    byte testValue = 123;
    EEPROM.write(testAddress, testValue);

    // Read the value back
    byte readValue = EEPROM.read(testAddress);
    if (readValue == testValue) {
        lcd.print("EEPROM OK");
    } else {
        lcd.print("EEPROM Fail");
    }
    delay(2000);
}


void testPasswordHashing() {
    String testPassword = "1234567"; // Example password
    unsigned long hashed = hashPassword(testPassword + salt);

    // Simulate storing and verifying the password
    EEPROM.put(sizeof(bool), hashed);
    unsigned long storedHash;
    EEPROM.get(sizeof(bool), storedHash);

    if (hashed == storedHash) {
        lcd.print("Hashing OK");
    } else {
        lcd.print("Hashing Fail");
    }
    delay(2000);
}
