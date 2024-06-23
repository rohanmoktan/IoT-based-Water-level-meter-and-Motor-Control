
 /**********************************************************************************/
 
/* Fill-in your Template ID (Required to use Blynk.Cloud) */
#define BLYNK_TEMPLATE_ID "TMPL3nbo3nAFe"
#define BLYNK_TEMPLATE_NAME "ISD2 lab project"
#define BLYNK_AUTH_TOKEN "EQXNobJWXjFioBziHUxCrfGQzUD-ZRfb"

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "Galaxy F2253E8";
char pass[] = "ojbe7471";

//Set Water Level Distance in CM
int emptyTankDistance = 70 ;  //Distance when tank is empty
int fullTankDistance =  10 ;  //Distance when tank is full

//Set trigger value in percentage
int triggerPer =   10 ;  //alarm will start when water level drop below triggerPer

#include <Adafruit_SSD1306.h> //for handling Oled display
#include <WiFi.h> //for wifi handling 
#include <WiFiClient.h> //for wifi handling
#include <BlynkSimpleEsp32.h> //for using blynk server
#include <AceButton.h> //for using external button
using namespace ace_button; 

// Define connections to sensor
#define TRIGPIN    27  //D27
#define ECHOPIN    26  //D26
#define wifiLed    2   //D2
#define ButtonPin1 12  //D12
#define BuzzerPin  13  //D13
#define GreenLed   14  //D14

//Change the virtual pins according the rooms
#define VPIN_BUTTON_1    V1 
#define VPIN_BUTTON_2    V2
#define VPIN_BUTTON_3    V3
#define VPIN_BUTTON_4    V4
#define VPIN_BUTTON_5    V5

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); //Wire is a library used for I2C communication


float duration;
float distance;
float d1,d2,speed; //for speed measurement
int   waterLevelPer;
bool  toggleBuzzer = HIGH; //Define to remember the toggle state
bool motorON;
bool Calib=0;
int buttonClicked = 2; //starting with 2%3 =2 (calling Calibrate when button clicked first time) 

char auth[] = BLYNK_AUTH_TOKEN;

ButtonConfig config1;
AceButton button1(&config1); 

void handleEvent1(AceButton*, uint8_t, uint8_t);

BlynkTimer timer; //blynk timer (object) for initialising to send the data every 2 seconds from H/w to Blynk server

void checkBlynkStatus() { // called every 3 seconds by SimpleTimer

  bool isconnected = Blynk.connected();
  if (isconnected == false) {
    //Serial.println("Blynk Not Connected");
    digitalWrite(wifiLed, LOW);
  }
  if (isconnected == true) {
    digitalWrite(wifiLed, HIGH);
    //Serial.println("Blynk Connected");
  }
}

BLYNK_CONNECTED() { //connecting the blynk virtual pin
  Blynk.syncVirtual(VPIN_BUTTON_1);
  Blynk.syncVirtual(VPIN_BUTTON_2);
  Blynk.syncVirtual(VPIN_BUTTON_3);
  Blynk.syncVirtual(VPIN_BUTTON_4);
  Blynk.syncVirtual(VPIN_BUTTON_5);
}

void displayData(int value){
  display.clearDisplay();
  display.setTextSize(4);
  display.setCursor(8,2);
  display.print(value);
  display.print(" ");
  display.print("%");
  display.display();
}

void displayDistance(int value){
  display.clearDisplay();
  //displaying mode & which calibration
  display.setTextSize(1);
  display.setCursor(0,0);
  if(buttonClicked%3==0){
    display.print("Mode: Calib (EMPTY)");
  }else if(buttonClicked%3==1){
    display.print("Mode: Calib (FULL)");
  }
  display.setTextSize(3);
  display.setCursor(8,12);
  display.print(value);
  display.print("");
  display.print("cm");
  display.display();
}


float measureDistance(){
  // Set the trigger pin LOW for 2uS
  digitalWrite(TRIGPIN, LOW);
  delayMicroseconds(2);
 
  // Set the trigger pin HIGH for 20us to send pulse
  digitalWrite(TRIGPIN, HIGH);
  delayMicroseconds(20);
 
  // Return the trigger pin to LOW
  digitalWrite(TRIGPIN, LOW);
 
  // Measure the width of the incoming pulse
  duration = pulseIn(ECHOPIN, HIGH);
 
  // Determine distance from duration
  // Use 343 metres per second as speed of sound
  // Divide by 1000 as we want millimeters
 
  distance = ((duration / 2) * 0.343)/10; // divide by ten for millimiters to centimeters

  if (distance > (fullTankDistance - 10)  && distance < emptyTankDistance ){
    waterLevelPer = map((int)distance ,emptyTankDistance, fullTankDistance, 0, 100);
    displayData(waterLevelPer);
    Blynk.virtualWrite(VPIN_BUTTON_1, waterLevelPer);
    Blynk.virtualWrite(VPIN_BUTTON_2, (String(distance) + " cm"));

    // Print result to serial monitor
    // Serial.print("Distance: ");
    // Serial.print(distance);
    // Serial.println(" cm");

    if (waterLevelPer < triggerPer+5){ //at triggerper+5 (i.e. 15% turn ON motor)
      digitalWrite(GreenLed, HIGH);
      motorON = 1;     
    }
    if(waterLevelPer<triggerPer){//turning ON buzzer when waterLevelPer<triggerPer i.e. 10
        if (toggleBuzzer == HIGH){
        digitalWrite(BuzzerPin, HIGH);
      } 
    }
    if(waterLevelPer >= 95){ //motor off before full tank achieved (i.e. at 95% turn of motor)
      digitalWrite(GreenLed, LOW);
      motorON = 0;
    }
    if (waterLevelPer>=99){ //overflow when more than equal to 99%, start buzzer
      if (toggleBuzzer == HIGH){
        digitalWrite(BuzzerPin, HIGH);
      } 
    }

    if (waterLevelPer>10 && waterLevelPer<=99){ //making sure buzzer turnedOFF in safe case
      toggleBuzzer = HIGH;
      digitalWrite(BuzzerPin, LOW);
    }        
  }
  
  // Delay before repeating measurement
  delay(100);
  
  return distance; //returning distance for speed measurment purpose
}

float measureCalibDistance(){
  // Set the trigger pin LOW for 2uS
  digitalWrite(TRIGPIN, LOW);
  delayMicroseconds(2);
 
  // Set the trigger pin HIGH for 20us to send pulse
  digitalWrite(TRIGPIN, HIGH);
  delayMicroseconds(20);
 
  // Return the trigger pin to LOW
  digitalWrite(TRIGPIN, LOW);
 
  // Measure the width of the incoming pulse
  duration = pulseIn(ECHOPIN, HIGH);
 
  // Determine distance from duration
  // Use 343 metres per second as speed of sound
  // Divide by 1000 as we want millimeters
 
  distance = ((duration / 2) * 0.343)/10; // divide by ten for millimiters to centimeters
  
  return distance; //returning distance for speed measurment purpose
}

void Calibrate(){
  Serial.print("Calibrate CAlled");
  while(buttonClicked%3<=1){
    distance = measureCalibDistance(); //measuring distance
    int dist = (int) distance;
    displayDistance(dist); //displaying current distance in OLED
    if(dist<5)digitalWrite(BuzzerPin, HIGH);//for alerting setting full tank distance low
    else digitalWrite(BuzzerPin, LOW);
    button1.check();
  }
}

void grp_members(){
  display.clearDisplay();
  //displaying mode & which calibration
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("Group 3:");
  display.setCursor(0,8);
  display.print("Rohan");
  display.setCursor(0,16);
  display.print("Utkarsh");
  display.setCursor(0,24);
  display.print("Anurag");
  display.display();
}

// 'iit kgp logo', 28x32px
const unsigned char iit_kgp_logo_array [] PROGMEM = {
	0xff, 0xd9, 0xff, 0xf0, 0xff, 0x80, 0x9f, 0xf0, 0xfd, 0x80, 0x1b, 0xf0, 0xf8, 0x00, 0x01, 0xf0, 
	0xfc, 0x00, 0x01, 0xf0, 0xe0, 0x1b, 0xc1, 0x70, 0xc0, 0xb6, 0x70, 0x30, 0xe0, 0x7d, 0x98, 0x70, 
	0xe3, 0xf8, 0xec, 0x70, 0x83, 0xf8, 0xfc, 0x10, 0x85, 0x87, 0x0a, 0x00, 0xc3, 0xc0, 0x9e, 0x30, 
	0x87, 0xf9, 0x7e, 0x10, 0x07, 0xc0, 0x1e, 0x00, 0x03, 0x80, 0x0c, 0x00, 0x87, 0xc8, 0xbe, 0x10, 
	0xc7, 0xc9, 0x3e, 0x30, 0x87, 0x39, 0xce, 0x00, 0x07, 0x79, 0xee, 0x00, 0x82, 0x79, 0xe4, 0x30, 
	0xe0, 0x78, 0xf0, 0x70, 0xc0, 0xf0, 0xf0, 0x30, 0xc0, 0xf0, 0xf8, 0x30, 0xfc, 0xfd, 0xf9, 0xf0, 
	0xf9, 0xfa, 0xf9, 0xf0, 0xf8, 0x07, 0x81, 0xf0, 0xf0, 0x00, 0x00, 0xf0, 0xf3, 0xf0, 0x7c, 0x70, 
	0xff, 0xf8, 0xff, 0xf0, 0xf0, 0x00, 0x00, 0x70, 0xe0, 0x00, 0x00, 0x30, 0xc0, 0x00, 0x00, 0x30
};

void lab_with_logo(){
  display.clearDisplay();
  //displaying mode & which calibration
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("ISD Prof.Prajit Nandi");
  display.drawBitmap(50,12,iit_kgp_logo_array,28,32,1); //53X60 is size and 1 means color, (o,3) means starting position of bitmap
  display.display();
}

 
void setup() {
  // Set up serial monitor
  Serial.begin(115200);
 
  // Set pinmodes for sensor connections
  pinMode(ECHOPIN, INPUT); //defining I/O ports
  pinMode(TRIGPIN, OUTPUT);
  pinMode(wifiLed, OUTPUT);
  pinMode(GreenLed, OUTPUT);
  pinMode(BuzzerPin, OUTPUT);

  pinMode(ButtonPin1, INPUT_PULLUP);

  digitalWrite(wifiLed, LOW); //Output value what
  digitalWrite(GreenLed, LOW);
  digitalWrite(BuzzerPin, LOW);

  config1.setEventHandler(button1Handler);
  
  button1.init(ButtonPin1);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(1000);  
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.clearDisplay();

  WiFi.begin(ssid, pass); //connecting to wifi (constantly trying to maintian wifi connection) 
  timer.setInterval(2000L, checkBlynkStatus); // check if Blynk server is connected every 2 seconds
  Blynk.config(auth); //authentication needed to connect to blynk server containing our specific project
  delay(1000);
  
  //displaying lab name & group members name
  lab_with_logo();
  delay(3000);
  grp_members();
  delay(3000);
}
 void loop() {

  button1.check(); //checking button pressed event
  d1= measureDistance();
  button1.check(); //checking button pressed event
  d2 = measureDistance();
  button1.check(); //checking button pressed event
  speed = (d2-d1)*10; //speed in cm/s, since 0.1 ms delay done in measureDistance() function
  button1.check(); //checking button pressed event
  speed=abs(speed);
  button1.check(); //checking button pressed event
  if(motorON==1)Blynk.virtualWrite(VPIN_BUTTON_4, (String(speed) + " cm/sec")); //sending speed details to blynk server, only motor on
  else Blynk.virtualWrite(VPIN_BUTTON_4, ("Motor is OFF!"));
  button1.check(); //checking button pressed event
  Blynk.virtualWrite(VPIN_BUTTON_3, fullTankDistance);
  button1.check(); //checking button pressed event
  Blynk.virtualWrite(VPIN_BUTTON_5, emptyTankDistance);
  button1.check(); //checking button pressed event
  Blynk.run();
  button1.check(); //checking button pressed event
  timer.run(); // Initiates SimpleTimer
  button1.check(); //checking button pressed event
 
}

void button1Handler(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  Serial.println("EVENT1");
  switch (eventType) {
    case AceButton::kEventReleased:
      Serial.println("kEventReleased"); //for checking in terminal
      //if button pressed & released
      //start calibration mode
      if(buttonClicked%3==2){ //first start
        Calib=1;
        buttonClicked++; //changin buttonClicked to 3, i.e. 3%3 =0
        Calibrate();
        Calib=0;
      }
      if(Calib==1){
        if(buttonClicked%3==0){ //first setting empty tank distance
        emptyTankDistance = (int)distance; //current distance update if in calibration mode
        Blynk.virtualWrite(VPIN_BUTTON_5, emptyTankDistance);//updating full tank distance in blynk server
        buttonClicked++;
        }else if(buttonClicked%3==1){ //then setting full tank distance
          fullTankDistance = (int)distance;
          Blynk.virtualWrite(VPIN_BUTTON_3, fullTankDistance);//updating full tank distance in blynk server
          buttonClicked++;
        }
      }
      break;
  }
}
