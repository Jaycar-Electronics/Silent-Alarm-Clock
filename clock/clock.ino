
#include <Wire.h>
#include <RTClib.h> //version 1.0.3
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

// if you want to debug the timing, you can enable the TIMELAPSE; and it will rush through it:
//#define TIMELAPSE

// constants:
const int pin_sensor = 2;
const int pin_buzzer = 3;
const int pin_shield = 13;
const long clap_delay_ms = 3000L;    // about 3 seconds between claps to activate
const long clap_debounce_ms = 10L; // a clap is about 10 milliseconds long

// time constants, at what time do the effects take place?
const int morningTime = 6; //morning
const int afternoonTime = 16;
const int nightTime = 20;

const short w = 5;
const short h = 8;

// volatile variables to be handled in the interrupt.
volatile unsigned long firstClapTime = 0;
volatile bool hasClapped = false;

// Module objects
Adafruit_NeoMatrix shield(w, h, pin_shield,
                          NEO_MATRIX_TOP | NEO_MATRIX_RIGHT | NEO_MATRIX_COLUMNS | NEO_MATRIX_PROGRESSIVE,
                          NEO_GRB | NEO_KHZ800);

RTC_DS1307 rtc;

char timeBuffer[] = "real time: hh:mm";


double hoursAsDecimal = 0;

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  shield.begin();
  shield.setBrightness(20);
  shield.setTextWrap(false);
  shield.setTextColor(shield.Color(255, 255, 0));

  pinMode(pin_sensor, INPUT);
  pinMode(pin_buzzer, OUTPUT);

  //we define the "detectClaps" function below;
  attachInterrupt(digitalPinToInterrupt(pin_sensor), detectClaps, RISING);

  //talk to RTC
  setupRTC();
}

void loop()
{
  // put your main code here, to run repeatedly:

  DateTime realTimeNow = rtc.now();

  int hours = realTimeNow.hour();
  int minutes = realTimeNow.minute();

#ifdef TIMELAPSE

  hoursAsDecimal += 0.1; //add 6 minutes each loop (100ms)
  if (hoursAsDecimal > 24)
  {
    hoursAsDecimal -= 24;
  }
  Serial.print("Time as hours of day: ");
  Serial.println(hoursAsDecimal);

#else

  hoursAsDecimal = hours + (minutes / 60); //calculate from real time

#endif

  shield.clear();
  setColourFromTime(hoursAsDecimal);
  shield.show();



  if (hasClapped)
  {
    //show time on the shield;
    Serial.print("Got clap signal, outputing time: ");

    char buf[] = "hh:mm:ss"; 

    Serial.println(realTimeNow.toString(buf));

    char buf2[] = "hh:mm";
    realTimeNow.toString(buf2);

    for(char *i = buf2; *i != 0; i++){
      shield.clear();
      shield.setCursor(w-5,0);
      shield.print(*i); 
      shield.show(); 
      Serial.println(*i);
      delay(1000);
    }


    hasClapped = false;
  }

  delay(100);
}

void setColourFromTime(double hours)
{

  //we want the morning, 0 -> 6; to go from red to bright white/blue;
  if (hours < morningTime)
  {

    //midnight is (128, 0, 0 );

    float rate = hours / morningTime; //0->1 according to how the hours fit into morning time.
    float sharpRate = rate * rate * rate; // sharper turn in the graph; still between 0->1 

    //we want to adjust the difference: (128,0,0) -> (204,230,255); over the time of rate:

    shield.fillRect(0, 0, w, h, shield.Color(128 + (76 * rate), 230 * sharpRate, 255 * sharpRate));
  }
  else if (hours < afternoonTime) // between 6am and 4pm
  {
    //fill with about a skyblue
    shield.fillRect(0, 0, w, h, shield.Color(204, 230, 255));

    double rate = (hours - morningTime) / (afternoonTime - morningTime);

    unsigned long time = 10000 * rate;

    shield.fillCircle(0, 0, 4, shield.Color(204 + abs(time % 100 - 50), 230 + abs(time % 40 - 20), 255));
    shield.fillCircle(4, 6, 3, shield.Color(204 + abs(time % 50 - 25), 230 + abs(time % 35 - 20), 255));
  }
  else if (hours < nightTime)
  {

    double rate = (hours - afternoonTime) / (nightTime - afternoonTime);
    shield.fillRect(0, 0, w, h, shield.Color(204 + (51 * rate),  //fill out the rest of red
                                             230 * (1 - rate),   //reduce green to nothing
                                             255 * (1 - rate))); //reduce blue to nothing
  }
  else //20->0 full dark red; nighttime
  {
    double rate = (hours - nightTime) / (24 - nightTime);
    //now reduce red to dim; sleepy time
    shield.fillRect(0, 0, shield.width(), shield.height(), shield.Color(128 + 128 * (1 - rate), 0, 0));
  }
}

// this is to set up the first time, and burn in the current time if it doesn't have anything
void setupRTC()
{

  if (!rtc.begin())
  {
    Serial.println("Could not find RTC, is the datalogging shield connected?");
    Serial.println("Halt!");
    while (1)
      ; //do nothing, forever
  }

  DateTime now = rtc.now();

  Serial.println("RTC running, time:");
  Serial.println(now.toString(timeBuffer));
}

// This function will fire any time we detect a clap
void detectClaps()
{
  unsigned long timeNow = millis();

  if (firstClapTime == 0)
  {
    firstClapTime = timeNow;
    return;
  }

  //duration from first clap
  unsigned long duration = timeNow - firstClapTime;

  if (duration > clap_debounce_ms && duration < clap_delay_ms)
  {
    //two claps genuine; set clapped flag
    firstClapTime = 0;
    hasClapped = true;
  }
  else if (duration > clap_delay_ms)
  {
    //timeout.. took too long
    //this is when we've clapped by the way, so set the timeNow to be our clap.
    firstClapTime = timeNow;
  }
}
