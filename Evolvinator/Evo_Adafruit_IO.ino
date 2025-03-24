#include "arduino_secrets.h"
#include "AdafruitIO_Ethernet.h"

AdafruitIO_Ethernet io(SECRET_ADAFRUIT_IO_USERNAME, SECRET_ADAFRUIT_IO_KEY);

AdafruitIO_Feed *temperature = io.feed("temperature");
AdafruitIO_Feed *od = io.feed("OD reading");
AdafruitIO_Feed *totalvolume = io.feed("Total Volume Added");

void AdafruitIOInitialize() {
  // connect to io.adafruit.com
  Serial.print("Connecting to Adafruit IO");
  io.connect();

  // wait for a connection
  while(io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  // we are connected
  Serial.println();
  Serial.println(io.statusText());
}

void AdafruitIOLoop() {
  io.run();
}

void LogDataToAdafruitIO(){
  Serial.println("Logging data to Adafruit: ");
  Serial.print("temp: ");
  Serial.println(tempPrintAvg);
  Serial.print("od: ");
  Serial.println(ODMin[0]);
  Serial.print("total volume: ");
  Serial.println(totalVol);
  temperature->save(tempPrintAvg);
  od->save(ODMin[0]);
  totalvolume->save(totalVol);
}
