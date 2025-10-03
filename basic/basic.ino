#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <RtcDS3231.h>
// #include <ThreeWire.h>

#include <EEPROM.h>
#define EEPROM_SIZE 18
// #define flag 0.5

#include "PressButton.h"
#include <String.h>



#include "BluetoothSerial.h"
BluetoothSerial SerialBT;

// RTC 3231

#include <Wire.h>

#include <type_traits> // Required for type checking

// Function to return a human-readable type name
template <typename T>
const char* getDataType(T value) {
  if (std::is_same<T, int>::value) return "int";
  if (std::is_same<T, float>::value) return "float";
  if (std::is_same<T, double>::value) return "double";
  if (std::is_same<T, char>::value) return "char";
  if (std::is_same<T, bool>::value) return "bool";
  if (std::is_same<T, const char*>::value) return "string (const char*)";
  if (std::is_same<T, String>::value) return "String";
  return "unknown type";
}



#define DISP_ITEM_ROWS 1     // number of rows usable in the display
#define DISP_CHAR_WIDTH 16   // general info about the how many characters in a single row
#define PACING_MS 25         // minimum wait milliseconds between executing code in menu loop
#define FLASH_RST_CNT 30     // number of loops between switching flash mode
#define SETTINGS_CHKVAL 3647 // value used to manage versioning control

#define ALARM_CHKVAL 859534

// led Pin

int bluePin = 14;
int redPin = 13;
int greenPin = 12;

// I/O PORT ALLOCATIONS
#define BTN_1 35
#define BTN_2 34
#define BTN_3 4  // MAIN BUTTON!!

#define tankStatePin 33
#define potPin 32 
#define relayPin 15
#define buzzerPin 21

// BUTTONS
PressButton btn_1(BTN_1);
PressButton btn_2(BTN_2);
// PressButton btn_3(BTN_3);

// BUTTON 3

boolean wasdown = false;
boolean capturedownstate();
boolean isdown();
boolean isup();
boolean pressreleased();

void printChars(int cnt, char c);
int getintCharCnt(int value);

// setup the enum with all the menu pages options
enum pageType
{
  MENU_ROOT,

  MENU_CURRENTALARM,
  MENU_CURRENTALARM_SETALARM,
  MENU_CURRENTALARM_BACK,

  MENU_MAINMENU,
  MENU_MAINMENU_ALARMLIST,
  MENU_MAINMENU_INDIVIDUALALARM,
  MENU_MAINMENU_INDIVIDUALALARM_BACK,
  MENU_MAINMENU_ALARMLIST_BACK,
  MENU_MAINMENU_SETTINGS,
  MENU_MAINMENU_BACK,

  MENU_ALARMON,

  MENU_ALARMON_BACK,
};

void split(String data, char delimiter, String parts[], int maxParts);


// holds which page is currently selected
enum pageType currPage = MENU_ROOT;

void page_MenuRoot();
void page_CurrentTime();
void page_MainMenu();
void page_MainMenu_AlarmList();
void page_MainMenu_IndividualAlarm();
void page_MainMenu_IndividualAlarm_Back();
void page_MainMenu_AlarmList_Back();
void page_MainMenu_Settings();
void page_CurrentTime_SetTime();
void page_CurrentTime_Back();
void page_MainMenu_Back();
void page_AlarmOn();
void page_AlarmOn_Back();

// MENU INTERVALS ------------------------------------------------------------------------------------------------------------------------
int loopStartMs;
boolean updateAllItems;
int updateItemValue;
int itemCnt;
int pntrPos;
int alarmNumber_pntrPos; // stores which alarm is selected
int dispOffset;
int root_pntrPos = 1;
int root_dispOffset = 0;
int flashCntr;
boolean flashIsOn;

void initMenuPage(String title, int itemCount);       // sets all the common menu values and prints the menu title
void captureButtonDownStates();                           // captures the pressed down state for all buttons (set only)
void adjustBoolean(boolean *v);                           // adjusts a Boolean value depending on the button state
void adjustUint8_tb(float *v, int min, int max); // adjusts a Uint8_t value depending onthe button state
void doPointerNavigation();                               // does the up/down point navigation
boolean isFlashChanged();                                 // Returns true whenever the flash state changes (flash interval  = PACING_MS)
void pacingWait();                                        // placed at the bottom of the loop to keep constant pacing of the interval
boolean menuItemPrintable(int xPos, int yPos);    // will return a positive state if item can be display

// PRINT TOOLS ------------------------------------------------------------------------------------------------------------------------
void printPointer();
void printOffsetArrows();     // print the arrows to indicate if the menu extends beyond current view
void printOnOff(boolean val); // print either On or Off depending on the boolean state
void printintAtWidth(int value, int width, char c, boolean isRight);

// MAIN TANK SETTINGS-------------------------------------------------------------------------------------------------------------------
void mainLoop();
void mainsOnOff();
void relayOn();
void relayOff();
boolean checkAlarm();
boolean tankFullState();
boolean checkWaterFlowState();
boolean checkWaterFlow();
void tankFullDisplay();
boolean durationComplete();
void waitTime();
void dryRunCheck();

// CURRENT TIME

void displayTime();
void displayWithoutTime();
boolean isMinuteChanged();
char manual[10];
char charManualHour[5];
char charManualMinute[5];
int manualHour;
int manualMinute;
void print_ManualTime();
void manualSetTime();

// Alarm List Settings

void showAlarmsList();
void showAlarmsOnList();
boolean wasRunningAlarm();
int onAlarm;
char alarmOnList[][15] = {};

// BUZZER

void buzzerOn();
void buzzerOff();
unsigned long buzzerWait = 5000; // 2 sec
unsigned long millisBuzzer;

// RTC SETUPwek

// ThreeWire myWire(18, 5, 19); // data clk rst
// RtcDS3231<ThreeWire> rtc(myWire);

RtcDS3231<TwoWire> rtc(Wire);

// Potentiometer value

int potVal;

// Relay State
boolean relayState = false;

// TANK SETTINGS

unsigned long previousMillis;

// UPDATES

boolean updateTankFullDisplay;
boolean updateDryRunCheck;
boolean updateWaitTime;
boolean updateCheckWaterFlow;
boolean updateTankState;

// INDIVIDUAL ALARM SETTINGS

void printState();
void print_StartTime();
void print_EndTime();
void print_DayOfWeek();
void adjustAlarmStartTime();
void adjustAlarmEndTime();
void adjustState();
void adjustDayOfWeek();

int currentlyRunningAlarm; // Alarm that is running now.

char week[8][10] = {"Sun    ", "Mon    ", "Tues   ", "Wed    ", "Thurs  ", "Fri    ", "Sat    "};


// SETTINGS ------------------------------------------------------------------------------------------------------------------------
struct MySettings
{
  int waterFlowCheckAverageTimes = 10;
  int tankFullCheckAverageTimes = 10;

  float waterFlowCheckingInterval_mins = EEPROM.read(0); // 1 min
  float dryRunCheckInterval_mins = EEPROM.read(2);       // 2 min
  float tankFullStateInterval_mins = EEPROM.read(1);
  float buzzerInterval_mins = 0.167f; // 10 sec

  // unsigned long waterFlowCheckingInterval = (unsigned long)(waterFlowCheckingInterval_mins*60.0f*1000.0f); // 1 min waterFlowCheckingInterval_mins*60*1000;
  // unsigned long dryRunCheckInterval = dryRunCheckInterval_mins*60.0f*1000.0f;       // 2 min dryRunCheckInterval_mins*60*1000;
  // unsigned long tankFullStateInterval = tankFullStateInterval_mins*60.0f*1000.0f; // 30 seconds

  float threshold_waterCheckPin = 4080.0f;
  float threshold_tankStatePin = 4080.0f;

  int settingCheckValue = SETTINGS_CHKVAL; // settings check value to confirm settings are valid !!! MUST BE AT END !!!
};

MySettings settings;

void sets_SetDefaults();
void sets_Load();
void sets_Save();

// Alarms List
struct Alarms
{
  char alarmName[10];
  int startTime[3];
  int endTime[3];
  boolean state;
  int alarmDay;
  uint32_t alarmCheckValue = ALARM_CHKVAL;
};

Alarms alarms[] = {
    {"Alarm 1", {1, 0}, {0, 0}, false, 0},
    {"Alarm 2", {2,0}, {0, 0}, false, 5},
    {"Alarm 3", {3, 0}, {0, 0}, false, 0},
    // {"Alarm 4", {4,0}, {0,0}, false, 0},
    // {"Alarm 5", {5, 0}, {0, 0}, false, 0}
    };


int size = sizeof(alarms) / sizeof(alarms[0]);

// DISPLAY ------------------------------------------------------------------------------------------------------------------------
LiquidCrystal_I2C lcd(0x27, 16, 2);
byte chrUp[] = {0b00000, // Arrow up custom character data
                0b00100,
                0b00100,
                0b01110,
                0b01110,
                0b11111,
                0b11111,
                0b00000};

byte chrDn[] = {0b00000, // Arrow down custom character data
                0b11111,
                0b11111,
                0b01110,
                0b01110,
                0b00100,
                0b00100,
                0b00000};

byte chrF1[] = {0b10000, // Menu title left side custom character data
                0b01000,
                0b10100,
                0b01010,
                0b10100,
                0b01000,
                0b10000,
                0b00000};

byte chrF2[] = {0b00001, // Menu title right side custom character data
                0b00010,
                0b00101,
                0b01010,
                0b00101,
                0b00010,
                0b00001,
                0b00000};

byte chrAr[] = {0b00100, // Selected item pointer custom character data
                0b00100,
                0b01110,
                0b01100,
                0b11001,
                0b10011,
                0b00111,
                0b01110};

void setup()
{

  Serial.begin(9600);
  SerialBT.begin("SEVAK-00");

  pinMode(relayPin, OUTPUT);
  pinMode(BTN_3, INPUT_PULLUP); // Watch this for buttoon on rgb

  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  // pinMode(buzzerPin, OUTPUT);

  rtc.Begin();
  // RtcDateTime currentTime = RtcDateTime(__DATE__, __TIME__); // ALREADY HAS THE CURRENT DATE AND TIME !!
  // rtc.SetDateTime(currentTime);

  EEPROM.begin(EEPROM_SIZE);

  // int u = EEPROM.read(4);

  // alarms[u-1].state = EEPROM.read(3);

  // alarms[u-1].startTime[0] = EEPROM.read(5);
  // alarms[u-1].startTime[1] = EEPROM.read(6);

  // alarms[u-1].endTime[0] = EEPROM.read(7);
  // alarms[u-1].endTime[1] = EEPROM.read(8);

  // loadAlarmsFromEEPROM();




  // if (EEPROM.read(0) == 255){

  // EEPROM.write(0, 1);
  // EEPROM.write(1, 1);
  // EEPROM.write(2, 10);

  // EEPROM.commit();

  // }

  // if (EEPROM.read(5) == 255) { EEPROM.write(5, 0); EEPROM.write(6, 0);}

  // if (EEPROM.read(7) == 255) { EEPROM.write(7, 0); EEPROM.write(8, 0);}


  lcd.init();

  lcd.createChar(1, chrUp);
  lcd.createChar(2, chrDn);
  lcd.createChar(3, chrF1);
  lcd.createChar(4, chrF2);
  lcd.createChar(5, chrAr);

  lcd.backlight();

  lcd.clear();

  // load the settings
  sets_Load();

  Serial.println("Setup Complete");

  for (int i = 1; i <= size; i++){
    alarms[i-1].state = EEPROM.read(3 + 5*(i-1));

    alarms[i-1].startTime[0] = EEPROM.read(4 + 5*(i-1));
    alarms[i-1].startTime[1] = EEPROM.read(5 + 5*(i-1));

    // Serial.print(alarms[i-1].startTime[0]);Serial.print("     "); Serial.println(EEPROM.read(4 + 5*(i-1)));
    // Serial.print(alarms[i-1].startTime[1]);Serial.print("     "); Serial.println(EEPROM.read(5 + 5*(i-1)));

    alarms[i-1].endTime[0] = EEPROM.read(6 + 5*(i-1));
    alarms[i-1].endTime[1] = EEPROM.read(7 + 5*(i-1));
  }

  settings.waterFlowCheckingInterval_mins = EEPROM.read(0); // 1 min
  settings.dryRunCheckInterval_mins = EEPROM.read(2);       // 2 min
  settings.tankFullStateInterval_mins = EEPROM.read(1);

  // Serial.print("adasdadasdasd");
}

void loop()
{
  mainLoop();
  // switch (currPage)
  // {
  // case MENU_ROOT:
  //   page_MenuRoot();
  //   break;
  // case MENU_MAINMENU:
  //   page_MainMenu();
  //   break;
  // case MENU_MAINMENU_ALARMLIST:
  //   page_MainMenu_AlarmList();
  //   break;
  // case MENU_MAINMENU_INDIVIDUALALARM:
  //   page_MainMenu_IndividualAlarm();
  //   break;
  // case MENU_MAINMENU_INDIVIDUALALARM_BACK:
  //   page_MainMenu_IndividualAlarm_Back();
  //   break;
  // case MENU_MAINMENU_ALARMLIST_BACK:
  //   page_MainMenu_AlarmList_Back();
  //   break;
  // case MENU_MAINMENU_SETTINGS:
  //   page_MainMenu_Settings();
  //   break;
  // case MENU_CURRENTALARM:
  //   page_CurrentTime();
  //   break;
  // case MENU_CURRENTALARM_SETALARM:
  //   page_CurrentTime_SetTime();
  //   break;
  // case MENU_CURRENTALARM_BACK:
  //   page_CurrentTime_Back();
  //   break;
  // case MENU_MAINMENU_BACK:
  //   page_MainMenu_Back();
  //   break;
  // case MENU_ALARMON:
  //   page_AlarmOn();
  //   break;
  // case MENU_ALARMON_BACK:
  //   page_AlarmOn_Back();
  //   break;
  // }
}

// =================================================================
// ||                      MENUS                                  ||
// =================================================================

void page_MenuRoot()
{

  // initializes the menu page
  initMenuPage(F("SEVAK-01"), 3);

  // for the ROOT MENU we will recall last known position and offset of the display
  pntrPos = root_pntrPos;
  dispOffset = root_dispOffset;

  // inner loop
  while (true)
  {

    mainLoop();

    // print the display items when requested
    if (updateAllItems)
    {

      // print the visible items
      if (menuItemPrintable(1, 2))
      {
        lcd.print(F("Main Menu     "));
      }
      if (menuItemPrintable(1, 3))
      {
        lcd.print(F("Alarms On     "));
      }

      // print offset up/down arrows
      printOffsetArrows();
    }

    if (isMinuteChanged())
    {
      updateItemValue = true;
    }

    if (updateItemValue || updateAllItems)
    {
      if (menuItemPrintable(1, 1))
      {
        displayTime();
      }
    }

    if (isFlashChanged())
    {
      printPointer();
    }

    // always clear update flags by this point
    updateAllItems = false;
    updateItemValue = false;

    // capture the button down states
    captureButtonDownStates();

    // check for the OK button
    if (btn_2.PressReleased())
    {
      // for the ROOT MENU we will recall last known position and offset of the display
      root_pntrPos = pntrPos;
      root_dispOffset = dispOffset;
      switch (pntrPos)
      {
      case 2:
        currPage = MENU_MAINMENU;
        return;
      case 3:
        currPage = MENU_ALARMON;
        return;
      }
    }

    // otherwise check for pointer up or down buttons
    doPointerNavigation();

    pacingWait();
  }
}

void page_MainMenu()
{
  // initializes the menu page
  initMenuPage(F("MAIN MENU"), 3);

  // inner loop
  while (true)
  {

    mainLoop();

    // print the display items when requested
    if (updateAllItems)
    {

      // print the visible items
      if (menuItemPrintable(1, 1))
      {
        lcd.print(F("Alarms List"));
      }
      if (menuItemPrintable(1, 2))
      {
        lcd.print(F("Settings   "));
      }
      if (menuItemPrintable(1, 3))
      {
        lcd.print(F("Back       "));
      }

      // print offset up/down arrows
      printOffsetArrows();
    }

    if (isFlashChanged())
    {
      printPointer();
    }

    // always clear update flags by this point
    updateAllItems = false;

    // capture the button down states
    captureButtonDownStates();

    // check for the OK button
    if (btn_2.PressReleased())
    {
      switch (pntrPos)
      {
      case 1:
        currPage = MENU_MAINMENU_ALARMLIST;
        return;
      case 2:
        currPage = MENU_MAINMENU_SETTINGS;
        return;
      case 3:
        currPage = MENU_MAINMENU_BACK;
        return;
      }
    }

    // otherwise check for pointer up or down buttons
    doPointerNavigation();

    pacingWait();
  }
}

void page_MainMenu_AlarmList()
{

  // initializes the menu page
  initMenuPage(F("ALARMS LIST"), 6);

  // inner loop
  while (true)
  {

    mainLoop();

    // print the display items when requested
    if (updateAllItems)
    {

      // print the visible items
      showAlarmsList();

      // print offset up/down arrows
      printOffsetArrows();
    }

    if (isFlashChanged())
    {
      printPointer();
    }

    // always clear update flags by this point
    updateAllItems = false;

    // capture the button down states
    captureButtonDownStates();

    // check for the OK button
    if (btn_2.PressReleased())
    {
      switch (pntrPos)
      {
      case 6:
        currPage = MENU_MAINMENU_ALARMLIST_BACK;
        return;
      default:
        alarmNumber_pntrPos = pntrPos;
        currPage = MENU_MAINMENU_INDIVIDUALALARM;
        return;
      }
    }

    // otherwise check for pointer up or down buttons
    doPointerNavigation();

    pacingWait();
  }
}

void page_MainMenu_IndividualAlarm()
{

  String IndividualAlarmName = "A - " + String(alarmNumber_pntrPos);

  // initializes the menu page
  initMenuPage(IndividualAlarmName, 5);

  // inner loop
  while (true)
  {

    mainLoop();

    // print the display items when requested
    if (updateAllItems)
    {

      // print the visible items
      if (menuItemPrintable(1, 1))
      {
        lcd.print(F("START:: "));
      }
      if (menuItemPrintable(1, 2))
      {
        lcd.print(F("END::   "));
      }
      if (menuItemPrintable(1, 3))
      {
        lcd.print(F("Day :: "));
      }
      if (menuItemPrintable(1, 4))
      {
        lcd.print(F("ON/OFF:: "));
      }
      if (menuItemPrintable(1, 5))
      {
        lcd.print(F("BACK         "));
      }

      // print offset up/down arrows
      printOffsetArrows();
    }

    // print the value for the display item when requested. "updateItem" allows only the pointed record to be updated
    if (updateAllItems || updateItemValue)
    {
      if (menuItemPrintable(10, 1))
      {
        print_StartTime();
      }
      if (menuItemPrintable(10, 2))
      {
        print_EndTime();
      }
      if (menuItemPrintable(8, 3))
      {
        print_DayOfWeek();
      }
      if (menuItemPrintable(10, 4))
      {
        printState();
      }
    }

    if (isFlashChanged())
    {
      printPointer();
    }

    // always clear update flags by this point
    updateAllItems = false;
    updateItemValue = false;

    // capture the button down states
    captureButtonDownStates();

    // check for the OK button
    if (btn_2.PressReleased())
    {
      switch (pntrPos)
      {
      case 1:
        adjustAlarmStartTime();
        break;
      case 2:
        adjustAlarmEndTime();
        break;
      case 3:
        adjustDayOfWeek();
        break;
      case 4:
        adjustState();
        break;
      case 5:
        page_MainMenu_AlarmList();
        return;
      }
    }

    // otherwise check for pointer up or down buttons
    doPointerNavigation();

    pacingWait();
  }
}

void page_MainMenu_IndividualAlarm_Back()
{
  currPage = MENU_ROOT;
  return;
}

void page_MainMenu_AlarmList_Back()
{
  currPage = MENU_MAINMENU;
  return;
}

// void page_MainMenu_Settings()
// {
//   // initializes the menu page
//   initMenuPage(F("Settings"), 4);

//   // inner loop
//   while (true)
//   {
//     mainLoop();

//     // print the display items when requested
//     if (updateAllItems)
//     { 

//       // print the visible items
//       if (menuItemPrintable(1, 1))
//       {
//         lcd.print(F("Water CHK= "));
//       }
//       if (menuItemPrintable(1, 2))
//       {
//         lcd.print(F("Dry CHK  = "));
//       }
//       if (menuItemPrintable(1, 3))
//       {
//         lcd.print(F("Update Time    "));
//       }
//       if (menuItemPrintable(1, 4))
//       {
//         lcd.print(F("BACK           "));
//       }

//       // print offset up/down arrows
//       printOffsetArrows();
//     }

//     // print the value for the display item when requested. "updateItem" allows only the pointed record to be updated
//     if (updateAllItems || updateItemValue)
//     {
//       if (menuItemPrintable(12, 1))
//       {
//         printintAtWidth(settings.waterFlowCheckingInterval_mins, 3, ' ', false);
//         lcd.setCursor(14, 1);
//         lcd.print(F("m"));
//       }
//       if (menuItemPrintable(12, 2))
//       {
//         printintAtWidth(settings.dryRunCheckInterval_mins, 3, ' ', false);
//         lcd.setCursor(14, 1);
//         lcd.print(F("m"));
//       }
//     }

//     if (isFlashChanged())
//     {
//       printPointer();
//     }

//     // always clear update flags by this point
//     updateAllItems = false;
//     updateItemValue = false;

//     // capture the button down states
//     captureButtonDownStates();

//     // check for the OK button
//     if (btn_2.PressReleased())
//     {
//       switch (pntrPos)
//       {
//       case 1:
//         adjustUint8_tb(&settings.waterFlowCheckingInterval_mins, 0, 60);
//         break;
//       case 2:
//         adjustUint8_tb(&settings.dryRunCheckInterval_mins, 0, 60);
//         break;
//       case 3:
//         currPage = MENU_CURRENTALARM;
//         return;
//       case 4:
//         currPage = MENU_MAINMENU;
//         return;
//       }
//     }

//     // otherwise check for pointer up or down buttons
//     doPointerNavigation();

//     pacingWait();
//   }
// }

void page_CurrentTime()
{

  // initializes the menu page
  initMenuPage(F("UPDATE TIME"), 2);

  // inner loop
  while (true)
  {

    // print the display items when requested
    if (updateAllItems)
    {

      // print the visible items
      if (menuItemPrintable(1, 1))
      {
        lcd.print(F("Set Time       "));
      }
      if (menuItemPrintable(1, 2))
      {
        lcd.print(F("BACK           "));
      }

      // print offset up/down arrows
      printOffsetArrows();
    }

    if (isFlashChanged())
    {
      printPointer();
    }

    // always clear update flags by this point
    updateAllItems = false;

    // capture the button down states
    captureButtonDownStates();

    // check for the OK button
    if (btn_2.PressReleased())
    {
      switch (pntrPos)
      {
      case 1:
        currPage = MENU_CURRENTALARM_SETALARM;
        return;
      case 2:
        currPage = MENU_MAINMENU_SETTINGS;
        return;
      }
    }

    // otherwise check for pointer up or down buttons
    doPointerNavigation();

    pacingWait();
  }
}

void page_CurrentTime_SetTime()
{

  // initializes the menu page
  initMenuPage("SET TIME", 1);

  // inner loop
  while (true)
  {

    mainLoop();

    // print the display items when requested
    if (updateAllItems)
    {

      // print the visible items
      if (menuItemPrintable(1, 1))
      {
        lcd.print(F("HH:MM ="));
      }

      if (menuItemPrintable(9, 1))
      {
        displayWithoutTime();
      }

      // print offset up/down arrows
      printOffsetArrows();
    }

    if (isFlashChanged())
    {
      printPointer();
    }

    // always clear update flags by this point
    updateAllItems = false;

    // capture the button down states
    captureButtonDownStates();

    // check for the OK button
    if (btn_2.PressReleased())
    {
      switch (pntrPos)
      {
        case 1: manualSetTime(); currPage = MENU_MAINMENU_SETTINGS; return;
      }
    }

    // otherwise check for pointer up or down buttons
    doPointerNavigation();

    pacingWait();
  }
}

void page_CurrentTime_Back()
{
  currPage = MENU_MAINMENU; return;
}

void page_MainMenu_Back()
{
  currPage = MENU_ROOT;
  return;
}

void page_AlarmOn()
{
  currPage = MENU_MAINMENU_ALARMLIST;
  return;
  // onAlarm = 0;
  // for (int i = 0; i < size; i++)
  // {
  //   if (alarms[i].state == true)
  //   {
  //     onAlarm++;
  //     char alarmNumber = alarms[i].alarmName[strlen(alarms[i].alarmName) - 1];
  //     strcpy(alarmOnList[i], &alarmNumber);
  //   }
  // }

  // Serial.println(alarmOnList[0]);

  // // initializes the menu page
  // initMenuPage(F("ALARMS ON"), onAlarm + 1);

  // // inner loop
  // while (true)
  // {

  //   mainLoop();

  //   // print the display items when requested
  //   if (updateAllItems)
  //   {

  //     // print the visible items
  //     showAlarmsOnList();

  //     // print offset up/down arrows
  //     printOffsetArrows();
  //   }

  //   if (isFlashChanged())
  //   {
  //     printPointer();
  //   }

  //   // always clear update flags by this point
  //   updateAllItems = false;

  //   // capture the button down states
  //   captureButtonDownStates();

  //   // check for the OK button
  //   if (btn_2.PressReleased())
  //   {

  //     if (pntrPos == onAlarm + 1)
  //     {
  //       currPage = MENU_ROOT;
  //       return;
  //     }
  //     else
  //     {
  //       alarmNumber_pntrPos = int(alarmOnList[pntrPos - 1] - '0');
  //       currPage = MENU_MAINMENU_INDIVIDUALALARM;
  //       return;
  //     }
  //   }

  //   // otherwise check for pointer up or down buttons
  //   doPointerNavigation();

  //   pacingWait();
  // }
}

void page_AlarmOn_Back()
{
  currPage = MENU_ROOT;
  return;
}

// =================================================================
// ||                 MENU INTERNALS                              ||
// =================================================================

void initMenuPage(String title, int itemCount)
{

  lcd.clear();
  lcd.setCursor(0, 0);

  int fillCnt = (DISP_CHAR_WIDTH - title.length()) / 2;
  if (fillCnt > 0)
  {
    for (int i = 0; i < fillCnt; i++)
    {
      lcd.print(F("\03"));
    }
  }
  lcd.print(title);
  if ((title.length() % 2) == 1)
  {
    fillCnt++;
  }
  if (fillCnt > 0)
  {
    for (int i = 0; i < fillCnt; i++)
    {
      lcd.print(F("\04"));
    }
  }

  // clear all button down states
  btn_1.ClearWasDown();
  btn_2.ClearWasDown();

  // set the menu item count
  itemCnt = itemCount;

  // current pointer position
  pntrPos = 1;

  // display offset
  dispOffset = 0;

  // flash counter > force immediate draw at startup
  flashCntr = 0;

  // flash state > will switch to being on at startup
  flashIsOn = true;

  // force a full update
  updateAllItems = true;

  // capture start time
  loopStartMs = millis();
}

void captureButtonDownStates()
{
  btn_1.CaptureDownState();
  btn_2.CaptureDownState();
}

void adjustBoolean(boolean *v)
{

  // capture the button down states
  captureButtonDownStates();

  if (btn_2.PressReleased())
  {
    *v = !*v;
    updateItemValue = true;
  }
}

void adjustUint8_tb(int *v, int min, int max)
{
  // capture the button down states
  captureButtonDownStates();

  if (!btn_2.PressReleased())
  // Serial.println(settings.waterFlowCheckingInterval_mins);
  {
    if ((*v > min) && (*v < max))
    {
      *v = *v + 1;
      updateItemValue = true;
    }
  }
}

// void adjustWaterCHK(int min, int max)
// {
//   // capture the button down states
//   captureButtonDownStates();
//   Serial.println(btn_2.PressReleased());
//   if (!btn_2.PressReleased())
//   {
//     Serial.println(waterFlowCheckingInterval_mins);
//     if ((waterFlowCheckingInterval_mins > min) && (waterFlowCheckingInterval_mins < max))
//     {
//       waterFlowCheckingInterval_mins = waterFlowCheckingInterval_mins + 1;
//       updateItemValue = true;
//     }
//   }
// }

// void adjustDryCHK(int min, int max)
// {
//   // capture the button down states
//   captureButtonDownStates();

//   if (btn_2.PressReleased())
//   {
//     if ((dryRunCheckInterval_mins > min) && (dryRunCheckInterval_mins < max))
//     {
//       dryRunCheckInterval_mins = dryRunCheckInterval_mins + 1;
//       updateItemValue = true;
//     }
//   }
// }

void doPointerNavigation()
{
  if (btn_1.PressReleased())
  {

    if (pntrPos < itemCnt)
    {
      // erase the current pointer & reset the flash state to get immediate redraw
      flashIsOn = false;
      flashCntr = 0;
      printPointer();

      // if the cursor is already at the bottom of the display then request a list update too
      if (pntrPos - dispOffset == DISP_ITEM_ROWS)
      {
        updateAllItems = true;
        dispOffset++;
      }

      // move the pointer
      pntrPos++;
    }
    else if (pntrPos == itemCnt)
    {
      // erase the current pointer & reset the flash state to get immediate redraw
      flashIsOn = false;
      flashCntr = 0;
      printPointer();

      pntrPos = 1;

      dispOffset = 0;

      updateAllItems = true;
    }
  }
}

boolean isFlashChanged()
{

  // check if counter expired
  if (flashCntr == 0)
  {

    // flip the pointer state
    flashIsOn = !flashIsOn;

    // reset the pointer counter
    flashCntr = FLASH_RST_CNT;

    // indicate it is time to flash
    return true;
  }
  else
  {
    flashCntr--;
    return false;
  }
}

void pacingWait()
{

  while (millis() - loopStartMs < PACING_MS)
  {
    delay(1);
  }

  loopStartMs = millis();
}

boolean menuItemPrintable(int xPos, int yPos)
{
  // basic check to see if the basic conditions to allow showing this item
  if (!(updateAllItems || (updateItemValue && pntrPos == yPos)))
  {
    return false;
  }

  // make a value use later to check if there is enough offset to make the value visible
  int yMaxOffset = 0;

  // this case means the value is typically beyond the offset display, so we remove the visible row count
  if (yPos > DISP_ITEM_ROWS)
  {
    yMaxOffset = yPos - DISP_ITEM_ROWS;
  }

  // taking into account any offset, check if the item position is currently visible -> position cursor and return true if so
  if (dispOffset <= (yPos - 1) && dispOffset >= yMaxOffset)
  {
    lcd.setCursor(xPos, yPos - dispOffset);
    return true;
  }

  // otherwise just return false
  return false;
}

// =================================================================
// ||                 TOOLS - DISPLAY                             ||
// =================================================================

void printPointer()
{

  // if (itemCnt < 2){return;}

  // move the cursor
  lcd.setCursor(0, pntrPos - dispOffset);

  // lcd.print(F("\05"));
  // show the pointer if set
  if (flashIsOn)
  {
    lcd.print(F("\05"));
  }

  // otherwise hide the pointer
  else
  {
    lcd.print(F(" "));
  }
}

void printOffsetArrows()
{

  // print the offset Up arrow depending on the offset value
  lcd.setCursor(DISP_CHAR_WIDTH - 1, 1);
  if (dispOffset > 0)
  {
    lcd.print(F("\01"));
  }
  else
  {
    lcd.print(F(" "));
  }

  lcd.setCursor(DISP_CHAR_WIDTH - 1, DISP_ITEM_ROWS);
  if ((itemCnt > DISP_ITEM_ROWS) && ((itemCnt - DISP_ITEM_ROWS) > dispOffset))
  {
    lcd.print(F("\02"));
  }
  else
  {
    lcd.print(F(" "));
  }
}

void printOnOff(boolean val)
{
  if (val)
  {
    lcd.print(F("ON "));
  }
  else
  {
    lcd.print(F("OFF"));
  }
}

// prints the specified character for the specified number of times
void printChars(int cnt, char c)
{

  // only bother if char count is more than zero
  if (cnt > 0)
  {

    // build the character array with the single character ready for printing
    char cc[] = " ";
    cc[0] = c;

    // print "cnt" number of the specified character
    for (int i = 1; i <= cnt; i++)
    {
      lcd.print(cc);
    }
  }
}

int getintCharCnt(int value)
{

  // value is zero then return 1
  if (value == 0)
  {
    return 1;
  }

  // setup the working variables
  int tensCalc = 10;
  int cnt = 1;

  // keeps increasing the tensClac value and counting digits for as long as the tensClac value is lower than the value
  while (tensCalc <= value && cnt < 20)
  {
    tensCalc *= 10;
    cnt += 1;
  }

  // return the result
  return cnt;
}

void printintAtWidth(int value, int width, char c, boolean isRight)
{

  // get the number of character in the unformatted number
  int numChars = getintCharCnt(value);

  // if right justified then add the justification characters first
  if (isRight)
  {
    printChars(width - numChars, c);
  }

  // print the value
  lcd.print(value);

  // if is left justified then add the justification characters last
  if (!isRight)
  {
    printChars(width - numChars, c);
  }
}

// =================================================================
// ||                 TOOLS - SETTINGS                             ||
// =================================================================

void sets_SetDefaults()
{
  MySettings tempSets;
  memcpy(&settings, &tempSets, sizeof settings);
}

void sets_Load()
{
  // load the values
  // EEPROM.get(0, settings);

  // if the check value does not match then load the defaults
  if (settings.settingCheckValue != SETTINGS_CHKVAL)
  {
    sets_SetDefaults();
  }
}

void sets_Save()
{
  // EEPROM.put(0, settings);
}

// =================================================================
// ||                 Current Time                                ||
// =================================================================

void displayTime()
{
  RtcDateTime timeNow = rtc.GetDateTime();

  lcd.setCursor(1, 1);
  lcd.print("TIME = ");
  if (timeNow.Hour() <= 9)
  {
    lcd.print(0);
    lcd.print(timeNow.Hour());
  }
  else
  {
    lcd.print(timeNow.Hour());
  }
  lcd.print(":");
  if (timeNow.Minute() <= 9)
  {
    lcd.print(0);
    lcd.print(timeNow.Minute());
    lcd.print("           ");
  }
  else
  {
    lcd.print(timeNow.Minute());
    lcd.print("           ");
  }
}

void displayWithoutTime()
{
  RtcDateTime timeNow = rtc.GetDateTime();

  if (timeNow.Hour() <= 9)
  {
    lcd.print(0);
    lcd.print(timeNow.Hour());
  }
  else
  {
    lcd.print(timeNow.Hour());
  }
  lcd.print(":");
  if (timeNow.Minute() <= 9)
  {
    lcd.print(0);
    lcd.print(timeNow.Minute());
    lcd.print("           ");
  }
  else
  {
    lcd.print(timeNow.Minute());
    lcd.print("           ");
  }
}

boolean isMinuteChanged()
{
  RtcDateTime timeNow = rtc.GetDateTime();
  static int prevMinute = timeNow.Minute();

  if (timeNow.Minute() != prevMinute)
  {
    prevMinute = timeNow.Minute();
    return true;
  }
  return false;
}

void print_ManualTime()
{
  // HOUR
  if (manualHour <= 9)
  {
    lcd.print(0);
    lcd.print(manualHour);
  }
  else
  {
    lcd.print(manualHour);
  }

  lcd.print(":");

  // MINUTE
  if (manualMinute <= 9)
  {
    lcd.print(0);
    lcd.print(manualMinute);
  }
  else
  {
    lcd.print(manualMinute);
  }
}

void manualSetTime()
{
  RtcDateTime timeNow = rtc.GetDateTime();
  manualHour = timeNow.Hour();
  manualMinute = timeNow.Minute();

  while (true)
  { // Changing HOUR
    // capture the button down states
    captureButtonDownStates();

    if (btn_1.Repeated())
    {
      if (manualHour >= 0 && manualHour < 24)
      {
        manualHour++;
      }
      if (manualHour >= 24)
      {
        manualHour = 0;
      }
      lcd.setCursor(9, 1);
      print_ManualTime();
    }
    if (btn_2.PressReleased())
    {
      break;
    }
  }

  while (true)
  { // Changing MINUTE

    // capture the button down states
    captureButtonDownStates();

    if (btn_1.Repeated())
    {
      if (manualMinute >= 0 && manualMinute < 60)
      {
        manualMinute++;
      }
      if (manualMinute >= 60)
      {
        manualMinute = 0;
      }
      lcd.setCursor(9, 1);
      print_ManualTime();
    }
    if (btn_2.PressReleased())
    {
      
      char *hourpntr = itoa(manualHour, charManualHour, 10);
      char *minutepntr = itoa(manualMinute, charManualMinute, 10);
      char *pntr_1 = strcat(manual, charManualHour);
      char *pntr_2 = strcat(manual, ":");
      char *pntr_3 = strcat(manual, charManualMinute);
      char *pntr_4 = strcat(manual, ":");
      char *pntr_5 = strcat(manual, "0");

      // RtcDateTime setTime = RtcDateTime(__DATE__, manual);
      // rtc.SetDateTime(setTime);
      break;
    }
    // updateItemValue = true;
  }
  currPage = MENU_MAINMENU_SETTINGS;
  return;
}

// ALRAM LIST SETTINGS

void showAlarmsList()
{
  for (int i = 1; i <= size; i++)
  {
    if (menuItemPrintable(1, i))
    {
      lcd.print(F(alarms[i - 1].alarmName));
      lcd.setCursor(11, 1);
      if (alarms[i - 1].state)
      {
        lcd.print(F("ON  "));
      }
      else
      {
        lcd.print(F("OFF"));
      }
    }
  }
  if (menuItemPrintable(1, 6))
  {
    lcd.print(F("BACK           "));
  }
}

void showAlarmsOnList()
{
  for (int i = 1; i <= onAlarm; i++)
  {
    if (menuItemPrintable(1, i))
    {
      lcd.print(F(alarms[i - 1].alarmName));
      lcd.setCursor(11, 1);
      if (alarms[i - 1].state)
      {
        lcd.print(F("ON  "));
      }
      else
      {
        lcd.print(F("OFF"));
      }
    }
  }
  if (menuItemPrintable(1, onAlarm + 1))
  {
    lcd.print(F("BACK           "));
  }
}

// INDIVIDUAL ALARM SETTINGS

void print_DayOfWeek()
{
    lcd.print(F(week[alarms[alarmNumber_pntrPos - 1].alarmDay]));
}

void printState()
{
  if (alarms[alarmNumber_pntrPos - 1].state)
  {
    lcd.print(F("ON   "));
  }
  else
  {
    {
      lcd.print(F("OFF  "));
    }
  }
}

void print_StartTime()
{
  // HOUR
  if (alarms[alarmNumber_pntrPos - 1].startTime[0] <= 9)
  {
    lcd.print(0);
    lcd.print(alarms[alarmNumber_pntrPos - 1].startTime[0]);
  }
  else
  {
    lcd.print(alarms[alarmNumber_pntrPos - 1].startTime[0]);
  }

  lcd.print(":");

  // MINUTE
  if (alarms[alarmNumber_pntrPos - 1].startTime[1] <= 9)
  {
    lcd.print(0);
    lcd.print(alarms[alarmNumber_pntrPos - 1].startTime[1]);
  }
  else
  {
    lcd.print(alarms[alarmNumber_pntrPos - 1].startTime[1]);
  }
}

void print_EndTime()
{
  // HOUR
  if (alarms[alarmNumber_pntrPos - 1].endTime[0] <= 9)
  {
    lcd.print(0);
    lcd.print(alarms[alarmNumber_pntrPos - 1].endTime[0]);
  }
  else
  {
    lcd.print(alarms[alarmNumber_pntrPos - 1].endTime[0]);
  }

  lcd.print(":");

  // MINUTE
  if (alarms[alarmNumber_pntrPos - 1].endTime[1] <= 9)
  {
    lcd.print(0);
    lcd.print(alarms[alarmNumber_pntrPos - 1].endTime[1]);
  }
  else
  {
    lcd.print(alarms[alarmNumber_pntrPos - 1].endTime[1]);
  }
}

void adjustAlarmStartTime()
{

  while (true)
  { // Changing HOUR
    // capture the button down states
    captureButtonDownStates();

    if (btn_1.Repeated())
    {
      if (alarms[alarmNumber_pntrPos - 1].startTime[0] >= 0 && alarms[alarmNumber_pntrPos - 1].startTime[0] < 24)
      {
        alarms[alarmNumber_pntrPos - 1].startTime[0]++;
      }
      if (alarms[alarmNumber_pntrPos - 1].startTime[0] >= 24)
      {
        alarms[alarmNumber_pntrPos - 1].startTime[0] = 0;
      }
      lcd.setCursor(10, 1);
      print_StartTime();
    }
    if (btn_2.PressReleased())
    {
      break;
    }
  }

  while (true)
  { // Changing MINUTE

    // capture the button down states
    captureButtonDownStates();

    if (btn_1.Repeated())
    {
      if (alarms[alarmNumber_pntrPos - 1].startTime[1] >= 0 && alarms[alarmNumber_pntrPos - 1].startTime[1] < 60)
      {
        alarms[alarmNumber_pntrPos - 1].startTime[1]++;
      }
      if (alarms[alarmNumber_pntrPos - 1].startTime[1] >= 60)
      {
        alarms[alarmNumber_pntrPos - 1].startTime[1] = 0;
      }
      lcd.setCursor(10, 1);
      print_StartTime();
    }
    if (btn_2.PressReleased())
    {
      break;
    }
  }
  updateItemValue = true;
}

void adjustAlarmEndTime()
{

  while (true)
  { // Changing HOUR

    // capture the button down states
    captureButtonDownStates();

    if (btn_1.Repeated())
    {
      if (alarms[alarmNumber_pntrPos - 1].endTime[0] >= 0 && alarms[alarmNumber_pntrPos - 1].endTime[0] < 24)
      {
        alarms[alarmNumber_pntrPos - 1].endTime[0]++;
      }
      if (alarms[alarmNumber_pntrPos - 1].endTime[0] >= 24)
      {
        alarms[alarmNumber_pntrPos - 1].endTime[0] = 0;
      }
      lcd.setCursor(10, 1);
      print_EndTime();
    }
    if (btn_2.PressReleased())
    {
      break;
    }
  }

  while (true)
  { // Changing MINUTE

    // capture the button down states
    captureButtonDownStates();

    if (btn_1.Repeated())
    {
      if (alarms[alarmNumber_pntrPos - 1].endTime[1] >= 0 && alarms[alarmNumber_pntrPos - 1].endTime[1] < 60)
      {
        alarms[alarmNumber_pntrPos - 1].endTime[1]++;
      }
      if (alarms[alarmNumber_pntrPos - 1].endTime[1] >= 60)
      {
        alarms[alarmNumber_pntrPos - 1].endTime[1] = 0;
      }
      lcd.setCursor(10, 1);
      print_EndTime();
    }
    if (btn_2.PressReleased())
    {
      break;
    }
  }
  updateItemValue = true;
}

void adjustState()
{

  while (true)
  {
    // capture the button down states
    captureButtonDownStates();

    if (btn_1.Repeated())
    {
      alarms[alarmNumber_pntrPos - 1].state = !alarms[alarmNumber_pntrPos - 1].state;
      lcd.setCursor(10, 1);
      printState();
    }
    if (btn_2.PressReleased())
    {
      break;
    }
  }
}

void adjustDayOfWeek()
{
  while (true)
  {
    // capture the button down states
    captureButtonDownStates();

    if (btn_1.Repeated())
    {
      if (alarms[alarmNumber_pntrPos - 1].alarmDay < 6)
      {
      alarms[alarmNumber_pntrPos - 1].alarmDay += 1;
      lcd.setCursor(8, 1);
      print_DayOfWeek();
      }
      else
      {
        alarms[alarmNumber_pntrPos - 1].alarmDay = 0;
        lcd.setCursor(8, 1);
        print_DayOfWeek();
      }
    }
    if (btn_2.PressReleased())
    {
      break;
    }
  }
}

// =================================================================
// ||                   MAIN TANK SETTTINGS                       ||
// =================================================================

void mainLoop()
{
  RtcDateTime timeNow = rtc.GetDateTime();
  mainsOnOff();
  led_Off();
  // Serial.println(checkWaterFlow());
  // Serial.println(relayState);
  // Serial.println(settings);

  // Serial.print("Start "); Serial.print(alarms[0].startTime[0]); Serial.print(":"); Serial.println(alarms[0].startTime[1]);
  // Serial.println(alarms[0].state);

  // Serial.println(settings.waterFlowCheckingInterval_mins);
  // Serial.println(settings.tankFullStateInterval_mins);
  // Serial.println(settings.dryRunCheckInterval_mins);
  
  // Serial.print(timeNow.Hour());
  // Serial.println(timeNow.Minute());

  // Serial.println(settings.waterFlowCheckingInterval_mins);
  // Serial.println((unsigned long)(settings.waterFlowCheckingInterval_mins*60.0f*1000.0f));
  // Serial.println(settings.waterFlowCheckingInterval);

  // Serial.begin(115200);
  // float waterFlowCheckingInterval_mins = 1.0f;
  // float intermediate = waterFlowCheckingInterval_mins * 60.0f * 1000.0f;
  // Serial.print("Intermediate float result: ");
  // Serial.println(intermediate);

  // unsigned long waterFlowCheckingInterval = (unsigned long)intermediate;
  // Serial.print("Converted unsigned long value: ");
  // Serial.println(waterFlowCheckingInterval);


  if (checkAlarm())
  { //  || wasRunningAlarm()
    updateTankFullDisplay = true;

    while (true)
    {
      // Serial.println("Here 1st");

      if (!checkAlarm())
      {
        // led_Off();
        break;}

      if (tankFullState() == true)
      {
        // Serial.println("Here 2nd");

        previousMillis = millis();
        updateTankState = true;

        if (updateTankState)
        {
          // Serial.println("Here 3rd");

          while (true)
          {
            // Serial.println("Here 4th");

            if ((millis() - previousMillis) >= (unsigned long)(settings.tankFullStateInterval_mins*60.0f*1000.0f))
            {
              // buzzerOff();
              // Serial.println("Here 5th");
              relayOff();
              updateTankState = false;

              if (tankFullState() == false) {
                break;}

              if (!checkAlarm())
              {
                // Serial.println("Here 6th");
                // led_Off();
                break;}
              
            }
            else
            {
              // Serial.println("Here 2nd");
              // if ((millis() - previousMillis) >= (unsigned long)(settings.buzzerInterval_mins*60.0f*1000.0f)) {buzzerOff();}
              // else {buzzerOn();}

              relayOn();
              mainsOnOff();
              tankFullDisplay();
              green();
              // buzzerOn();

              if (!checkAlarm())
              {
                // led_Off();
                break;}
              }
          }
        }
      }
      else
      {
        Serial.println(tankFullState());
        led_Off();
        dryRunCheck();

        if (!checkAlarm())
        {
          break;
        }

        updateTankFullDisplay = true;
      }
    }
    root_pntrPos = 1;
    root_dispOffset = 0;
    // lcd.clear();
    // page_MenuRoot();
    relayOff();
  }
}

void mainsOnOff()
{ // MANUAL ON OFF for the relay
  
  parametersBT();

  capturedownstate();

  if (pressreleased())
  {
    relayState = !relayState;
    digitalWrite(relayPin, relayState);

  }
}

void relayOn()
{
  relayState = true;
  digitalWrite(relayPin, relayState);
}

void relayOff()
{
  relayState = false;
  digitalWrite(relayPin, relayState);
}

// boolean checkAlarm()
// {
//   RtcDateTime timeNow = rtc.GetDateTime();

//   Serial.println("------------------");
//   for (int i = 0; i < size; i++)
//   {
//     // Serial.println(alarms[i].alarmDay);
//     // Serial.println(timeNow.DayOfWeek());
//     // Serial.println()
//     if (
//         alarms[i].state == true &&
//         alarms[i].startTime[0] <= timeNow.Hour() &&
//         alarms[i].startTime[1] <= timeNow.Minute() &&
//         alarms[i].endTime[0] >= timeNow.Hour() &&
//         alarms[i].endTime[1] >= timeNow.Minute())
//         // alarms[i].endTime[0] <= timeNow.Hour() &&
//         // alarms[i].endTime[1] <= timeNow.Minute())

//         // alarms[i].alarmDay == timeNow.DayOfWeek()) // THINK ABOUT ITS USE!!!!!!!!!!!!!!!!!

//         // alarms[i].startTime[0] <= timeNow.Hour() <= alarms[i].endTime[0] &&
//         // alarms[i].startTime[1] <= timeNow.Minute() <= alarms[i].endTime[1])
//     {
//       Serial.print("checkAlarm ");
//       // Serial.println(alarms[currentlyRunningAlarm].startTime[0]);
//       Serial.println(currentlyRunningAlarm);

//       currentlyRunningAlarm = i;
//       return true;
//     }
//     Serial.print(alarms[i].state == true); Serial.print("      "); 
//     Serial.print(alarms[i].startTime[0] <= timeNow.Hour()); Serial.print("      "); 
//     Serial.print(alarms[i].startTime[1] <= timeNow.Minute()); Serial.print("      "); 
//     Serial.print(alarms[i].endTime[0] >= timeNow.Hour()); Serial.print("      "); 
//     Serial.println(alarms[i].endTime[1] >= timeNow.Minute());
//   }
//   Serial.println("------------------");
//   return false;
// }

// boolean wasRunningAlarm()
// {

//   RtcDateTime timeNow = rtc.GetDateTime();

//   if (
//       alarms[currentlyRunningAlarm].startTime[0] <= timeNow.Hour() <= alarms[currentlyRunningAlarm].endTime[0] &&
//       alarms[currentlyRunningAlarm].startTime[1] == timeNow.Minute() <= alarms[currentlyRunningAlarm].endTime[1] &&
//       alarms[currentlyRunningAlarm].state == true)
//   {
//     return true;
//   }
//   else
//   {
//     return false;
//   }
// }

boolean checkAlarm() {
  RtcDateTime timeNow = rtc.GetDateTime();

  Serial.println("------------------");
  for (int i = 0; i < size; i++) {
    bool isActive = alarms[i].state;

    bool startTimePassed = (alarms[i].startTime[0] < timeNow.Hour()) ||
                           (alarms[i].startTime[0] == timeNow.Hour() && alarms[i].startTime[1] <= timeNow.Minute());

    bool endTimeNotPassed = (alarms[i].endTime[0] > timeNow.Hour()) ||
                            (alarms[i].endTime[0] == timeNow.Hour() && alarms[i].endTime[1] > timeNow.Minute());

    // Handle alarms that go past midnight
    bool overnightAlarm = alarms[i].startTime[0] > alarms[i].endTime[0];

    // Ensure correct logic for alarms spanning multiple days
    if (isActive && (startTimePassed || overnightAlarm) && (endTimeNotPassed || overnightAlarm)) {
      currentlyRunningAlarm = i;
      // Serial.print("Alarm Running: ");
      // Serial.println(alarms[i].alarmName);
      return true;
    }

    // Serial.print("Alarm "); Serial.print(i+1);
    // Serial.print(" | State: "); Serial.print(isActive);
    // Serial.print(" | Start OK: "); Serial.print(startTimePassed);
    // Serial.print(" | End OK: "); Serial.println(endTimeNotPassed);
  }
  
  
  // Serial.println("No active alarms.");
  // Serial.println("------------------");
  return false;
}

boolean wasRunningAlarm() {
  if (currentlyRunningAlarm < 0 || currentlyRunningAlarm >= size) {
    return false; // No valid running alarm
  }

  RtcDateTime timeNow = rtc.GetDateTime();
  Alarms &alarm = alarms[currentlyRunningAlarm];

  bool startTimePassed = (alarm.startTime[0] < timeNow.Hour()) ||
                         (alarm.startTime[0] == timeNow.Hour() && alarm.startTime[1] <= timeNow.Minute());

  bool endTimeNotPassed = (alarm.endTime[0] > timeNow.Hour()) ||
                          (alarm.endTime[0] == timeNow.Hour() && alarm.endTime[1] > timeNow.Minute());

  bool overnightAlarm = alarm.startTime[0] > alarm.endTime[0];

  return alarm.state && (startTimePassed || overnightAlarm) && (endTimeNotPassed || overnightAlarm);
}



boolean tankFullState()
{ // Check if the tank if full or not

  float average = 0.0f;
  for (int i = 0; i < settings.tankFullCheckAverageTimes; i++)
  {
    average = average + analogRead(tankStatePin);
  }
  average = average / settings.tankFullCheckAverageTimes;

  if (average <= settings.threshold_tankStatePin)
  { // The Tank is Full.
  Serial.println("Work");
    return true;
  }
  else
  {
    return false;
  }
}

boolean checkWaterFlowState()
{ // returns true if the water is flowing

  float average = 0.0f;
  for (int i = 0; i < settings.waterFlowCheckAverageTimes; i++)
  {
    average = average + analogRead(potPin);
  }
  average = average / settings.waterFlowCheckAverageTimes;
  if (average <= settings.threshold_waterCheckPin)
  {
    return true;
  }
  else
  {
    return false;
  }
}

boolean checkWaterFlow()
{
  Serial.print("checkWaterFlow  ");
  updateCheckWaterFlow = true;

  previousMillis = millis();
  digitalWrite(relayPin, HIGH);
  // lcd.clear();

  while (true)
  {

    if ((millis() - previousMillis) > (unsigned long)(settings.waterFlowCheckingInterval_mins*60.0f*1000.0f))
    {

      // Serial.println((millis() - previousMillis) > settings.waterFlowCheckingInterval);

      led_Off();
      if (!checkAlarm())
      {
        return false;
      }
      if (tankFullState() == true)
      {
        return false;
      }

      if (checkWaterFlowState() == true)
      {
        digitalWrite(relayPin, LOW);
        return true;
      }
      else
      {
        digitalWrite(relayPin, LOW);
        return false;
      }
    }
    else
    {

      
      mainsOnOff(); // To turn ON and OFF relay
      Serial.println(analogRead(potPin));
      Serial.println(tankFullState());

      // if (tankFullState() == true)
      // {
      //   return;
      // }

      // Serial.println(settings.waterFlowCheckingInterval);

      // Serial.println((millis() - previousMillis));

      if (!checkAlarm())
      {
        return false;
      }

      // if (tankFullState() == true) // LATER
      // {
      //   return false;
      // }

      if (updateCheckWaterFlow)
      {
        blue();
        mainsOnOff();
        // lcd.setCursor(0, 0);
        // lcd.print(F("WaterFlow Check"));
        // lcd.setCursor(0, 1);
        // lcd.print(F("Wait "));
        // lcd.print(settings.waterFlowCheckingInterval_mins);
        // lcd.print(F("mins"));

        updateCheckWaterFlow = false;
      }
    }
  }
}

void tankFullDisplay()
{
  if (updateTankFullDisplay)
  {
    relayOff();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Tank is Full!");
    updateTankFullDisplay = false;

    // millisBuzzer = millis();
    // while(true) {
    //   if ((millis() - millisBuzzer) >= buzzerWait) {
    //     buzzerOff();
    //     break;
    //   }
    //   else {
    //     buzzerOn();
    //   }
    // }
  }
}

boolean durationComplete()
{
  RtcDateTime timeNow = rtc.GetDateTime();
  // Serial.print("HeY   ");
  // Serial.print(timeNow.Minute());
  // Serial.print("    ");
  // Serial.println(currentlyRunningAlarm);

  if (
      alarms[currentlyRunningAlarm].endTime[0] <= timeNow.Hour() &&
      alarms[currentlyRunningAlarm].endTime[1] <= timeNow.Minute())
      //  &&
      // alarms[currentlyRunningAlarm].alarmDay == timeNow.DayOfWeek())
  {
    // Serial.println("Working");
    relayOff();
    return true;
  }
  else
  {
    return false;
  }
}

void waitTime()
{
  // Serial.println("waitTime");
  updateWaitTime = true;
  previousMillis = millis();

  relayOff();

  while (true)
  {
    if ((millis() - previousMillis) >= (unsigned long)(settings.dryRunCheckInterval_mins*60.0f*1000.0f))
    {

      led_Off();
      if (!checkAlarm())
      {
        return;
      }
      if (tankFullState() == true)
      {
        return;
      }

      dryRunCheck();
    }
    else
    {
      mainsOnOff();

      if (!checkAlarm())
      {
        return;
      }

      if (updateWaitTime)
      {
        red();

        if (tankFullState() == true)
        {
          return;
        }

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("No Water Flow!!"));
        lcd.setCursor(0, 1);
        lcd.print(F("Wait "));
        lcd.setCursor(5, 1);
        lcd.print(settings.dryRunCheckInterval_mins);
        lcd.print(F("mins"));

        updateWaitTime = false;
      }
    }
  }
}

void dryRunCheck()
{
  updateDryRunCheck = true;
  RtcDateTime timeNow = rtc.GetDateTime();

  if (tankFullState() == true)
  {
    return;
  }
  else
  {
    if (checkWaterFlow() == false)
    {

      if (tankFullState() == true)
      {
        return;
      }

      if (!checkAlarm())
      {
        return;
      }

      waitTime(); // waits for x time
    }
    else
    {

      while (true)
      {
        if (checkWaterFlowState())
        {
          blue();

          mainsOnOff();
          // Serial.println(relayState);
          // Serial.println(analogRead(32));
          // Serial.println(analogRead(33));

          if (!checkAlarm())
          {
            return;
          }

          if (updateDryRunCheck)
          {
            relayState = true;
            digitalWrite(relayPin, HIGH);
            // lcd.clear();
            // lcd.setCursor(0, 0);
            // lcd.print(F("TANK FILLING \05\05"));
            updateDryRunCheck = false;
          }

          if (tankFullState() == true)
          {
            return;
          }
        }
        else
        {

          led_Off();
          relayOff();
          // lcd.clear();
          // lcd.setCursor(0, 0);
          // lcd.print("Flow Stopped!!");
          if (tankFullState() == true)
          {
            return;
          }
          dryRunCheck();
        }
      }
    }
  }
}

// BUZZER

void buzzerOn() {
  digitalWrite(buzzerPin, HIGH);
}

void buzzerOff() {
  digitalWrite(buzzerPin, LOW);
}

// BUTTON 3

boolean capturedownstate()
{
  if (isdown())
  {
    wasdown = true;
  }
  return wasdown;
}

boolean isdown() { return digitalRead(BTN_3) == LOW && digitalRead(BTN_3) == LOW; }

boolean isup() { return digitalRead(BTN_3) == HIGH && digitalRead(BTN_3) == HIGH; }

boolean pressreleased()
{
  if (wasdown && isup())
  {
    wasdown = false;
    return true;
  }
  return false;
}

boolean outOfAlarm()
{
  static boolean previousButtonState = LOW;
  int interval = 5000;
  static unsigned long previousMillis_1;

  if (isdown() == previousButtonState)
  {
    previousMillis_1 = millis();
  }

  if ((millis() - previousMillis_1) > interval)
  {
    return true;
  }
  else
  {
    return false;
  }
}

void red() {
  digitalWrite(redPin, 255);
  digitalWrite(greenPin, 0);
  digitalWrite(bluePin, 0);
}

void green() {
  digitalWrite(redPin, 0);
  digitalWrite(greenPin, 255);
  digitalWrite(bluePin, 0);
}

void blue() {
  digitalWrite(redPin, 0);
  digitalWrite(greenPin, 0);
  digitalWrite(bluePin, 255);
}

void led_Off() {
  digitalWrite(redPin, 0);
  digitalWrite(greenPin, 0);
  digitalWrite(bluePin, 0);
}

// Format for String
template <typename... Args>
String format(const String& fmt, Args... args) {
    String result = fmt;
    String arguments[] = { String(args)... };  // Expand arguments into an array of Strings

    for (int i = 0; i < sizeof...(args); i++) {
        String placeholder = "{" + String(i) + "}";  // Placeholder pattern {0}, {1}, etc.
        result.replace(placeholder, arguments[i]);  // Replace placeholders with arguments
    }
    return result;
}

// Split function for String
void split(String data, char delimiter, String parts[], int maxParts) {
    int startIndex = 0;
    int endIndex = 0;
    int partIndex = 0;

    while ((endIndex = data.indexOf(delimiter, startIndex)) != -1) {
        if (partIndex >= maxParts) break;
        parts[partIndex++] = data.substring(startIndex, endIndex);
        startIndex = endIndex + 1;
    }
    if (partIndex < maxParts) {
        parts[partIndex++] = data.substring(startIndex); // Add the last part
    }
}

// Checking for Substring
bool isSubstringInString(String text, String substring) {
    return text.indexOf(substring) != -1;  // Returns true if substring is found
}


void parametersBT() {
  if (SerialBT.available()) {
    String c = SerialBT.readString();

    Serial.println(c);

    // Show current alarm
    for (int i = 1; i <= size; i++){
      if(c == format("Alarm {0}p", i)) {
      //   Serial.print(alarms[i-1].startTime[0]); Serial.print(":"); Serial.print(alarms[i-1].startTime[1]);
      //   Serial.print("::");
      //   Serial.print(alarms[i-1].endTime[0]); Serial.print(":"); Serial.print(alarms[i-1].endTime[1]); 
        SerialBT.print(alarms[i-1].startTime[0]); SerialBT.print(":"); SerialBT.print(alarms[i-1].startTime[1]);
        SerialBT.print("::");
        SerialBT.print(alarms[i-1].endTime[0]); SerialBT.print(":"); SerialBT.print(alarms[i-1].endTime[1]);
        SerialBT.print("::");
        SerialBT.print(alarms[i-1].state);
        // clearSerialMonitor();
      }
    }

    if (c == "timings") {
      SerialBT.print(settings.waterFlowCheckingInterval_mins);
      SerialBT.print(",");
      SerialBT.print(settings.tankFullStateInterval_mins);
      SerialBT.print(",");
      SerialBT.print(settings.dryRunCheckInterval_mins);

      // Serial.print(settings.waterFlowCheckingInterval_mins);
      // Serial.println(",");
      // Serial.print(settings.tankFullStateInterval_mins);
      // Serial.println(",");
      // Serial.print(settings.dryRunCheckInterval_mins);
    }

    // Serial.println(c);
  //   RtcDateTime timeNow = rtc.GetDateTime();
  //     Serial.print(timeNow.Hour());
  // Serial.println(timeNow.Minute());
    
    String newWaterTime[20];
    split(c, ',', newWaterTime, 20);

    if ( newWaterTime[0] == "waterFillTime") {
      settings.waterFlowCheckingInterval_mins = atof(newWaterTime[1].c_str()); 
      EEPROM.write(0, settings.waterFlowCheckingInterval_mins);
      // Serial.println(settings.waterFlowCheckingInterval_mins);
    }

    String newTankTime[20];
    split(c, ',', newTankTime, 20);

    if ( newTankTime[0] == "tankTime") {
      settings.tankFullStateInterval_mins = atof(newTankTime[1].c_str());
      EEPROM.write(1, settings.tankFullStateInterval_mins);
      // Serial.println(settings.tankFullStateInterval_mins);
    }

    String newDryRunCheckTime[20];
    split(c, ',', newDryRunCheckTime, 20);

    if ( newDryRunCheckTime[0] == "dryRunCheckTime") {
      settings.dryRunCheckInterval_mins = atof(newDryRunCheckTime[1].c_str());
      EEPROM.write(2, settings.dryRunCheckInterval_mins);
      // Serial.println(settings.dryRunCheckInterval_mins);
    }

    // RELAY
    if(c == "y") {
      relayOn();
    } else if (c == "z") {
      relayOff();
    }

    // Alarm State On/Off
    for (int i = 1; i <= size; i++){
      if(c == format("Alarm {0}t", i)) {
        alarms[i-1].state = true;
        // EEPROM.write(4, i);
        EEPROM.write(3 + 5*(i-1), alarms[i-1].state);
        // Serial.println(alarms[i].state);
      } else if (c == format("Alarm {0}f", i)) {
        alarms[i-1].state = false;
        // EEPROM.write(4, i);
        EEPROM.write(3 + 5*(i-1), alarms[i-1].state);
        // Serial.println(alarms[i].state);
      }
    }

    String parts[5];
    split(c, ':', parts, 5);
    // Serial.println(format("Alarm {0}", 1));
    // Serial.println(parts[0]);

    String time[20];
    split(c, ',', time, 20);

    // Set Current time to RTC

    if (time[0] == "current") {
      // String subString = time[1].substring(0, 10);

      const char* newDate = time[2].c_str();
      const char* newTime = time[1].c_str();

      RtcDateTime currentTime = RtcDateTime(newDate, newTime); // ALREADY HAS THE CURRENT DATE AND TIME !!
      rtc.SetDateTime(currentTime);

      // Serial.print(currentTime.Hour());
      // Serial.println(currentTime.Minute());

      // Serial.println(time[1]);
      // Serial.println(time[2]);
    }

    // Show Current Time

    if (c == "devicetime") {
      RtcDateTime timeNow = rtc.GetDateTime();
      SerialBT.print(timeNow.Hour());
      SerialBT.print(",");
      SerialBT.print(timeNow.Minute());

      // Serial.print(timeNow.Hour());
      // Serial.print(",");
      // Serial.print(timeNow.Minute());
    }

    // Set Alarm Time
    for (int i = 1; i <= size; i++){
      if( parts[0] == format("Alarm {0}", i) and parts[1] == "s" ) {
        alarms[i-1].startTime[0] = atoi(parts[2].c_str());
        alarms[i-1].startTime[1] =  atoi(parts[3].c_str());

        // EEPROM.write(4 + 5*(i-1), i);
        EEPROM.write(4 + 5*(i-1), alarms[i-1].startTime[0]);
        EEPROM.write(5 + 5*(i-1), alarms[i-1].startTime[1]);
        
        // Serial.print("Start "); Serial.print(EEPROM.read(4 + 5*(i-1))); Serial.print(":"); Serial.print(EEPROM.read(5 + 5*(i-1))); 

        // Serial.println(alarms[i-1].startTime[0]);
      } else if ( parts[0] == format("Alarm {0}", i) and parts[1] == "e" ) {
        alarms[i-1].endTime[0] = atoi(parts[2].c_str());
        alarms[i-1].endTime[1] =  atoi(parts[3].c_str());
        // EEPROM.write(4, i);
        EEPROM.write(6 + 5*(i-1), alarms[i-1].endTime[0]);
        EEPROM.write(7 + 5*(i-1), alarms[i-1].endTime[1]);

        // Serial.print("End "); Serial.print(alarms[i-1].endTime[0]); Serial.print(":"); Serial.print(alarms[i-1].endTime[1]);
      }
    }

    // for (int i = 0; i < EEPROM.length(); i++)
    // {Serial.println(EEPROM.read(i));} //Serial.print("     "); Serial.println(EEPROM.read(i) == 0);} //Serial.println(getDataType(EEPROM.readShort(i)));}

    Serial.println("+++++++++++");

    EEPROM.commit();
  }
}

// void setAlarmsDefault()
// {
//   Alarms tempAlarms;
//   memcpy(&alarms, &tempAlarms, sizeof alarms);
// }

// void saveAlarmsToEEPROM()
// {
//     // for (int i = 0; i < size; i++)
//     // {
//     //     int address = i * sizeof(Alarms); // Offset calculation
//     //     EEPROM.put(address, alarms[i]);
//     // }

//     EEPROM.put(0, alarms[0]);


//     EEPROM.commit();
//     // Serial.println("Users saved!");
// }

// void loadAlarmsFromEEPROM()
// {
//     // for (int i = 0; i < size; i++)
//     // {
//     //   EEPROM.get(i * sizeof(Alarms), alarms[i]);

//     //   // Check if EEPROM contains uninitialized values (255)
//     //   // if (alarms[i].startTime[0] == 255)
//     //   // {
//     //   //     Serial.println("EEPROM Uninitialized: Setting Defaults");
//     //   //     saveAlarmsToEEPROM();
//     //   //     break;
//     //   // }
//     // }

//     EEPROM.get(0, alarms[0]);

//     // if (settings.settingCheckValue != ALARM_CHKVAL){setAlarmsDefault();}
// }













