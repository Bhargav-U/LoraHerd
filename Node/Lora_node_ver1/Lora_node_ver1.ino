#include "LORA.h"

void setup() {
  Serial.begin(115200);
  setuplora();
  randomSeed(1);
}

void loop() {

  double lat = random(1273, 1280) / 100.0;
  double lon = random(8019, 8020) / 100.0;
  int id = random(1, 6);


  // Delimiter
  String delim = "_";

  // Convert double values to String
  String latString = String(lat, 2);  // 2 decimal places
  String lonString = String(lon, 2);  // 2 decimal places
  String cow_id = String(id);

  // Concatenate the strings with the delimiter
  String resultString = latString + delim + lonString + delim + cow_id;


  // Print the result
  Serial.println(resultString);


  // Replace the uint8_t array with the generated string
  resultString.toCharArray(reinterpret_cast<char*>(buff), sizeof(buff));

  //send data
  send_data();
  Serial.println("Data sent");
  Serial.println();
  delay(10000);  //have a delay between packets
}
