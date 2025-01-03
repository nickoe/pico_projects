#include <Arduino.h>

// put function declarations here:
int myFunction(int, int);

void setup() {
  // put your setup code here, to run once:
  int result = myFunction(2, 3);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}
int i = 0;
void loop() {
  // put your main code here, to run repeatedly:
  i = myFunction(i,4);
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}