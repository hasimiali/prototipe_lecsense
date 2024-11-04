#include <Wire.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <math.h>

// Replace with your network credentials
const char *ssid = ".";
const char *password = "12345678";

// Replace with your API endpoint
const char *serverName = "https://dev.lecsens.zeonkun.tech/lecsens-data/store-new";

const int dataSize = 800;
float data_x[dataSize];
float data_y[dataSize];
float peak_x[2];
float peak_y[2];

float current_ppm;

String mac_address = "00:00:00:00:00:00";

// Array of cities for randomization
String cities[10] = {"Jakarta", "Surabaya", "Bandung", "Yogyakarta", "Medan", "Denpasar", "Makassar", "Semarang", "Palembang", "Balikpapan"};

// Array of labels for randomization
String labels[5] = {"Timbal", "Merkuri", "Kadmium", "Arsen", "Microplastic"};

// Function to convert float array to JSON string
String arrayToJson(float arr[], int size)
{
  String json = "[";
  for (int i = 0; i < size; i++)
  {
    json += String(arr[i], 6); // Use 6 decimal places
    if (i < size - 1)
    {
      json += ",";
    }
  }
  json += "]";
  return json;
}

// Function to initialize voltammetry data with the specified equation
void initVoltammetryData()
{
  // Randomize the coefficients a and b for the first cycle
  float a1 = 0.0160 + random(0, 11) * 0.0001; // Random between 0.0160 and 0.0170
  float b1 = 0.60 + random(0, 11) * 0.01;     // Random between 0.60 and 0.70

  // Randomize the coefficients a2 and b2 for the second cycle
  float a2 = 0.0160 + random(0, 11) * 0.0001; // Random between 0.0160 and 0.0170
  float b2 = 0.50 + random(0, 11) * 0.01;     // Random between 0.50 and 0.60

  // Initialize data_x values for two cycles
  for (int i = 0; i < 400; i++)
  {
    data_x[i] = -200 + i * 1.0; // x from -200 to 200 for the first cycle
  }
  for (int i = 400; i < 800; i++)
  {
    data_x[i] = 200 - (i - 400) * 1.0; // x from 200 to -200 for the second cycle
  }

  // Calculate data_y based on the equation for the first and second cycle
  for (int i = 0; i < 400; i++)
  {
    float x = data_x[i];
    x += 200;
    data_y[i] = -1.78e-7 * pow(x, 4) + 0.00011 * pow(x, 3) - a1 * pow(x, 2) + b1 * x - 6;
    // Serial.println(data_y[i]);
  }
  for (int i = 400; i < 800; i++)
  {
    float x = data_x[i];
    x += 200;
    data_y[i] = -1.78e-7 * pow(x, 4) + 0.00011 * pow(x, 3) - a2 * pow(x, 2) + b2 * x - 6;
    // Serial.println(data_y[i]);
  }
}

// Function to find the two peaks in data_y
void findPeaks()
{
  // Search for the highest value in the first cycle (0-400)
  float maxY1 = data_y[0];
  int maxIdx1 = 0;
  for (int i = 1; i < 400; i++)
  {
    if (data_y[i] > maxY1)
    {
      maxY1 = data_y[i];
      maxIdx1 = i;
    }
  }
  peak_x[0] = data_x[maxIdx1];
  peak_y[0] = maxY1;

  // Search for the highest value in the second cycle (400-800)
  float maxY2 = data_y[400];
  int maxIdx2 = 400;
  for (int i = 401; i < 800; i++)
  {
    if (data_y[i] > maxY2)
    {
      maxY2 = data_y[i];
      maxIdx2 = i;
    }
  }
  peak_x[1] = data_x[maxIdx2];
  peak_y[1] = maxY2;
}

void initWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());
}

// void sendData() {
//   if (WiFi.status() == WL_CONNECTED) {
//     HTTPClient http;

//     // Specify the target URL
//     http.begin(serverName);

//     // Set the content type to application/json
//     http.addHeader("Content-Type", "application/json");

//     // Randomize ppm, city, and label
//     int ppm = random(0, 101);  // Random ppm between 0 and 100
//     String alamat = cities[random(0, 10)];  // Random city from the list
//     String label = labels[random(0, 5)];  // Random label from the list

//     // Create the JSON payload using the variables
//     String jsonPayload = "{";
//     jsonPayload += "\"mac_address\":\"" + mac_address + "\",";
//     jsonPayload += "\"data_x\":" + arrayToJson(data_x, dataSize) + ",";
//     jsonPayload += "\"data_y\":" + arrayToJson(data_y, dataSize) + ",";
//     jsonPayload += "\"peak_x\":" + arrayToJson(peak_x, 2) + ",";
//     jsonPayload += "\"peak_y\":" + arrayToJson(peak_y, 2) + ",";
//     jsonPayload += "\"ppm\":" + String(ppm) + ",";
//     jsonPayload += "\"alamat\":\"" + alamat + "\",";
//     jsonPayload += "\"label\":\"" + label + "\"}";

//     // Send the POST request
//     int httpResponseCode = http.POST(jsonPayload);

//     // Check the response code
//     if (httpResponseCode > 0) {
//       String response = http.getString();
//       Serial.println(httpResponseCode);
//       Serial.println(response);
//     } else {
//       Serial.print("Error on sending POST: ");
//       Serial.println(httpResponseCode);
//     }

//     // Free resources
//     http.end();
//   } else {
//     Serial.println("WiFi Disconnected");
//   }
// }

int ppmIndex = 0; // Variable to keep track of the ppm sequence

void sendData(String customLabel)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;

    // Specify the target URL
    http.begin(serverName);

    // Set headers for the request
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJJRCI6IjQxMTc5ZTM5LWRhM2EtNGM4ZS1hMzYwLTAwNTA2MTBmMzMxNyIsIlVzZXJSb2xlIjoiQURNSU4iLCJleHAiOjE3MzA3NzUwODl9.MvKMYeHwItrlD7xZ9aiGMfnrVTUz8PHWKv6lJZtgkoI");

    // Calculate ppm based on the sequence
    int ppm = random(175, 191); // Generates a random value between 175 and 190 (inclusive)
    current_ppm = ppm;
    // current_ppm = 190;

    // Increment the index and reset if necessary
    ppmIndex = (ppmIndex + 1) % 3;

    // Random city from the list (replace the hardcoded string if needed)
    String alamat = "Surabaya, Jawa Timur"; // You can randomize this if needed

    // Create the JSON payload using the variables and the custom label
    String jsonPayload = "{";
    jsonPayload += "\"mac_address\":\"" + mac_address + "\",";
    jsonPayload += "\"data_x\":" + arrayToJson(data_x, dataSize) + ",";
    jsonPayload += "\"data_y\":" + arrayToJson(data_y, dataSize) + ",";
    jsonPayload += "\"peak_x\":" + arrayToJson(peak_x, 2) + ",";
    jsonPayload += "\"peak_y\":" + arrayToJson(peak_y, 2) + ",";
    jsonPayload += "\"ppm\":" + String(ppm) + ",";
    jsonPayload += "\"alamat\":\"" + alamat + "\",";
    jsonPayload += "\"label\":\"" + customLabel + "\"}";

    // Send the POST request
    int httpResponseCode = http.POST(jsonPayload);

    // Check the response code
    if (httpResponseCode > 0)
    {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);
    }
    else
    {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }

    // Free resources
    http.end();
  }
  else
  {
    Serial.println("WiFi Disconnected");
  }
}

// Define pins
const int button1Pin = 14;
const int button2Pin = 12;
// SDA 21
// SCL 22

// Variables to store the button state
int button1State = 0;
int button2State = 0;

// Variables to store the previous button state for edge detection
int lastButton1State = HIGH;
int lastButton2State = HIGH;

// OLED display dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

int displayState = 0;
int lastState = 0;
float randomNumber;
char buffer[20];

// Initialize the display with the SH1106 driver
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE, /* clock=*/22, /* data=*/21);
// Function prototype for displayCenteredText
void displayCenteredText(const char *text);
void displayCenteredText2(const char *line1, const char *line2);

void setup()
{
  // Initialize serial communication at 115200 baud rate
  Serial.begin(115200);
  initWiFi();

  // Initialize the button pins as input
  pinMode(button1Pin, INPUT_PULLUP);
  pinMode(button2Pin, INPUT_PULLUP);

  // Initialize the OLED display
  u8g2.begin();

  // Display initial message
  displayCenteredText("Press buttons");
}

void loop()
{

  if (displayState == 0)
  {
    displayCenteredText("LecSens");
  }
  else if (displayState == 1)
  {
    displayCenteredText("Merkuri Mode");
  }
  else if (displayState == 2)
  {
    displayCenteredText("Timbal Mode");
  }
  else if (displayState == 3)
  {
    displayCenteredText("Kadmium Mode");
  }
  else if (displayState == 4)
  {
    displayCenteredText("Arsen Mode");
  }
  else if (displayState == 5)
  {
    displayCenteredText("Micro Plastic Mode");
  }
  else if (displayState == 6)
  {
    if (lastState == 1)
    {
      displayCenteredText2("Merkuri(Hg): ", buffer);
    }
    else if (lastState == 2)
    {
      displayCenteredText2("Timbal(Pb): ", buffer);
    }
    else if (lastState == 3)
    {
      displayCenteredText2("Kadmium: ", buffer);
    }
    else if (lastState == 4)
    {
      displayCenteredText2("Arsen: ", buffer);
    }
    else if (lastState == 5)
    {
      displayCenteredText2("Micro Plastic: ", buffer);
    }
  }

  // Read the state of button 1
  button1State = digitalRead(button1Pin);

  // Read the state of button 2
  button2State = digitalRead(button2Pin);

  // Check if button 1 is pressed (state changed from HIGH to LOW)
  if (button1State == LOW && lastButton1State == HIGH)
  {
    Serial.println("Button 1 pressed!");

    if (displayState == 0)
    {
      displayCenteredText("Loading.");
      delay(1000);
      displayCenteredText("Loading..");
      delay(1000);
      displayCenteredText("Loading...");
      delay(1000);
      displayState = 1;
    }
    else if (displayState == 1)
    {
      displayCenteredText("Calculating.");
      delay(4000);
      displayCenteredText("Calculating..");
      delay(4000);
      displayCenteredText("Calculating...");
      delay(4000);

      initVoltammetryData();
      findPeaks();
      sendData("Merkuri");

      randomNumber = current_ppm;
      buffer[20];
      sprintf(buffer, "%.2f", randomNumber);
      displayState = 6;
      lastState = 1;
    }
    else if (displayState == 2)
    {
      displayCenteredText("Calculating.");
      delay(4000);
      displayCenteredText("Calculating..");
      delay(4000);
      displayCenteredText("Calculating...");
      delay(4000);

      initVoltammetryData();
      findPeaks();
      sendData("Timbal");

      randomNumber = current_ppm;
      buffer[20];
      sprintf(buffer, "%.2f", randomNumber);
      displayState = 6;
      lastState = 2;
    }
    else if (displayState == 3)
    {
      displayCenteredText("Calculating.");
      delay(4000);
      displayCenteredText("Calculating..");
      delay(4000);
      displayCenteredText("Calculating...");
      delay(4000);

      initVoltammetryData();
      findPeaks();
      sendData("Kadmium");

      randomNumber = current_ppm;
      buffer[20];
      sprintf(buffer, "%.2f", randomNumber);
      displayState = 6;
      lastState = 3;
    }
    else if (displayState == 4)
    {
      displayCenteredText("Calculating.");
      delay(4000);
      displayCenteredText("Calculating..");
      delay(4000);
      displayCenteredText("Calculating...");
      delay(4000);

      initVoltammetryData();
      findPeaks();
      sendData("Arsen");

      randomNumber = current_ppm;
      buffer[20];
      sprintf(buffer, "%.2f", randomNumber);
      displayState = 6;
      lastState = 4;
    }
    else if (displayState == 5)
    {
      displayCenteredText("Calculating.");
      delay(4000);
      displayCenteredText("Calculating..");
      delay(4000);
      displayCenteredText("Calculating...");
      delay(4000);

      initVoltammetryData();
      findPeaks();
      sendData("Microplastic");

      randomNumber = current_ppm;
      buffer[20];
      sprintf(buffer, "%.2f", randomNumber);
      displayState = 6;
      lastState = 5;
    }
    else if (displayState == 6)
    {
      displayState = lastState;
    }
    // Debounce delay
    delay(200);
  }

  // Check if button 2 is pressed (state changed from HIGH to LOW)
  if (button2State == LOW && lastButton2State == HIGH)
  {
    Serial.println("Button 2 pressed!");
    if (displayState == 1)
    {
      displayState = 2;
    }
    else if (displayState == 2)
    {
      displayState = 3;
    }
    else if (displayState == 3)
    {
      displayState = 4;
    }
    else if (displayState == 4)
    {
      displayState = 5;
    }
    else if (displayState == 5)
    {
      displayState = 1;
    }
    // Debounce delay
    delay(200);
  }

  // Save the current state as the last state for the next loop iteration
  lastButton1State = button1State;
  lastButton2State = button2State;
}

void displayCenteredText(const char *text)
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
  int16_t text_width = u8g2.getStrWidth(text);
  int16_t x = (SCREEN_WIDTH - text_width) / 2;
  int16_t y = (SCREEN_HEIGHT / 2);
  u8g2.setCursor(x, y);
  u8g2.print(text);
  u8g2.sendBuffer();
}

void displayCenteredText2(const char *line1, const char *line2)
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font

  // Display line 1
  int16_t text_width1 = u8g2.getStrWidth(line1);
  int16_t x1 = (SCREEN_WIDTH - text_width1) / 2;
  int16_t y1 = (SCREEN_HEIGHT / 2) - 10; // Place line 1 above center
  u8g2.setCursor(x1, y1);
  u8g2.print(line1);

  // Display line 2
  int16_t text_width2 = u8g2.getStrWidth(line2);
  int16_t x2 = (SCREEN_WIDTH - text_width2) / 2;
  int16_t y2 = (SCREEN_HEIGHT / 2) + 10; // Place line 2 below center
  u8g2.setCursor(x2, y2);
  u8g2.print(line2);

  u8g2.sendBuffer();
}