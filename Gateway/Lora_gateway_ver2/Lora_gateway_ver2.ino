#include "LORA.h"  //include the setiings file, frequencies, LoRa settings etc

#ifdef ESP32
#include <HardwareSerial.h>  //hardware serial for esp32
#else
#include <SoftwareSerial.h>
SoftwareSerial Serial2(6, 5);  // RX, TX for GSM module
#endif


char* Received_data = "";

String base_url = "https://firestore.googleapis.com/v1/projects/first-7333/databases/(default)/documents/locations?documentId=cow_";
String del_url = "https://firestore.googleapis.com/v1/projects/first-7333/databases/(default)/documents/locations?name=projects/first-7333/databases/(default)/documents/locations/cow_";
String contentType = "application/json\"";

const char* actions[] = {
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
    if(time - start  > 60000){//just added a timeout f0r 60seconds
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
void http_request(String url = "NULL", String content_type = "NULL", String action = "NULL", String data = "NULL") {
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

//send data to database via http
void send_to_database(String cowLatitude, String cowLongitude, String cowID) {
  String url = "";
  String action = "";
  String json_data =
    "{"
    "\"fields\":{"
    "\"latitude\":{\"doubleValue\":"
    + cowLatitude + "},"
                    "\"longitude\":{\"doubleValue\":"
    + cowLongitude + "}"
                     "}"
                     "}";

  action = "DELETE";
  url = "\"" + del_url + cowID + "\"";
  http_request(url, contentType, action, json_data);

  action = "POST";
  url = "\"" + base_url + cowID + "\"";
  http_request(url, contentType, action, json_data);
}


void setup() {
  Serial.begin(115200);
#ifdef ESP32
  Serial2.begin(115200, SERIAL_8N1, 16, 17);
#else
  Serial2.begin(115200);
#endif
  setuplora();
}


void loop() {
  RXPacketL = LT.receive(RXBUFFER, RXBUFFER_SIZE, 60000, WAIT_RX);  //wait for a packet to arrive with 60seconds (60000mS) timeout

  if (RXPacketL == 0) {
    Serial.println(".");
  } else {
    if (RXBUFFER) {
      Received_data = (char*)RXBUFFER;
    }
  }
  Serial.println();

  //actual data from the nodes
  Serial.print("gps: ");
  Serial.println(Received_data);

  //variables collected from the nodes
  char* latitude;
  char* longitude;
  char* id;

  // Parsing the latitude
  latitude = strtok(Received_data, "_");
  String stringLatitude = String(latitude);

  // Parsing the longitude
  longitude = strtok(NULL, "_");
  String stringLongitude = String(longitude);

  // Parsing the ID
  id = strtok(NULL, "_");
  String stringID = String(id);

  // Printing the results
  Serial.print("Latitude: ");
  Serial.println(stringLatitude);
  Serial.print("Longitude: ");
  Serial.println(stringLongitude);
  Serial.print("ID: ");
  Serial.println(stringID);

  //sending data to cloud
  send_to_database(stringLatitude, stringLongitude, stringID);

  Serial.println("-----------");
}
