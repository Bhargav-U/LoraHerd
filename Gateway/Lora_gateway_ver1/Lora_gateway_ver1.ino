#include "LORA.h"  //include the setiings file, frequencies, LoRa settings etc
#include "GSM.h"//gsm 

char* gps = "";

const char* base_url = "https://firestore.googleapis.com/v1/projects/first-7333/databases/(default)/documents/locations";
const char* contentType = "application/json";

void send_to_database(String stringLatitude,String stringLongitude,String stringID) {

  String json_data = 
  "{"
    "\"fields\":{"
      "\"latitude\":{\"doubleValue\":" + stringLatitude + "},"
      "\"longitude\":{\"doubleValue\":" + stringLongitude +"}"
    "}"
  "}";
  
  String url = base_url;
  String action = "POST";

  http_request(url,contentType,action,json_data);
}

void setup() {
  //start teh serial communication
  Serial.begin(baudrate);
  // Start serial communication with GSM module
  setupgsm();
  //start the lora module
  setuplora();
}


void loop() {
  //receive the data from nodes
  gps = receive_lora_data();
  delay(1000);

  //actual data from the nodes
  Serial.print("gps: ");
  Serial.println(gps);

  //variables collected from the nodes
  char* latitude;
  char* longitude;
  char* id;

  // Parsing the latitude
  latitude = strtok(gps, "_");
  String stringLatitude = String(latitude);

  // Parsing the longitude
  longitude = strtok(NULL, "_");
  String stringLongitude = String(longitude);

  // Parsing the ID
  id = strtok(NULL, "_");
  String stringID = "cow_" + String(id);

  // Printing the results
  Serial.print("Latitude: ");
  Serial.println(stringLatitude);
  Serial.print("Longitude: ");
  Serial.println(stringLongitude);
  Serial.print("ID: ");
  Serial.println(stringID);

  send_to_database(stringLatitude, stringLongitude, stringID);

  Serial.println("-----------");
}