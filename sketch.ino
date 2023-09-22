#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>

//OLED Display parameters
#define OLED_RESET -1
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C

//Parameters for NTP
#define NTP_SERVER     "pool.ntp.org"
#define UTC_OFFSET_DST 0

//Pin definitions
#define LED_1 15
#define LED_2 2
#define PB_CANCEL 34
#define PB_UP 35
#define PB_DOWN 32
#define PB_OK 33
#define BUZZER 18
#define DHT 12

//Object declarations
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHTesp dhtSensor;

//Variables to configure time zone
int offset_hours = 0, offset_minutes = 0;
unsigned long utc_offset = 0;

//Variables to keep time
int seconds = 0, minutes = 0, hours = 0, days = 0;
//unsigned long timeNow = 0, timeLast = 0; //Used when using keeping time by counting seconds

//Variables to ring buzzer
int n_notes = 8;
int C = 262;
int D = 294;
int E = 330;
int F = 349;
int G = 392;
int A = 440;
int B = 494;
int C1 = 523;
int alarmTone[] = {C,D,E,F,G,A,B,C1};
int startTone[] = {B,A,B,A,G};

//Variables for menu
int current_mode = 0;
int n_modes = 5;
String menu[] = {"1 - Set Time zone", "2 - Set Alarm 1", "3 - Set Alarm 2", "4 - Set Alarm 3", "5 - Remove Alarms"};

//Variables for alarms
bool alarms_enabled = true;
int n_alarms = 3;
int alarm_hours[] = {0, 0, 0};
int alarm_minutes[] = {0, 0, 0};
bool alarm_triggered[] = {false, false, false};

void setup() {
  Serial.begin(9600);

  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(PB_CANCEL, INPUT);
  pinMode(PB_UP, INPUT);
  pinMode(PB_DOWN, INPUT);
  pinMode(PB_OK, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(DHT, INPUT);

  dhtSensor.setup(DHT, DHTesp::DHT22); //Set up DHT22 sensor

  //Initialize display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    for(;;); //Don't proceed, loop forever
  }  
  display.display();
  delay(2000);

  display.clearDisplay();
  print_line("Welcome to Medibox", 2, 0, 0);
  delay(3000);

  //Play the start tone using buzzer
  for (int i = 0; i < n_notes; ++i) {     
    tone(BUZZER, startTone[i]);
    delay(300);
    noTone(BUZZER);
    delay(2);
  }

  //Connect to wifi
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    display.clearDisplay();
    print_line("Connecting to WiFi", 2, 0, 0);
  }

  display.clearDisplay();
  print_line("Connected to WiFi", 2, 0, 0);

  //Configure current time over wifi
  configTime(utc_offset, UTC_OFFSET_DST, NTP_SERVER);
}

void loop() {
  update_time_with_check_alarm();

  //Go to menu if OK button pressed
  if (digitalRead(PB_OK) == LOW) {
    delay(1000);
    go_to_menu();
  }

  check_temp();
}

//Function to print given line of string in given size and position on display
void print_line(String text, int text_size, int row, int column) {
  display.setTextSize(text_size);
  display.setTextColor(SSD1306_WHITE); //Text displayed in white
  display.setCursor(column, row);
  display.println(text);
  
  display.display();
}

//Function to update current time over wifi
void update_time(void) {
  //Save current time in a tm struct over wifi
  struct tm timeinfo;
  getLocalTime(&timeinfo);

  //Update variables saving time
  char str_hours[8];
  strftime(str_hours, 8, "%H", &timeinfo);
  hours = atoi(str_hours);

  char str_minutes[8];
  strftime(str_minutes,8, "%M", &timeinfo);
  minutes = atoi(str_minutes);

  char str_seconds[8];
  strftime(str_seconds, 8, "%S", &timeinfo);
  seconds = atoi(str_seconds);

  char str_days[8];
  strftime(str_days, 8, "%d", &timeinfo);
  days = atoi(str_days);
}

//Function to update current time by counting seconds
/*void update_time_without_wifi(void) {
  timeNow = millis() / 1000;
  seconds = timeNow - timeLast;

  if (seconds >= 60) {
    minutes += 1;
    timeLast += 60;
  }

  if (minutes == 60) {
    minutes = 0;
    hours += 1;
  }

  if (hours == 24) {
    hours = 0;
    days += 1;

    // If a day passed, enable alarms again
    for (int i = 0; i < n_alarms; i++) {
      alarm_triggered[i] = false;
    }
  }
}*/

//Function to display current time in DD:HH:MM:SS format on display
void print_time_now() {
  print_line(String(days) + ":" 
          + String(hours) + ":" 
          + String(minutes) + ":" 
          + String(seconds), 2, 0, 0);
}

//Function to update current time while checking whether alarm times reached
void update_time_with_check_alarm() {
  display.clearDisplay();
  update_time();
  print_time_now();

  //Check for alarms
  if (alarms_enabled) {
    for (int i = 0; i < n_alarms; ++i) {
      if (!alarm_triggered[i] && alarm_hours[i] == hours && alarm_minutes[i] == minutes) {
        //If alarm time reached and not triggered, ring the alarm
        ring_alarm();
        alarm_triggered[i] = true;
      }
    }
  }
}

//Function to display a message, light up a LED and ring the buzzer for an alarm
void ring_alarm(void) {
  //Display a message on display
  display.clearDisplay();
  print_line("Medicine  Time", 2, 0, 0);

  //Light up LED 1
  digitalWrite(LED_1, HIGH);

  //Ring buzzer till Cancel button pressed
  while (digitalRead(PB_CANCEL) == HIGH) {
    for (int i = 0; i < n_notes; ++i) {
      if (digitalRead(PB_CANCEL) == LOW) {
        delay(200);
        break;
      }

      tone(BUZZER, alarmTone[i]);
      delay(500);
      noTone(BUZZER);
      delay(2);
    }
  }

  delay(200);
  digitalWrite(LED_1, LOW);
}

//Function to wait till a button pressed an return the pressed button
int wait_for_button_press() {
  while (true) {
    if (digitalRead(PB_UP) == LOW) {
      delay(200); //Button debounce time
      return PB_UP;
    }

    if (digitalRead(PB_DOWN) == LOW) {
      delay(200);
      return PB_DOWN;
    }

    if (digitalRead(PB_CANCEL) == LOW) {
      delay(200);
      return PB_CANCEL;
    }

    if (digitalRead(PB_OK) == LOW) {
      delay(200);
      return PB_OK;
    }

    //Update time while waiting
    update_time();
  }
}

//Function to set utc offset by getting it as input
void set_time_zone() {

  //Get hours of utc offset as input
  int temp_hours = offset_hours;
  while (true) {
    display.clearDisplay();
    print_line("Enter hours: " + String(temp_hours), 2, 0, 0);

    int button = wait_for_button_press();
    if (button == PB_OK) {
      delay(200);
      offset_hours = temp_hours;
      break;
    }

    if (button == PB_CANCEL) {
      delay(200);
      break;
    }

    if (button == PB_UP) {
      delay(200);
      temp_hours += 1;
      temp_hours %= 24;
    }

    else if (button == PB_DOWN) {
      delay(200);
      temp_hours -= 1;
      if (temp_hours < 0) {
        temp_hours += 24;
      }
    }
  }

  //Get minutes of utc offset as input
  int temp_minutes = offset_minutes;
  while (true) {
    display.clearDisplay();
    print_line("Enter minutes: " + String(temp_minutes), 2, 0, 0);

    int button = wait_for_button_press();
    if (button == PB_OK) {
      delay(200);
      offset_minutes = temp_minutes;
      break;
    }

    if (button == PB_CANCEL) {
      delay(200);
      break;
    }

    if (button == PB_UP) {
      delay(200);
      temp_minutes += 1;
      temp_minutes %= 60;
    }

    else if (button == PB_DOWN) {
      delay(200);
      temp_minutes -= 1;
      if (temp_minutes < 0) {
        temp_minutes += 60;
      }
    }
  }
  
  utc_offset = offset_hours * 60 * 60 + offset_minutes * 60; // Calculate utc offset in seconds
  configTime(utc_offset, UTC_OFFSET_DST, NTP_SERVER); //Use setted utc offset and configure current time

  display.clearDisplay();
  print_line("Time zone is set", 2, 0, 0);
  delay(1000);
}

//Function to set time by getting time as input. Used when using counting seconds method.
/*void set_time_without_wifi() {

  //Set hours
  int temp_hours = hours;
  while (true) {
    display.clearDisplay();
    print_line("Enter hour: " + String(temp_hours), 2, 0, 0);

    int button = wait_for_button_press();
    if (button == PB_OK) {
      delay(200);
      hours = temp_hours;
      break;
    }

    if (button == PB_CANCEL) {
      delay(200);
      break;
    }

    if (button == PB_UP) {
      delay(200);
      temp_hours += 1;
      temp_hours %= 24;
    }

    else if (button == PB_DOWN) {
      delay(200);
      temp_hours -= 1;
      if (temp_hours < 0) {
        temp_hours += 24;
      }
    }
  }

  //Set minutes
  int temp_minutes = minutes;
  while (true) {
    display.clearDisplay();
    print_line("Enter minute: " + String(temp_minutes), 2, 0, 0);

    int button = wait_for_button_press();
    if (button == PB_OK) {
      delay(200);
      minutes = temp_minutes;
      break;
    }

    if (button == PB_CANCEL) {
      delay(200);
      break;
    }

    if (button == PB_UP) {
      delay(200);
      temp_minutes += 1;
      temp_minutes %= 60;
    }

    else if (button == PB_DOWN) {
      delay(200);
      temp_minutes -= 1;
      if (temp_minutes < 0) {
        temp_minutes += 60;
      }
    }
  }
  
  display.clearDisplay();
  print_line("Time is set", 2, 0, 0);
  delay(1000);
}*/

//Functions to set given alarm by getting alarm time as input
void set_alarm(int alarm) {

  //Get hour as input
  int temp_hours = alarm_hours[alarm];
  while (true) {
    display.clearDisplay();
    print_line("Enter hour: " + String(temp_hours), 2, 0, 0);

    int button = wait_for_button_press();
    if (button == PB_OK) {
      delay(200);
      alarm_hours[alarm] = temp_hours;
      break;
    }

    if (button == PB_CANCEL) {
      delay(200);
      break;
    }

    if (button == PB_UP) {
      delay(200);
      temp_hours += 1;
      temp_hours %= 24;
    }

    else if (button == PB_DOWN) {
      delay(200);
      temp_hours -= 1;
      if (temp_hours < 0) {
        temp_hours += 24;
      }
    }
  }

  //Get minutes as input
  int temp_minutes = alarm_minutes[alarm];
  while (true) {
    display.clearDisplay();
    print_line("Enter minute: " + String(temp_minutes), 2, 0, 0);

    int button = wait_for_button_press();
    if (button == PB_OK) {
      delay(200);
      alarm_minutes[alarm] = temp_minutes;
      break;
    }

    if (button == PB_CANCEL) {
      delay(200);
      break;
    }

    if (button == PB_UP) {
      delay(200);
      temp_minutes += 1;
      temp_minutes %= 60;
    }

    else if (button == PB_DOWN) {
      delay(200);
      temp_minutes -= 1;
      if (temp_minutes < 0) {
        temp_minutes += 60;
      }
    }
  }
  
  display.clearDisplay();
  print_line("Alarm is set", 2, 0, 0);
  delay(1000);
}

//Function to get current temperature and humidity and check whether they are in healthy limits
void check_temp(void) {
  TempAndHumidity data = dhtSensor.getTempAndHumidity(); //Read temperature and humidity from sensor
  bool all_good = true;

  //Give warnings if healthy limits exceeded
  if (data.temperature > 32) {
    all_good = false;
    digitalWrite(LED_2, HIGH);
    print_line("Temperature High", 1, 40, 0);
  }

  else if (data.temperature < 26) {
    all_good = false;
    digitalWrite(LED_2, HIGH);
    print_line("Temperature Low", 1, 40, 0);
  }

  if (data.humidity > 80) {
    all_good = false;
    digitalWrite(LED_2, HIGH);
    print_line("Humidity High", 1, 50, 0);
  }

  if (data.humidity < 60) {
    all_good = false;
    digitalWrite(LED_2, HIGH);
    print_line("Humidity Low", 1, 50, 0);
  }

  if (all_good) {
    digitalWrite(LED_2, LOW);
  }
  else {
      tone(BUZZER, G);
      delay(600);
      noTone(BUZZER);
      delay(2);
  }
}

//Function to navigate through the menu
void go_to_menu(void) {
  while (true) {
    display.clearDisplay();
    print_line(menu[current_mode], 2, 0, 0);
    int button = wait_for_button_press();
    
    //Navigate through menu based on buttons pressed
    if (button == PB_OK) {
      delay(200);
      run_current_mode(); //current_mode not passed to function. This function reads current mode from global variable
    }
    
    if (button == PB_CANCEL) {
      delay(200);
      break;
    }

    if (button == PB_UP) {
      delay(200);
      current_mode += 1;
      current_mode %= n_modes;
    }

    else if (button == PB_DOWN) {
      delay(200);
      current_mode -= 1;
      if (current_mode < 0) {
        current_mode += n_modes;
      }
    }
  }
}

//Function to execute the functions related to current mode
void run_current_mode() {
  if (current_mode == 0) {
    set_time_zone();
  }
  else if (current_mode == 1 || current_mode == 2 || current_mode == 3) {
    set_alarm(current_mode - 1);
  }
  else if (current_mode == 4) {
    alarms_enabled = false;
  }
}
