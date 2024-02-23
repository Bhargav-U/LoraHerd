#ifndef GSM_H
#define GSM_H

#define baudrate 115200

// Check for the target platform
#ifdef ESP32
#include <HardwareSerial.h>
HardwareSerial GSM(2);
void setupgsm() {
  GSM.begin(baudrate, SERIAL_8N1, 16, 17);
}
#else  // It will be Arduino
#include <SoftwareSerial.h>
#define RX_PIN 5
#define TX_PIN 6
SoftwareSerial GSM(5, 6);
void setupgsm() {
  GSM.begin(baudrate);
}
#endif


// Function to read response from the GSM module
String read_from_gsm() {
  Serial.println("GSM says:");
  // Print the response from the GSM module to the Serial Monitor
  String response = "";

  // Read and store the response until timeout or available characters
  if (GSM.available() > 0) {
    response = GSM.readStringUntil('\n');
  }

  return response;
}

// Function to take user input to control the GSM module
void user_input() {
  Serial.println("Enter the AT commands:");
  while (Serial.available() == 0) {}

  // Read user input from Serial Monitor until newline character
  String userInput = Serial.readStringUntil('\n');
  // Send user input to GSM module
  GSM.println(userInput);

  // Print the response from the GSM module
  Serial.println(read_from_gsm());
}

// HTTP service function
void http_request(String url = "NULL", String content_type = "NULL", String action = "NULL", String data = "NULL") {
  // Check if essential arguments (url and action) are missing
  String response = "";

  GSM.println("AT+HTTPTERM");
  delay(100);

  if (url == "NULL" || action == "NULL") {
    Serial.println("Missing arguments, URL or action is missing");
  } else {
    Serial.print("Checking GSM:");
    GSM.println("AT");
    response = read_from_gsm();
    if (response != "OK") {
      Serial.println("GSM not responding");
      Serial.println(response);
    } else {
      // Variables to store data length for HTTP request
      int data_length = 0;

      // Prepare AT command strings for HTTP service
      url = "AT+HTTPPARA=\"URL\"," + url;
      content_type = "AT+HTTPPARA=\"CONTENT\"," + content_type;
      data_length = data.length();
      String download_request = "AT+HTTPDATA=" + String(data_length) + "," + "3000";

      Serial.print("HTTP service initiation:");
      GSM.println("AT+HTTPINIT");
      response = read_from_gsm();
      if (response == "OK") {
        Serial.println("Success");
      } else {
        Serial.println(response);
      }

      Serial.print("Calling for the URL:");
      GSM.println(url);
      response = read_from_gsm();
      if (response == "OK") {
        Serial.println("Success");
      } else {
        Serial.println(response);
      }

      Serial.println("Setting parameters");
      Serial.println("Setting content type:");
      GSM.println(content_type);
      response = read_from_gsm();
      if (response == "OK") {
        Serial.println("Success");
      } else {
        Serial.println(response);
      }

      // Check if the action is a GET request
      if (action == "GET") {
        Serial.println("Sending GET request:");
        GSM.println("AT+HTTPACTION=0");
        response = read_from_gsm();
        response = read_from_gsm();
        int firstComma = response.indexOf(',');
        int secondComma = response.indexOf(',', firstComma + 1);
        String actionCode = response.substring(firstComma + 1, secondComma);
        int actionCodeInt = actionCode.toInt();
        if(actionCodeInt == 200){
          Serial.println("get request sucessful");
        }else{
          Serial.println("Response: " + actionCode);
        }
      }

      // Check if the action is a POST request
      if (action == "POST") {
        Serial.println("Sending POST request:");
        GSM.println(download_request);
        delay(300);
        GSM.println(data);
        GSM.println("AT+HTTPACTION=1");
        response = read_from_gsm();
        int firstComma = response.indexOf(',');
        int secondComma = response.indexOf(',', firstComma + 1);
        String actionCode = response.substring(firstComma + 1, secondComma);
        int actionCodeInt = actionCode.toInt();
        if(actionCodeInt == 200){
          Serial.println("post request sucessful");
        }else{
          Serial.println("Response: " + actionCode);
        }
      }

      Serial.println("Terminating the HTTP service:");
      GSM.println("AT+HTTPTERM");
      response = read_from_gsm();
      if (response == "OK") {
        Serial.println("Success");
      } else {
        Serial.println(response);
      }
    }
  }
}

#endif  GSM_H
