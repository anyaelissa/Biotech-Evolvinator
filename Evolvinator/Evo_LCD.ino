
void LcdUpdate () {
  lcd.clear();

  // Print time
  lcd.setCursor(0,0);
  unsigned long tNow = now();
  lcd.print(year(tNow));
  lcd.print("-");
  if (month(tNow) < 10) {
    lcd.print("0");
  }
  lcd.print(month(tNow));
  lcd.print("-");
  if (day(tNow) < 10) {
    lcd.print("0");
  }
  lcd.print(day(tNow));
  lcd.print(" ");
  if (hour(tNow) < 10) {
    lcd.print("0");
  }
  lcd.print(hour(tNow));
  lcd.print(":");
  if (minute(tNow) < 10) {
    lcd.print("0");
  }
  lcd.print(minute(tNow));
  lcd.print(":");
  if (second(tNow) < 10) {
    lcd.print("0");
  }
  lcd.print(second(tNow));

  // Print temp
  lcd.setCursor(0,1);
  lcd.print("Temp: ");
  lcd.print(tempPrintAvg);

  // Print OD data
  lcd.setCursor(0,2);
  lcd.print("OD: ");
  lcd.print(ODMin[0]);

  // Print IP address
  lcd.setCursor(0, 3);
  lcd.print(Ethernet.localIP());
}