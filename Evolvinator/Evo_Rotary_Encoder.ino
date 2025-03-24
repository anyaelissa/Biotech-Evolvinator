const int channelAPin = 23;
const int channelBPin = 25;
const int buttonPin   = 27;
int rotaryCounter;

int aLastState;
int buttonLastState;
 
void setupRotaryEncoder() {
  Serial.println("Setting up rotary encoder...");
  pinMode(channelAPin, INPUT);
  pinMode(channelBPin, INPUT);
  pinMode(buttonPin, INPUT);
  aLastState = digitalRead(channelAPin);
  buttonLastState = digitalRead(buttonPin);
  rotaryCounter = 0;
}

void readRotaryEncoder() {
  int aState = digitalRead(channelAPin);
  if (aState != aLastState){
    // outputB != outputA state, encoder is rotating clockwise
    if (digitalRead(channelBPin) != aState) {
      movedClockwise();
    } else {
      movedCounterClockwise();
    }
  } 
  aLastState = aState;

  int buttonState = digitalRead(buttonPin);
  if (buttonState != buttonLastState) {
    if (buttonState == 1) {
      buttonUp();
    } else {
      buttonDown();
    }
    buttonLastState = buttonState;
  }
}

void movedClockwise() {
  rotaryCounter++;
  printCounter();
}

void movedCounterClockwise() {
  rotaryCounter--;
  printCounter();
}

void printCounter() {
  Serial.print("Rotary encoder position: ");
  Serial.println(rotaryCounter);
}

void buttonUp() {
  Serial.println("Button up event");
}

void buttonDown() {
  Serial.println("Button down event");
}
