//the code
//temporray queu fix
#define MAX_QUEUE_LENGTH 10  // Define the maximum length of the queue

String messageQueue[MAX_QUEUE_LENGTH];
int front = 0;
int rear = -1;
bool stopReceiving = false;


#include <HardwareSerial.h>  //hardware serial for esp32

#include "LORA.h"  //include the setiings file, frequencies, LoRa settings etc
#include <WiFi.h>

//independent tasks
TaskHandle_t LoraTaskHandle;
TaskHandle_t CloudTaskHandle;
TaskHandle_t NodeRegisterTaskHandle;

const char *apSSID = "Gateway";
const char *apPassword = "LoraHeard";

const char *wifiSSID = "SSN";
const char *wifiPassword = "Ssn1!Som2@Sase3#";

#define registrationButton 32

String GATEWAYMAC = "";


char *Received_data = "";

String base_url = "https://firestore.googleapis.com/v1/projects/first-7333/databases/(default)/documents/locations?documentId=cow_";
String del_url = "https://firestore.googleapis.com/v1/projects/first-7333/databases/(default)/documents/locations?name=projects/first-7333/databases/(default)/documents/locations/cow_";
String contentType = "application/json\"";

const char *actions[] = {
  "GET",
  "POST",
  "DELETE",
  "PUT",
  "PATCH"
};

bool wifiStatus = false;
bool registrationStatus = false;

//need to remove this later
String process_data(String data = "") {
  if (data != "") {
    // Check if the queue is not full
    if ((rear - front + 1) < MAX_QUEUE_LENGTH) {
      // Add data to the global queue
      rear = (rear + 1) % MAX_QUEUE_LENGTH;
      messageQueue[rear] = data;

      // Check queue length
      if ((rear - front + 1) == MAX_QUEUE_LENGTH) {
        // If the queue is full, set the stopReceiving flag to true
        stopReceiving = true;
      }

      // Return an empty string to indicate successful addition of data
      return "";
    } else {
      // Return an indicator string to inform that data couldn't be added as the queue is full
      return "Queue full, data not added.";
    }
  } else {
    if (front <= rear) {
      // Retrieve data from the queue
      String retrievedData = messageQueue[front];
      front++;

      // Reset the stopReceiving flag to false if the queue is not full
      if ((rear - front + 1) < MAX_QUEUE_LENGTH) {
        stopReceiving = false;
      }

      // Return the retrieved data
      return retrievedData;
    } else {
      // Reset queue indices if the queue is empty
      front = 0;
      rear = -1;

      // Return an empty string to indicate that no data is available
      return "";
    }
  }
}

void registerNode() {
  registrationStatus = true;
  WiFi.softAP(apSSID, apPassword);
  while (!digitalRead(registrationButton)) {
    delay(1000);
    digitalWrite(2, HIGH);
  }
  WiFi.softAPdisconnect(true);
  registrationStatus = false;
}

void handleWiFi(int maxAttempts) {
  int attempts = 0;
  Serial.print("Connecting to WiFi");
  while (attempts < maxAttempts) {
    WiFi.begin(wifiSSID, wifiPassword);
    delay(500);
    Serial.print(".");
    attempts++;
    if (WiFi.status() == WL_CONNECTED) {
      break;
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    wifiStatus = true;
    Serial.println("connected to wifi");
  } else {
    wifiStatus = false;
    Serial.println("not connected to wifi");
  }
}


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
  // //uncommnet below to not use mac of node as the cow id
  // int id = random(1, 6);
  // cowID = String(id);
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


void loraReceiveTask(void *parameter) {
  while (true) {
    if (!stopReceiving) {
      RXPacketL = LT.receive(RXBUFFER, RXBUFFER_SIZE, 5000, WAIT_RX);  //wait for a packet to arrive with 60seconds (60000mS) timeout

      if (RXPacketL >= 48) {
        if (RXBUFFER) {
          Received_data = (char *)RXBUFFER;
        }

        String data = Received_data;

        process_data(data);

        int delimiter, delimiter_1, delimiter_2, delimiter_3, delimiter_4;

        delimiter = data.indexOf("_");
        delimiter_1 = data.indexOf("_", delimiter + 1);
        delimiter_2 = data.indexOf("_", delimiter_1 + 1);
        delimiter_3 = data.indexOf("_", delimiter_2 + 1);
        delimiter_4 = data.indexOf("_", delimiter_3 + 1);

        String gatewayID = data.substring(delimiter + 1, delimiter_1);
        String latitude = data.substring(delimiter_1 + 1, delimiter_2);
        String longitude = data.substring(delimiter_2 + 1, delimiter_3);
        String nodeID = data.substring(delimiter_3 + 1, delimiter_4);

        if (GATEWAYMAC == gatewayID) {

          //actual data from the nodes
          Serial.print("packet: ");
          Serial.println(Received_data);

          // Print formatted output to serial monitor
          Serial.println("Gateway ID: " + gatewayID);
          Serial.println("Latitude: " + latitude);
          Serial.println("Longitude: " + longitude);
          Serial.println("Node ID: " + nodeID);
        } else {
          //actual data from the nodes
          Serial.print("packet: ");
          Serial.println(Received_data);
          Serial.println("Gateway ID: " + gatewayID);
          Serial.println("Gateway mac:" + GATEWAYMAC);
          Serial.println("invalid packet");
        }
      }
      Serial.println("-------------------------------------------------------");
    } else {
      Serial.println("queue full");
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void cloudUpdateTask(void *parameter) {
  while (true) {
    String data = process_data();

    if (data != "") {
      int delimiter, delimiter_1, delimiter_2, delimiter_3, delimiter_4;

      delimiter = data.indexOf("_");
      delimiter_1 = data.indexOf("_", delimiter + 1);
      delimiter_2 = data.indexOf("_", delimiter_1 + 1);
      delimiter_3 = data.indexOf("_", delimiter_2 + 1);
      delimiter_4 = data.indexOf("_", delimiter_3 + 1);

      String gatewayID = data.substring(delimiter + 1, delimiter_1);
      String latitude = data.substring(delimiter_1 + 1, delimiter_2);
      String longitude = data.substring(delimiter_2 + 1, delimiter_3);
      String nodeID = data.substring(delimiter_3 + 1, delimiter_4);
      send_to_database(latitude, longitude, nodeID);
    }
    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
}

void nodeRegisterTask(void *parameter) {
  while (true) {

    if (!digitalRead(registrationButton)) {
      registerNode();
      digitalWrite(2, LOW);
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}



// button init
// mac read
// gsm serial(2) init
// lora init
void setup() {

  Serial.begin(115200);

  pinMode(registrationButton, INPUT);
  pinMode(2, OUTPUT);



  delay(1000);
  GATEWAYMAC = WiFi.macAddress();

  // in gsm:                        tx   rx
  //            bd rate       ??     rx  tx
  Serial2.begin(115200, SERIAL_8N1, 16, 17);

  Serial2.println("AT");
  read_from_gsm();

  // NEEDS WORK HERE !!
  setuplora();

  // Create tasks
  xTaskCreatePinnedToCore(loraReceiveTask, "loraReceive", 4096, NULL, 1, &LoraTaskHandle, 1);
  xTaskCreatePinnedToCore(cloudUpdateTask, "cloudUpdate", 2048, NULL, 2, &CloudTaskHandle, 0);
  xTaskCreatePinnedToCore(nodeRegisterTask, "nodeRegister", 2048, NULL, 3, &NodeRegisterTaskHandle, 0);
  // Start scheduler
  vTaskStartScheduler();
}


void loop() {
}
