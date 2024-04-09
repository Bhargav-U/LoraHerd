#ifndef GSM_H
#define GSM_H

#include <HardwareSerial.h>  //hardware serial for esp32


//method possible with gsm(http methods)
const char *actions[] = {
  "GET",
  "POST",
  "DELETE",
  "PUT",
  "PATCH"
};

//read what gsm responsds to at commands
void read_from_gsm() {
  Serial.println("gsm says:");
  // Print the response from the GSM module to the Serial Monitor
  String response = "";

  // Read and store the response until timeout or available characters
  long start = millis();
  while (!Serial2.available()) {
    delay(100);
    long time = millis();
    if (time - start > 30000) {  //just added a timeout f0r 20seconds
      Serial.println("Timeout");
      break;
    }
  }
  while (Serial2.available()) {
    char c = Serial2.read();
    response += c;
  }

  Serial.println(response);
}


//send http requests
void gsm_http_request(String url = "NULL", String content_type = "NULL", String action = "NULL", String data = "NULL") {
  // Check if essential arguments (url and action) are missing
  String response = "";

  String url_command = "AT+HTTPPARA=\"URL\"," + url;
  String content_type_command = "AT+HTTPPARA=\"CONTENT\"," + content_type;
  int data_size = data.length();
  int time_value = 3000;
  String data_to_post_command = "AT+HTTPDATA=" + String(data_size) + "," + String(time_value);

  if (url == "NULL" || action == "NULL") {
    Serial.println("Missing arguments, URL or action is missing");
  } else {
    bool wifiStatus = false;
    bool registrationStatus = false;
    Serial2.println("AT+HTTPTERM");
    read_from_gsm();
    //start http
    Serial2.println("AT+HTTPINIT");
    read_from_gsm();

    //set url
    Serial2.println(url_command);
    read_from_gsm();

    int method = 0;
    for (int i = 0; i < 5; i++) {
      if (strcmp(action.c_str(), actions[i]) == 0) {
        method = i;
        break;
      }
    }

    switch (method) {
      case 0:
        Serial2.println("AT+HTTPACTION=0");
        read_from_gsm();
        read_from_gsm();
        break;
      case 1:
        //set json
        Serial2.println(content_type_command);
        read_from_gsm();

        //ask for sneding data
        Serial2.println(data_to_post_command);
        read_from_gsm();


        //send data
        Serial2.println(data);
        read_from_gsm();

        //post
        Serial2.println("AT+HTTPACTION=1");
        read_from_gsm();
        read_from_gsm();
        break;
      case 2:
        //deleye
        Serial2.println("AT+HTTPACTION=3");
        read_from_gsm();
        read_from_gsm();
        break;
      default:
        Serial.println("invalid action");
        break;
    }

    //end
    Serial2.println("AT+HTTPTERM");
    read_from_gsm();
  }
}

void setupgsm(){
    Serial2.begin(115200, SERIAL_8N1, 16, 17);
    delay(5000);
}


#endif GSM_H
