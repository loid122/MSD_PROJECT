#include <Servo.h>

// Pin definitions
#define LDR_PIN A2
#define RED_PIN 10
#define GREEN_PIN 11
#define BLUE_PIN 9
#define SERVO_PIN 5
#define MAGNET_PIN 7

// Servo control
Servo myServo;

// Calibration factors
float redFactor = 1.0, greenFactor = 1.0, blueFactor = 1.0;
int redVal, greenVal, blueVal;
const int colorThreshold = 15;

// Detection tracking
int detectionCount = 0;
const int requiredDetections = 3;
char pickupColor = ' ';
char dropColor = ' ';
bool colorFound = false;
bool magnetOn = false;

// Operation states
enum OperationState { PICKUP, DROP };
OperationState currentState = PICKUP;

void setup() {
  Serial.begin(9600);
  
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(MAGNET_PIN, OUTPUT);
  digitalWrite(MAGNET_PIN, LOW);

  // Initialize servo
  myServo.attach(SERVO_PIN);
  myServo.write(0);
  delay(1000);

  Serial.println("Starting Auto Calibration...");
  autoCalibrate();
  
  Serial.println("Calibration Complete!");
  Serial.println("----------------------------");
  
  // Start with pickup mode
  askForPickupColor();
}

void loop() {
  if (currentState == PICKUP && pickupColor != ' ' && !colorFound) {
    findAndPickColor();
  } else if (currentState == DROP && dropColor != ' ' && !colorFound) {
    findAndDropColor();
  }
}

void autoCalibrate() {
  Serial.println("Starting RGB calibration on white surface...");
  
  int redReadings[5], greenReadings[5], blueReadings[5];
  float redAvg = 0, greenAvg = 0, blueAvg = 0;
  
  // Take multiple readings for each color
  for (int i = 0; i < 5; i++) {
    // Read RED
    setColor(255, 0, 0);
    delay(100);
    redReadings[i] = analogRead(LDR_PIN);
    delay(50);
    
    // Read GREEN
    setColor(0, 255, 0);
    delay(100);
    greenReadings[i] = analogRead(LDR_PIN);
    delay(50);
    
    // Read BLUE
    setColor(0, 0, 255);
    delay(100);
    blueReadings[i] = analogRead(LDR_PIN);
    delay(50);
    
    setColor(0, 0, 0);
    delay(100);
    
    redAvg += redReadings[i];
    greenAvg += greenReadings[i];
    blueAvg += blueReadings[i];
    
    Serial.print("Sample "); Serial.print(i+1);
    Serial.print(" - R:"); Serial.print(redReadings[i]);
    Serial.print(" G:"); Serial.print(greenReadings[i]);
    Serial.print(" B:"); Serial.println(blueReadings[i]);
  }
  
  // Calculate averages
  redAvg /= 5;
  greenAvg /= 5;
  blueAvg /= 5;
  
  Serial.print("Averages - R:"); Serial.print(redAvg);
  Serial.print(" G:"); Serial.print(greenAvg);
  Serial.print(" B:"); Serial.println(blueAvg);
  
  // Calculate overall average (target value)
  float targetAvg = (redAvg + greenAvg + blueAvg) / 3.0;
  Serial.print("Target average: "); Serial.println(targetAvg);
  
  // Calculate calibration factors
  redFactor = targetAvg / redAvg;
  greenFactor = targetAvg / greenAvg;
  blueFactor = targetAvg / blueAvg;
  
  // Clamp factors to reasonable range
  redFactor = constrain(redFactor, 0.1, 2.0);
  greenFactor = constrain(greenFactor, 0.1, 2.0);
  blueFactor = constrain(blueFactor, 0.1, 2.0);
  
  Serial.println("Calibration Factors:");
  Serial.print("Red Factor: "); Serial.println(redFactor, 3);
  Serial.print("Green Factor: "); Serial.println(greenFactor, 3);
  Serial.print("Blue Factor: "); Serial.println(blueFactor, 3);
  
  // Test calibration
  testCalibration();
}

void testCalibration() {
  int testRed, testGreen, testBlue;
  
  setColor(255, 0, 0);
  delay(100);
  testRed = analogRead(LDR_PIN);
  delay(50);
  
  setColor(0, 255, 0);
  delay(100);
  testGreen = analogRead(LDR_PIN);
  delay(50);
  
  setColor(0, 0, 255);
  delay(100);
  testBlue = analogRead(LDR_PIN);
  delay(50);
  
  setColor(0, 0, 0);
  
  float adjRed = testRed * redFactor;
  float adjGreen = testGreen * greenFactor;
  float adjBlue = testBlue * blueFactor;
  
  Serial.print("Test Raw - R:"); Serial.print(testRed);
  Serial.print(" G:"); Serial.print(testGreen);
  Serial.print(" B:"); Serial.println(testBlue);
  
  Serial.print("Test Adj - R:"); Serial.print(adjRed);
  Serial.print(" G:"); Serial.print(adjGreen);
  Serial.print(" B:"); Serial.println(adjBlue);
}

void askForPickupColor() {
  Serial.println("\n=== PICKUP MODE ===");
  Serial.println("Enter color to pickup (R/G/B):");
  
  pickupColor = ' '; // Reset pickup color
  while (pickupColor == ' ') {
    if (Serial.available()) {
      char c = Serial.read();
      while (Serial.available()) Serial.read(); // Clear buffer
      
      if (c == 'R' || c == 'r' || c == 'G' || c == 'g' || c == 'B' || c == 'b') {
        pickupColor = toupper(c);
        Serial.print("Pickup color set to: ");
        Serial.println(pickupColor);
        currentState = PICKUP;
        colorFound = false;
      } else if (c != '\n' && c != '\r') {
        Serial.println("Invalid input. Please enter R, G, or B:");
      }
    }
    delay(100);
  }
}

void askForDropColor() {
  Serial.println("\n=== DROP MODE ===");
  Serial.println("Enter drop location color (R/G/B):");
  
  dropColor = ' '; // Reset drop color
  while (dropColor == ' ') {
    if (Serial.available()) {
      char c = Serial.read();
      while (Serial.available()) Serial.read(); // Clear buffer
      
      if (c == 'R' || c == 'r' || c == 'G' || c == 'g' || c == 'B' || c == 'b') {
        dropColor = toupper(c);
        Serial.print("Drop color set to: ");
        Serial.println(dropColor);
        currentState = DROP;
        colorFound = false; // Reset for drop phase
      } else if (c != '\n' && c != '\r') {
        Serial.println("Invalid input. Please enter R, G, or B:");
      }
    }
    delay(100);
  }
}

void findAndPickColor() {
  Serial.println("Starting sweep to find pickup color...");
  Serial.println("Angle\tDetected\tCount");
  Serial.println("----------------------------");
  
  detectionCount = 0;
  colorFound = false;
  
  // Sweep from 0 to 180 degrees
  for (int angle = 0; angle <= 180; angle += 5) {
    // Move servo to current angle
    myServo.write(angle);
    delay(500);  // Wait for servo to stabilize
    
    // Read colors at this position
    readAllColorsOriginal();
    
    // Apply calibration factors
    float calibratedRed = redVal * redFactor;
    float calibratedGreen = greenVal * greenFactor;
    float calibratedBlue = blueVal * blueFactor;
    
    // Detect color using calibrated values
    char detectedColor = detectColorCalibrated(calibratedRed, calibratedGreen, calibratedBlue);
    
    Serial.print(angle);
    Serial.print("°\t");
    Serial.print(detectedColor);
    Serial.print("\t\t");
    Serial.println(detectionCount);
    
    // Check if detected color matches target
    if (detectedColor == pickupColor) {
      detectionCount++;
      Serial.print("Match found! Detection count: ");
      Serial.println(detectionCount);
      
      // If we've detected enough times, stop and activate magnet
      if (detectionCount >= requiredDetections) {
        Serial.println("Pickup color confirmed! Activating magnet...");
        activateMagnet();
        colorFound = true;
        
        // Move to drop mode
        currentState = DROP;
        askForDropColor();
        return; // Exit the function immediately after setting up drop mode
      }
    } else {
      // Reset counter if color doesn't match
      detectionCount = 0;
    }
    
    // Small delay between readings
    delay(100);
  }
  
  // Only reach here if color wasn't found in sweep
  if (!colorFound) {
    Serial.println("Pickup color not found in sweep range.");
    // Reset for next attempt
    pickupColor = ' ';
    askForPickupColor();
  }
}

void findAndDropColor() {
  Serial.println("Moving servo to 0 position...");
  myServo.write(0);
  delay(1000);
  
  Serial.println("Starting sweep to find drop color...");
  Serial.println("Angle\tDetected\tCount");
  Serial.println("----------------------------");
  
  detectionCount = 0;
  colorFound = false;
  
  // Sweep from 0 to 180 degrees
  for (int angle = 0; angle <= 180; angle += 5) {
    // Move servo to current angle
    myServo.write(angle);
    delay(500);  // Wait for servo to stabilize
    
    // Read colors at this position
    readAllColorsOriginal();
    
    // Apply calibration factors
    float calibratedRed = redVal * redFactor;
    float calibratedGreen = greenVal * greenFactor;
    float calibratedBlue = blueVal * blueFactor;
    
    // Detect color using calibrated values
    char detectedColor = detectColorCalibrated(calibratedRed, calibratedGreen, calibratedBlue);
    
    Serial.print(angle);
    Serial.print("°\t");
    Serial.print(detectedColor);
    Serial.print("\t\t");
    Serial.println(detectionCount);
    
    // Check if detected color matches target
    if (detectedColor == dropColor) {
      detectionCount++;
      Serial.print("Match found! Detection count: ");
      Serial.println(detectionCount);
      
      // If we've detected enough times, stop and deactivate magnet
      if (detectionCount >= requiredDetections) {
        Serial.println("Drop color confirmed! Deactivating magnet...");
        deactivateMagnet();
        colorFound = true;
        
        // Reset for next cycle
        delay(2000);
        pickupColor = ' ';
        dropColor = ' ';
        currentState = PICKUP;
        askForPickupColor();
        return; // Exit the function immediately
      }
    } else {
      // Reset counter if color doesn't match
      detectionCount = 0;
    }
    
    // Small delay between readings
    delay(100);
  }
  
  // Only reach here if color wasn't found in sweep
  if (!colorFound) {
    Serial.println("Drop color not found in sweep range.");
    // Reset for next attempt
    dropColor = ' ';
    askForDropColor();
  }
}

void readAllColorsOriginal() {
  // RED light test
  setColor(255, 0, 0);         
  delay(75);
  redVal = analogRead(LDR_PIN);
  delay(75);

  // GREEN light test
  setColor(0, 255, 0);         
  delay(75);
  greenVal = analogRead(LDR_PIN);
  delay(75);

  // BLUE light test
  setColor(0, 0, 255);         
  delay(75);
  blueVal = analogRead(LDR_PIN);
  delay(75);

  // Turn OFF LED
  setColor(0, 0, 0);
}

char detectColorCalibrated(float r, float g, float b) {
  // Find minimum and second minimum
  float minVal = min(r, min(g, b));
  float secondMin;

  if (minVal == r)
    secondMin = min(g, b);
  else if (minVal == g)
    secondMin = min(r, b);
  else
    secondMin = min(r, g);

  // Check if difference is significant
  if (abs(secondMin - minVal) > colorThreshold) {
    if (r == minVal)
      return 'R';
    else if (g == minVal)
      return 'G';
    else if (b == minVal)
      return 'B';
  }
  
  return 'U'; // Unknown
}

void activateMagnet() {
  Serial.println("ACTIVATING MAGNET - PICKUP IN PROGRESS");
  digitalWrite(MAGNET_PIN, HIGH);
  magnetOn = true;
  delay(1000);  // Keep magnet on for pickup
  Serial.println("Magnet activated and holding.");
}

void deactivateMagnet() {
  Serial.println("DEACTIVATING MAGNET - DROP IN PROGRESS");
  digitalWrite(MAGNET_PIN, LOW);
  magnetOn = false;
  delay(1000);  // Wait for drop to complete
  Serial.println("Magnet deactivated. Drop complete.");
}

// Function to control RGB LED
void setColor(int r, int g, int b) {
  analogWrite(RED_PIN, r);
  analogWrite(GREEN_PIN, g);
  analogWrite(BLUE_PIN, b);
}
