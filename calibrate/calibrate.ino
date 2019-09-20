
#include <Wire.h>
#include <RTClib.h>

#define println(x) Serial.println(x)
#define print(x) Serial.print(x)

const int DEBOUNCE_VALUE = 100;

RTC_DS1307 rtc;

volatile unsigned long clap = 0;

unsigned long timeThen = 0;

char timeBuffer[] = "time: hh:mm";

void detectClap()
{
  if(clap > 0 ){
    return;
  }

  clap = millis();
}

void setup()
{

  Serial.begin(9600);

  pinMode(2,INPUT);
  attachInterrupt(digitalPinToInterrupt(2), detectClap, RISING);

  while (!rtc.begin())
  {
    println("Could not find RTC, is the datalogging shield connected?");
    println("Checking in 1 second...\n");
    delay(1000);
  }

  if (!rtc.isrunning())
  {
    println("RTC is fresh and not running; burning the upload time:");
    println(F(__DATE__));
    println(F(__TIME__));

    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  DateTime timecheck = rtc.now();

  println("RTC running, time:");
  println(rtc.now().toString(timeBuffer));
  println("----------------------------------------");
  println(" Edit the sketch if this is not correct ");
  println("----------------------------------------");
  println("Now to adjust the pot; you should find two lights");
  println("on the audio module; if you turn the pot counter-clockwise");
  println("the light will turn off, and clockwise will turn the light on");
  println("adjust this so it is off until you clap\n\n");

}

void loop()
{

  unsigned long timeNow = millis();

  if (clap > 0)
  {
    println("CLAPPED!");
    println(clap);
    clap = 0;
  }

  unsigned long duration = timeNow - timeThen;

  if (duration > 1000)
  {
    print("t seconds:");
    println(rtc.now().unixtime());

    timeThen = timeNow;
  }

  delay(1);
}
