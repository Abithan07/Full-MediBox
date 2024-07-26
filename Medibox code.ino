#include <WiFi.h>
#include "DHTesp.h"
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneButton.h>
#include <ESP32Servo.h>


// Define constants for the OLED screen
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C


#define LED_1 4
#define LED_2 2
#define UP 33
#define DOWN 35
#define DHT_PIN 15
#define BUZZER 5

#define LDR_L A0
#define LDR_R A3
#define SERVMO 18


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

DHTesp dht_sensor;


//time initialization
#define NTP_SERVER     "pool.ntp.org"
int UTC_OFFSET = 5*60*60 + 30*60;;
#define UTC_OFFSET_DST 0
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

//Wifi and mqtt clients
WiFiClient espClient;
PubSubClient mqttClient(espClient);


//Parameters related to light intensity
float GAMMA = 0.75;
const float RL10 = 50;
float MIN_ANGLE = 30;
float lux_L = 0;
float lux_R = 0;
int D = 0;

float Max_lux;
String Max_from;

int days = 0;
int month = 0;
int year = 0;

int hours = 0;
int minutes = 0;
int seconds = 0;

bool alarm_enabled = true;
bool Stop_Alarm = true;
int num_alarms = 3;
int alarm_hours[] = {20,1,0};
int alarm_minutes[] = {42,5,10};
bool alarm_triggered[] = {false, false, false};
int current_mode = 0;
int max_modes = 5;
String options[] = {"1 - Set Time", "2 - Set Alarm 1", "3 - Set Alarm 2","4 - Set Alarm 3", "5 - Remove Alarm"};

OneButton U_btn(UP, true);
OneButton D_btn(DOWN, true);

// Global variable to track button press status
int buttonStatus = 0;


char tempAr[6];
char humAr[6];
char lightAr[6];
char sideAr[3];
char msgAr[5];

int num_notes = 8;
int notes[] = {220, 294, 330, 349, 440, 494, 450, 523};

//servo moter initializing position
int pos = 0;
Servo servo;

//initiating DHT sensor
DHTesp dhtSensor;

void setup() {
  // put your setup code here, to run once:
  // Set up the pins
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(UP, INPUT_PULLUP);
  pinMode(DOWN, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);
  pinMode(LDR_L, INPUT);
  pinMode(LDR_R, INPUT);

  digitalWrite(BUZZER, LOW);

  servo.attach(SERVMO, 500, 2400);


  Serial.begin(115200);

  setupWiFi();
  setupMqtt();

  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);

  // Set up the OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("ERROR"));
    for (;;);
  }

  // Wait for the OLED display to initialize
  display.display();
  delay(2000);

  // Clear the OLED display and display a welcome message
  display.clearDisplay();
  print_line("Welcome to Medibox!",2,0,2);
  delay(3000);

  timeClient.begin();
  timeClient.setTimeOffset(5.5*3600);
  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);
}

void loop() {
  // put your main code here, to run repeatedly:

  update_time_with_check_alarm();
  
  updateTemperature();
  Serial.println(tempAr);

  read_Max_Lux();

  if(digitalRead(UP) == LOW){
    delay(1000);
    Serial.println("MENU");
    go_to_menu();
  }



  if(!mqttClient.connected()){
    connectToBroker();
  }
  mqttClient.loop();

  mqttClient.publish("mqtt-TEMP", tempAr);
  mqttClient.publish("mqtt-HUM", humAr);
  mqttClient.publish("mqtt-LIGHT-INTENSITY", lightAr);
  mqttClient.publish("mqtt-MAX-INTENS-SIDE", sideAr);
  delay(500);

}


void connectToBroker(){
  while (!mqttClient.connected()){
    Serial.println("Establishing MQTT connection...");
    if(mqttClient.connect("ESP32-12345645454")){
      Serial.println("MQTT Connection Established");
      //all the subscriptions goes here
      mqttClient.subscribe("mqtt-ANGLE");
      mqttClient.subscribe("mqtt-CONT-FACTOR");
    }else{
      Serial.println("failed ");
      Serial.println(mqttClient.state());
      delay(5000);
    }
  }
}


void setupMqtt(){
  mqttClient.setServer("test.mosquitto.org", 1883);
  mqttClient.setCallback(receiveCallback);
}

void receiveCallback(char* topic, byte* payload, unsigned int length){
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]");

  char payloadCharAr[length];
  for(int i = 0; i < length; i++){
    Serial.print((char)payload[i]);
    payloadCharAr[i] = (char)payload[i];
  }

  Serial.println();

  //receive minimum angle 
  if (strcmp(topic, "mqtt-ANGLE") == 0){
    MIN_ANGLE = atoi(payloadCharAr);
  }
  //receive control factor
  if (strcmp(topic, "mqtt-CONT-FACTOR") == 0) {
    GAMMA = atof(payloadCharAr);
  }

  //receive alarm stop signal
  if (strcmp(topic, "mqtt-ALARM") == 0) {
    if (payloadCharAr[0] == 'O'){
      Stop_Alarm = false ;
    }
  }
}



void updateTemperature(){
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  String(data.temperature, 2).toCharArray(tempAr, 6);
  String(data.humidity, 2).toCharArray(humAr, 6);
}


void setupWiFi(){
  Serial.println();
  Serial.print("connecting to ");
  Serial.println("Wokwi-GUEST");
  WiFi.begin("Wokwi-GUEST", "", 6);

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi Connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}


unsigned long getTime(){
  timeClient.update();
  return timeClient.getEpochTime();
}


float read_ldr(int LDR) {
  int analogValue = analogRead(LDR);

  //calculate light intensity in 0-1 range
  float voltage = analogValue / 1024. * 5;
  float resistance = 2000 * voltage / (1 - voltage / 5);
  float maxlux = pow(RL10 * 1e3 * pow(10, GAMMA) / 322.58, (1 / GAMMA));
  float lux = pow(RL10 * 1e3 * pow(10, GAMMA) / resistance, (1 / GAMMA))/maxlux;
  //String(lux).toCharArray(lightAr,6);
  //Serial.println(lux);
  return(lux);
}

void read_Max_Lux(){
  lux_L = read_ldr(LDR_L);
  lux_R = read_ldr(LDR_R);


  if (lux_L > lux_R) {
    Max_lux = lux_L;
    Max_from = "Le"; // Track that the max value is from LDR_L
    D = 1.5;
  } else {
    Max_lux = lux_R;
    Max_from = "Rg"; // Track that the max value is from LDR_R
    D = 0.5;
  }

  serv_mo();
  String(Max_lux).toCharArray(lightAr,6);
  String(Max_from).toCharArray(sideAr,3);
}

void serv_mo(){
  //calculating position of servo motor
  pos = min(int(D*MIN_ANGLE + (180 - MIN_ANGLE) * Max_lux * GAMMA), 180);
  servo.write(pos);
}
//===============================================================

void update_time_over_wifi() {
  struct tm timeinfo;
  getLocalTime(&timeinfo);

  char timeHour[3];
  strftime(timeHour, 3, "%H", &timeinfo);
  hours = atoi(timeHour);

  char timeMinute[3];
  strftime(timeMinute, 3, "%M", &timeinfo);
  minutes = atoi(timeMinute);

  char timeSecond[3];
  strftime(timeSecond, 3, "%S", &timeinfo);
  seconds = atoi(timeSecond);

  char timeDay[3];
  strftime(timeDay, 3, "%d", &timeinfo);
  days = atoi(timeDay);

  char timeMonth[3];
  strftime(timeMonth, 3, "%m", &timeinfo);
  month = atoi(timeMonth);

  char timeYear[5]; // Year needs at least 4 characters (for example, "2024")
  strftime(timeYear, 5, "%Y", &timeinfo);
  year = atoi(timeYear);
}

void print_time_now(){
  display.clearDisplay();
  print_line(String(days), 1, 0, 0);
  print_line("/", 1, 0, 10);
  print_line(String(month), 1, 0, 20);
  print_line("/", 1, 0, 30);
  print_line(String(year), 1, 0, 40);
 
  print_line(String(hours), 2, 10, 0);
  print_line(":", 2, 10, 20);
  print_line(String(minutes), 2, 10, 30);
  print_line(":", 2, 10, 50);
  print_line(String(seconds), 2, 10, 60);
}

// Function to print a message on a given row and column of the display
void print_line(String message,int text_size, int row, int column){
  display.setTextSize(text_size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column, row);
  display.println(message);
  display.display();
}

// Function to update time and check if alarm needs to be triggered
void update_time_with_check_alarm(){
  display.clearDisplay(); // Clear the display before updating the time
  update_time_over_wifi(); // Update the time over WiFi
  print_time_now(); // Print the updated time on the display

  // Check if alarm is enabled and if it's time to trigger the alarm
  if(alarm_enabled){
    for (int i = 0; i < num_alarms; i++){
      if(alarm_triggered[i] == false && hours == alarm_hours[i] && minutes == alarm_minutes[i]){
        ring_alarm(); // Trigger the alarm
        alarm_triggered[i] = true;
      }
    }
  }
}

// Function to trigger the alarm and play a tone until cancelled
void ring_alarm(){
  display.clearDisplay(); // Clear the display
  print_line("Medicine Time", 2, 0, 0); // Print the title on the display

  digitalWrite(LED_1, HIGH); // Turn on LED 1

  Stop_Alarm == true;
  String("Push").toCharArray(msgAr,5);
  mqttClient.publish("mqtt-ALARM-TRIGGERED", msgAr);

  bool break_happened = false;

  // Loop until alarm is cancelled
  while(digitalRead(DOWN) == HIGH){
    for( int i = 0; i < num_notes; i++){
      if(digitalRead(DOWN) == LOW || Stop_Alarm == false){
        delay(200);
        break_happened = true;
        break;
      }
      tone(BUZZER, notes[i]);
      delay(500);
      noTone(BUZZER);
      delay(2);
    }
  }

  delay(200);
  digitalWrite(LED_1,  LOW); // Turn off LED 1

}

void U_handleClick() {
  //Serial.println("Clicked!");
  buttonStatus = 1;
}

void U_handleLongPressStop() {
  //Serial.println("Long Pressed!");
  buttonStatus = 2;
}

void D_handleClick() {
  //Serial.println("Clicked!");
  buttonStatus = 3;
}

void D_handleLongPressStop() {
  //Serial.println("Long Pressed!");
  buttonStatus = 4;
}

// Function to wait for a button press and return the button pressed
int wait_for_button_press(){

  buttonStatus = 0;

  while(true){

  U_btn.tick();
  D_btn.tick();

  U_btn.attachLongPressStop(U_handleLongPressStop);
  U_btn.attachClick(U_handleClick);
  D_btn.attachLongPressStop(D_handleLongPressStop);
  D_btn.attachClick(D_handleClick);

  
    if (buttonStatus == 1){
      delay(200);
      return UP;
    }

    else if (buttonStatus == 2){
              delay(200);
              return OK;
            }

    else if (buttonStatus == 3){
      delay(200);
      return DOWN;
    }
    
    else if (buttonStatus == 4){
      delay(200);
      return CANCEL;
    }
  }
}


// function to navigate through the menu
void go_to_menu(void) { 
  while (true) {
    display.clearDisplay(); 
    print_line(options [current_mode], 2, 0, 0);

    int pressed = wait_for_button_press();

    if (pressed == UP) {
      current_mode += 1;
      current_mode %= max_modes; 
      delay(200);
    }

    else if (pressed == DOWN) { 
      current_mode -= 1;
      if (current_mode < 0) { 
        current_mode = max_modes - 1;
      } 
      delay(200);
    } 

    //Entering into current mode if OK is pressed
    else if (pressed == OK) {
      Serial.println(current_mode); 
      delay(200);
      run_mode(current_mode);
    }

    //Exiting Menu if CANCEL is pressed
  else if (pressed == CANCEL) {
      delay(200);
      break;
    }

  }
}

// The UTC offset is calculated from the user's inputs

void set_time() {
  int temp_UTC = 0;
  int temp_hour = 0;

  // Prompt user to enter hour
  while (true){
    display.clearDisplay();
    print_line("Enter hour: " + String(temp_hour) , 2, 0, 2);

    

    // Wait for button press
    int pressed = wait_for_button_press();

    // Adjust hour value based on button press
    if (pressed == UP){
      delay(200);
      temp_hour += 1;
      if (temp_hour > 24){
        temp_hour = 0;
      }
    }
    else if (pressed == DOWN){
      delay(200);
      temp_hour -= 1;
      if (temp_hour < 0){
        temp_hour = 24;
      }   
    }
    // Save UTC offset and exit loop on OK button press
    else if (pressed == OK){
      delay(200);
      temp_UTC = temp_hour*3600;
      break;
    }
    // Exit loop on CANCEL button press
    else if (pressed == CANCEL){
      delay(200);
      break;
    }

  }

  int temp_minute = 0;

  // Prompt user to enter minute
  while (true){
    display.clearDisplay();
    print_line("Enter minute: " + String(temp_minute) , 2, 0, 2);



    // Wait for button press
    int pressed = wait_for_button_press();

    // Adjust minute value based on button press
    if (pressed == UP){
      delay(200);
      temp_minute += 1;
      if (temp_minute > 59){
        temp_minute = 0;
      }
    }
    else if (pressed == DOWN){
      delay(200);
      temp_minute -= 1;
      if (temp_minute < 0){
        temp_minute = 59;
      }   
    }
    // Calculate and save UTC offset and exit loop on OK button press
    else if (pressed == OK){
      delay(200);
      if (temp_minute < 0){
        temp_minute = temp_minute*(-1);
      }
      temp_UTC += (temp_minute*60);
      UTC_OFFSET = temp_UTC;
      break;
    }
    // Exit loop on CANCEL button press
    else if (pressed == CANCEL){
      delay(200);
      break;
    }

  }

  // Display confirmation message
  display.clearDisplay();
  print_line("Time is set", 2, 0, 2);
  delay(1000);
}

void set_alarm(int alarm){

  // Get the current hour for the selected alarm
  int temp_hour = alarm_hours[alarm];

  // Loop until the user sets the alarm hour or cancels the operation
  while (true){
    // Clear the display and show the current hour
    display.clearDisplay();
    print_line("Enter hour: " + String(temp_hour) , 2, 0, 2);

    // Wait for a button press
    int pressed = wait_for_button_press();

    // If the UP button is pressed, increment the hour
    if (pressed == UP){
      delay(200);
      temp_hour += 1;
      temp_hour = temp_hour % 24;
    }

    // If the DOWN button is pressed, decrement the hour
    else if (pressed == DOWN){
      delay(200);
      temp_hour -= 1;
      if (temp_hour < 0){
        temp_hour = 23;
      }
    }

    // If the OK button is pressed, update the alarm hour and break the loop
    else if (pressed == OK){
      delay(200);
      alarm_hours[alarm]  = temp_hour;
      break;
    }

    // If the CANCEL button is pressed, break the loop and cancel the operation
    else if (pressed == CANCEL){
      delay(200);
      break;
    }
  }

  // Get the current minute for the selected alarm
  int temp_minute = alarm_minutes[alarm];

  // Loop until the user sets the alarm minute or cancels the operation
  while (true){
    // Clear the display and show the current minute
    display.clearDisplay();
    print_line("Enter minute: " + String(temp_minute) , 2, 0, 2);

    // Wait for a button press
    int pressed = wait_for_button_press();

    // If the UP button is pressed, increment the minute
    if (pressed == UP){
      delay(200);
      temp_minute += 1;
      temp_minute = temp_minute % 60;
    }

    // If the DOWN button is pressed, decrement the minute
    else if (pressed == DOWN){
      delay(200);
      temp_minute -= 1;
      if (temp_minute < 0){
        temp_minute = 59;
      }
    }

    // If the OK button is pressed, update the alarm minute and break the loop
    else if (pressed == OK){
      delay(200);
      alarm_minutes[alarm]  = temp_minute;
      break;
    }

    // If the CANCEL button is pressed, break the loop and cancel the operation
    else if (pressed == CANCEL){
      delay(200);
      break;
    }
  }

  //enabling all alarms
  alarm_enabled = true;

  // Show a confirmation message
  display.clearDisplay();
  print_line("Alarm is set", 2, 0, 2);
  delay(1000);

}


// This function allows the user to set the time or an alarm depending on the mode selected.
void run_mode(int mode){
  // If the mode is 0, set_time() function is called to set the current time.
  if (mode == 0){
    set_time();
  }
  // If the mode is between 1 and 3 (inclusive), set_alarm() function is called to set the corresponding alarm.
  else if (mode == 1 || mode == 2 || mode == 3){
    set_alarm(mode-1);
  }
  // If the mode is 4, the alarm_enabled flag is set to false, disabling all alarms.
  else if (mode == 4){
    alarm_enabled = false;
    display.clearDisplay();
    print_line("Alarms are Disabled", 2, 0, 2);
    delay(1000);
  }
}
