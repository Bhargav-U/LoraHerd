//the code
#include <HardwareSerial.h>  //hardware serial for esp32

#include "LORA.h"  //include the setiings file, frequencies, LoRa settings etc
#include <WiFi.h>
#include <HTTPClient.h>

#include <Arduino.h>



// Structure to hold node data
struct NodeData {
  String node_mac;
  String longitude;
  String latitude;
  String activity;
};

//independent tasks
TaskHandle_t LoraTaskHandle;
TaskHandle_t CloudTaskHandle;
TaskHandle_t NodeRegisterTaskHandle;

//cnsatnst for the softap
const char *apSSID = "Gateway";
const char *apPassword = "LoraHeard";

//wifi credentials for futre use,dual mode wifi and gsm
const char *wifiSSID= "SSN";
const char *wifiPassword= "Ssn1!Som2@Sase3#";

//button and other variables
#define registrationButton 32

String GATEWAYMAC = "";


char *Received_data = "";

//urls and content type
String url = "http://10.17.27.94:8000/";
String contentType = "application/json\"";





//method possible with gsm(http methods)
const char *actions[] = {
  "GET",
  "POST",
  "DELETE",
  "PUT",
  "PATCH"
};


//flags for future
bool wifiStatus = false;
bool registrationStatus = false;

// Dynamic array to store node data
NodeData *nodeDataArray;
const int maxNodes = 200;  // Maximum number of nodes
int arraySize = 0;         // Current size of the array


// Function to send data to server via POST request as JSON
void send_to_server(String url, String json_data) {
  // Initialize HTTPClient object
  HTTPClient http;

  // Set up headers
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  // Send POST request
  int httpResponseCode = http.POST(json_data);

  // Check for response
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String response = http.getString();
    Serial.println(response);
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  // End HTTP connection
  http.end();
}

// Function to update node data
void updateNodeData(String node_mac, String longitude, String latitude, String activity) {
  if (arraySize < maxNodes) {
    bool found = false;

    // Iterate through the array to check if the node already exists
    for (int i = 0; i < arraySize; i++) {
      if (nodeDataArray[i].node_mac == node_mac) {
        // Update existing node data
        nodeDataArray[i].longitude = longitude;
        nodeDataArray[i].latitude = latitude;
        nodeDataArray[i].activity = activity;
        found = true;
        break;
      }
    }

    // If node doesn't exist, add a new entry
    if (!found) {
      // Add new data to the end of the array
      nodeDataArray[arraySize].node_mac = node_mac;
      nodeDataArray[arraySize].longitude = longitude;
      nodeDataArray[arraySize].latitude = latitude;
      nodeDataArray[arraySize].activity = activity;
      // Increment array size
      arraySize++;
    }
  } else {
    Serial.println("Maximum number of nodes reached. Cannot add more nodes.");
  }
}


// Function to return node data as JSON string
String getNodeDataAsJson() {
  // Initialize the JSON string
  String jsonString = "{\"gateway_id\":\"" + GATEWAYMAC + "\",\"node_count\":" + String(arraySize) + ",\"memory_used\":" + String(maxNodes * sizeof(NodeData)) + ",\"free_memory\":" + String(ESP.getFreeHeap()) + ",\"nodes\":[";

  // Add details of each node as a JSON object
  for (int i = 0; i < arraySize; i++) {
    jsonString += "{\"node_mac\":\"" + nodeDataArray[i].node_mac + "\",\"longitude\":\"" + nodeDataArray[i].longitude + "\",\"latitude\":\"" + nodeDataArray[i].latitude + "\",\"activity\":\"" + nodeDataArray[i].activity + "\"}";

    // Add comma if it's not the last element
    if (i < arraySize - 1) {
      jsonString += ",";
    }
  }

  // Close the JSON array
  jsonString += "]}";

  return jsonString;
}

//registre node,when button is pressed
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

//to conet o wifi
void handleWiFi(int maxAttempts = 5) {
  if (WiFi.status() != WL_CONNECTED) {
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
void send_to_database() {

  String url = "";
  String action = "";
  String json_data = getNodeDataAsJson();


  action = "POST";
  http_request(url, contentType, action, json_data);
}


void loraReceiveTask(void *parameter) {
  while (true) {

    RXPacketL = LT.receive(RXBUFFER, RXBUFFER_SIZE, 5000, WAIT_RX);  //wait for a packet to arrive with 60seconds (60000mS) timeout

    if (RXPacketL >= 48) {
      if (RXBUFFER) {
        Received_data = (char *)RXBUFFER;
      }

      String data = Received_data;

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

        Serial.println("data updated in the table");
        updateNodeData(nodeID, latitude, longitude, "status_ok");

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
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void cloudUpdateTask(void *parameter) {
  while (true) {
    //send_to_database();
    handleWiFi();
    send_to_server(url, getNodeDataAsJson());
    vTaskDelay(5000 / portTICK_PERIOD_MS);//increase the delay
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

  handleWiFi();

  pinMode(registrationButton, INPUT);
  pinMode(2, OUTPUT);

  // Initialize the dynamic array
  nodeDataArray = new NodeData[maxNodes];


  delay(1000);
  GATEWAYMAC = WiFi.macAddress();

  // in gsm:                        tx   rx
  //            bd rate       ??     rx  tx
  Serial2.begin(115200, SERIAL_8N1, 16, 17);

  // Serial2.println("AT");
  // read_from_gsm();

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
