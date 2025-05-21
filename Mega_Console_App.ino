#include <LiquidCrystal.h>
#include <Keypad.h>
#include <Wire.h>
#include <RTClib.h>

// LCD (Standard)
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

// LED and Buzzer pins
const int greenLED = 5;
const int redLED = 4;
const int buzzer = 6;

// Keypad setup
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {A0, A1, A2, A3};
byte colPins[COLS] = {A4, A5, 2, 3};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

RTC_DS1307 rtc;

// Signal functions for feedback
void signalCorrect() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(greenLED, HIGH);
    tone(buzzer, 1000);
    delay(300);
    digitalWrite(greenLED, LOW);
    noTone(buzzer);
    delay(300);
  }
}

void signalWrong() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(redLED, HIGH);
    tone(buzzer, 200);
    delay(300);
    digitalWrite(redLED, LOW);
    noTone(buzzer);
    delay(300);
  }
}

// Helper: get a single char input from keypad or serial
char getInputChar() {
  char key = keypad.getKey();
  if (key) return key;
  if (Serial.available()) return Serial.read();
  return 0; // no input available
}

// Helper: get a string input line from keypad or serial, ends on '#' or newline
String getInputLine(const char* prompt = nullptr) {
  if (prompt != nullptr) {
    lcd.clear();
    lcd.print(prompt);
    Serial.println(prompt);
  }

  String input = "";
  lcd.setCursor(0, 1);
  lcd.print("> ");
  while (true) {
    char k = getInputChar();
    if (k) {
      if (k == '#' || k == '\n') break;
      else if (k == '*') {
        if (input.length() > 0) {
          input.remove(input.length() - 1);
          lcd.setCursor(2, 1);
          lcd.print("                ");
          lcd.setCursor(2, 1);
          lcd.print(input);
          Serial.print("\r");
          Serial.print(input);
          Serial.print(" ");
        }
      } else if (isPrintable(k) && input.length() < 31) {
        input += k;
        lcd.setCursor(2, 1);
        lcd.print(input);
        Serial.print(k);
      }
    }
    delay(10);
  }
  Serial.println();
  return input;
}

// Helper: get integer input
int getIntInput(const char* prompt = nullptr) {
  return getInputLine(prompt).toInt();
}

// Helper: get float input
float getFloatInput(const char* prompt = nullptr) {
  return getInputLine(prompt).toFloat();
}

// ------------------- App 1: Scientific Calculator -------------------

String getInputExpr() {
  String input = "";
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calc (A=OK B=Clr)");

  lcd.setCursor(0, 1);
  lcd.print("> ");

  while (true) {
    char key = getInputChar();
    if (key) {
      if (key == 'A') break;
      else if (key == 'B') {
        input = "";
        lcd.setCursor(2, 1);
        lcd.print("                ");
      }
      else if (key == 'D') {
        if (input.length() > 0) {
          input.remove(input.length() - 1);
          lcd.setCursor(2, 1);
          lcd.print("                ");
          lcd.setCursor(2, 1);
          lcd.print(input);
        }
      }
      else if (isPrintable(key)) {
        if (input.length() < 31) {
          input += key;
          lcd.setCursor(2, 1);
          lcd.print(input);
        }
      }
    }
    delay(10);
  }
  return input;
}

float factorial(int n) {
  if (n < 0) return NAN;
  float fact = 1;
  for (int i = 1; i <= n; i++) fact *= i;
  return fact;
}

float evaluateExpr(String expr) {
  expr.trim();
  expr.toUpperCase();

   // Handle factorial
  if (expr.endsWith("!")) {
    int num = expr.substring(0, expr.length() - 1).toInt();
    return factorial(num);
  }
  // Handle functions: SIN(30), COS(45), etc.
  if (expr.startsWith("S")) return sin(radians(expr.substring(1).toFloat()));
  if (expr.startsWith("C")) return cos(radians(expr.substring(1).toFloat()));
  if (expr.startsWith("T")) return tan(radians(expr.substring(1).toFloat()));
  if (expr.startsWith("L")) return log10(expr.substring(1).toFloat());
  if (expr.startsWith("LN("))  return log(expr.substring(3, expr.length() - 1).toFloat());
  if (expr.startsWith("R")) return sqrt(expr.substring(1).toFloat());

  // Look for basic operator: +, -, *, /, %, ^
  char op = 0;
  int opIndex = -1;
  const char ops[] = {'+', '-', '*', '/', '%', '^'};

  for (int i = 1; i < expr.length(); i++) {
    char c = expr.charAt(i);
    if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '^') {
      op = c;
      opIndex = i;
      break;
    }
  }
  if (opIndex == -1) return NAN;

  float num1 = expr.substring(0, opIndex).toFloat();
  float num2 = expr.substring(opIndex + 1).toFloat();

  switch (op) {
    case '+': return num1 + num2;
    case '-': return num1 - num2;
    case '*': return num1 * num2;
    case '/': return num2 != 0 ? num1 / num2 : NAN;
    case '%': return int(num2) != 0 ? int(num1) % int(num2) : NAN;
    case '^': return pow(num1, num2);
    default: return NAN;
  }
}

void scientificCalculatorApp() {
  while (true) {
    String expr = getInputExpr();
    float result = evaluateExpr(expr);

    lcd.clear();
    if (isnan(result)) {
      lcd.print("Invalid Input");
      signalWrong();
    } else {
      lcd.print("Result:");
      lcd.setCursor(0, 1);
      lcd.print(result, 4);
      signalCorrect();
    }

    delay(3000);
    lcd.clear();
    lcd.print("A: Again B: Exit");

    while (true) {
      char key = getInputChar();
      if (key == 'A') break;
      if (key == 'B') return;
      delay(10);
    }
  }
}

// ------------------- App 2: To-Do List -------------------

#define MAX_TASKS 5
String todoList[MAX_TASKS];
int taskCount = 0;

String getTextInput(String prompt) {
  return getInputLine(prompt.c_str());
}

void viewList() {
  lcd.clear();
  if (taskCount == 0) {
    lcd.print("List is empty");
    signalWrong();
    delay(2000);
    return;
  }
  for (int i = 0; i < taskCount; i++) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(String(i + 1) + ": " + todoList[i]);
    delay(2000);
  }
}

void addTask() {
  if (taskCount >= MAX_TASKS) {
    lcd.clear();
    lcd.print("List Full!");
    signalWrong();
    delay(2000);
    return;
  }
  String task = getTextInput("Add Task (#=OK)");
  if (task.length() > 0) {
    todoList[taskCount++] = task;
    lcd.clear();
    lcd.print("Task Added!");
    signalCorrect();
  } else {
    lcd.clear();
    lcd.print("Empty Task!");
    signalWrong();
  }
  delay(2000);
}

void deleteTask() {
  if (taskCount == 0) {
    lcd.clear();
    lcd.print("List Empty");
    signalWrong();
    delay(2000);
    return;
  }
  String input = getTextInput("Del Task Num:");
  int idx = input.toInt() - 1;
  if (idx >= 0 && idx < taskCount) {
    for (int i = idx; i < taskCount - 1; i++) {
      todoList[i] = todoList[i + 1];
    }
    taskCount--;
    lcd.clear();
    lcd.print("Task Deleted");
    signalCorrect();
  } else {
    lcd.clear();
    lcd.print("Invalid No.");
    signalWrong();
  }
  delay(2000);
}

void todoListApp() {
  while (true) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("A:Add B:View");
    lcd.setCursor(0, 1);
    lcd.print("C:Del D:Exit");

    char key = 0;
    while (!key) key = getInputChar();

    if (key == 'A') addTask();
    else if (key == 'B') viewList();
    else if (key == 'C') deleteTask();
    else if (key == 'D') break;
    delay(100);
  }
}

// ------------------- App 3: Unit Converter -------------------

enum Category { LENGTH, TEMP, WEIGHT };
Category currentCategory = LENGTH;
bool direction = true;  // true = one direction, false = reverse

float getNumberInput(const char* prompt) {
  return getFloatInput(prompt);
}

void showCategory() {
  lcd.clear();
  lcd.setCursor(0, 0);
  switch (currentCategory) {
    case LENGTH:
      lcd.print("Length Conv");
      lcd.setCursor(0, 1);
      lcd.print(direction ? "M->Ft" : "Ft->M");
      break;
    case TEMP:
      lcd.print("Temp Conv");
      lcd.setCursor(0, 1);
      lcd.print(direction ? "C->F" : "F->C");
      break;
    case WEIGHT:
      lcd.print("Weight Conv");
      lcd.setCursor(0, 1);
      lcd.print(direction ? "Kg->Lbs" : "Lbs->Kg");
      break;
  }
}

float convertLength(float val, bool dir) {
  return dir ? val * 3.28084 : val / 3.28084;
}

float convertTemp(float val, bool dir) {
  return dir ? val * 9.0 / 5.0 + 32 : (val - 32) * 5.0 / 9.0;
}

float convertWeight(float val, bool dir) {
  return dir ? val * 2.20462 : val / 2.20462;
}

void unitConverterApp() {
  while (true) {
    showCategory();
    lcd.setCursor(0, 1);
    lcd.print("A:Cat B:Dir D:Exit");

    char key = 0;
    while (!key) key = getInputChar();

    if (key == 'A') {
      currentCategory = static_cast<Category>((currentCategory + 1) % 3);
      delay(300);
      continue;
    }
    if (key == 'B') {
      direction = !direction;
      delay(300);
      continue;
    }
    if (key == 'D') {
      lcd.clear();
      lcd.print("Exiting...");
      delay(1000);
      break;
    }
    if (key == 'C') {
      // reserved for future use
      delay(300);
      continue;
    }

    float inputValue = getNumberInput("Enter val:#=OK");
    if (inputValue == 0.0) {
      lcd.clear();
      lcd.print("Invalid input");
      signalWrong();
      delay(1500);
      continue;
    }

    float result;
    switch (currentCategory) {
      case LENGTH: result = convertLength(inputValue, direction); break;
      case TEMP: result = convertTemp(inputValue, direction); break;
      case WEIGHT: result = convertWeight(inputValue, direction); break;
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Result:");
    lcd.setCursor(0, 1);
    lcd.print(result, 2);
    signalCorrect();
    delay(4000);
  }
}

// ------------------- App 4: Number Guessing Game -------------------

int getGuessInput() {
  String input = "";
  unsigned long startTime = millis();

  while (true) {
    char key = getInputChar();
    if (key) {
      if (key == '#') break;
      else if (key == '*') {
        if (input.length() > 0) input.remove(input.length() - 1);
      } else if (isDigit(key) && input.length() < 3) {
        input += key;
      }

      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print(input);
      Serial.print("\rYour guess: " + input);
    }

    if (Serial.available()) {
      int val = Serial.parseInt();
      if (val > 0 && val <= 999) return val;
    }

    if (millis() - startTime > 10000) {
      if (input.length() > 0) return input.toInt();
      else return -1;
    }

    delay(50);
  }

  return input.toInt();
}

void numberGuessingGame() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Guess (1 to 100):");
  Serial.println("Number Guessing Game Started!");
  Serial.println("Enter a number between 1 and 100...");

  int secretNumber = random(1, 101);
  int attempts = 0;

  while (true) {
    lcd.setCursor(0, 0);
    lcd.print("Guess (1 to 100):");
    lcd.setCursor(0, 1);
    lcd.print("                ");

    int guess = getGuessInput();

    if (guess == -1) {
      Serial.println("\nTimeout!");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Time's up!");
      signalWrong();
      break;
    }

    attempts++;
    Serial.print("\nYou guessed:  ");
    Serial.println(guess);

    if (guess == secretNumber) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Correct in ");
      lcd.print(attempts);
      lcd.print(" tries!");
      Serial.println("Correct guess!");
      signalCorrect();
      break;
    } else if (guess < secretNumber) {
      lcd.setCursor(0, 1);
      lcd.print("Too low!");
      Serial.println("Too low!");
      signalWrong();
    } else {
      lcd.setCursor(0, 1);
      lcd.print("Too high!");
      Serial.println("Too high!");
      signalWrong();
    }

    delay(1000);
    lcd.clear();
  }

  delay(3000);
  lcd.clear();
}

// ------------------- App 5: Music Player -------------------
void playMelody(int melody[], int durations[], int length) {
  lcd.clear();
  lcd.print("Playing tune...");
  for (int i = 0; i < length; i++) {
    tone(buzzer, melody[i], durations[i]);
    digitalWrite(greenLED, HIGH);
    delay(durations[i] * 1.2);
    digitalWrite(greenLED, LOW);
    noTone(buzzer);
  }
  lcd.clear();
  lcd.print("Done!");
  delay(1000);
}

void musicPlayer() {
  lcd.clear();
  lcd.print("Select Tune:");
  lcd.setCursor(0, 1);
  lcd.print("1:HB 2:Mary 3:Twink");

  char choice = 0;
  while (!choice) {
    char k = getInputChar();
    if (k == '1' || k == '2' || k == '3') choice = k;
  }

  if (choice == '1') {
    int melody[] = {262, 262, 294, 262, 349, 330,
                    262, 262, 294, 262, 392, 349,
                    262, 262, 523, 440, 349, 330, 294,
                    466, 466, 440, 349, 392, 349};
    int durations[] = {300, 300, 600, 600, 600, 900,
                       300, 300, 600, 600, 600, 900,
                       300, 300, 600, 600, 600, 600, 900,
                       300, 300, 600, 600, 600, 900};
    int length = sizeof(melody) / sizeof(melody[0]);
    playMelody(melody, durations, length);
  } else if (choice == '2') {
    int melody[] = {330, 294, 262, 294, 330, 330, 330,
                    294, 294, 294, 330, 392, 392,
                    330, 294, 262, 294, 330, 330, 330,
                    330, 294, 294, 330, 294, 262};
    int durations[] = {400,400,400,400,400,400,800,
                       400,400,800,400,400,800,
                       400,400,400,400,400,400,800,
                       400,400,400,400,400,800};
    int length = sizeof(melody) / sizeof(melody[0]);
    playMelody(melody, durations, length);
  } else if (choice == '3') {
    int melody[] = {262, 262, 392, 392, 440, 440, 392,
                    349, 349, 330, 330, 294, 294, 262,
                    392, 392, 349, 349, 330, 330, 294,
                    392, 392, 349, 349, 330, 330, 294,
                    262, 262, 392, 392, 440, 440, 392,
                    349, 349, 330, 330, 294, 294, 262};
    int durations[] = {400,400,400,400,400,400,800,
                       400,400,400,400,400,400,800,
                       400,400,400,400,400,400,800,
                       400,400,400,400,400,400,800,
                       400,400,400,400,400,400,800,
                       400,400,400,400,400,400,800};
    int length = sizeof(melody) / sizeof(melody[0]);
    playMelody(melody, durations, length);
  }
  lcd.clear();
  lcd.print("Music Done!");
  delay(2000);
}

// ------------------- RTC Idle Display -------------------

void displayRTC() {
  DateTime now = rtc.now();
  char buf[17];

  lcd.clear();

  sprintf(buf, "Date: %02d/%02d/%04d", now.day(), now.month(), now.year());
  lcd.setCursor(0, 0);
  lcd.print(buf);

  sprintf(buf, "Time: %02d:%02d:%02d", now.hour(), now.minute(), now.second());
  lcd.setCursor(0, 1);
  lcd.print(buf);
}

int getMenuChoiceWithTimeout(unsigned long timeout=5000) {
  unsigned long start = millis();
  while (millis() - start < timeout) {
    // check keypad or serial input
    char k = getInputChar();
    if (k && k >= '1' && k <= '5') {
      Serial.print("\nSelected Input: ");
      Serial.println(k);
      return k - '0';
    }
    displayRTC();
    delay(200);
  }
  return 0; // timeout
}

// ------------------- Main Setup & Loop -------------------

void setup() {
  lcd.begin(16, 2);
  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(buzzer, OUTPUT);
  digitalWrite(greenLED, LOW);
  digitalWrite(redLED, LOW);
  noTone(buzzer);
  Serial.begin(9600);
  randomSeed(analogRead(A5));
  Wire.begin();

  if (!rtc.begin()) {
    lcd.clear();
    lcd.print("RTC not found!");
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running, setting time");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void loop() {
  int choice = getMenuChoiceWithTimeout(5000);

  if (choice == 0) {
    while (true) {
      displayRTC();
      delay(1000);
      char k = getInputChar();
      if (k && k >= '1' && k <= '5') {
        choice = k - '0';
        break;
      }
    }
  }

  switch (choice) {
    case 1: scientificCalculatorApp(); break;
    case 2: todoListApp(); break;
    case 3: unitConverterApp(); break;
    case 4: numberGuessingGame(); break;
    case 5: musicPlayer(); break;
    default:
      lcd.clear();
      lcd.print("Invalid input");
      signalWrong();
      delay(2000);
      break;
  }
}
