//the code
#include "LORA.h"  //include the setiings file, frequencies, LoRa settings etc
#include "GSM.h"   //include this for gsm

#include <WiFi.h>
#include <HTTPClient.h>


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
const char *wifiSSID = "SSN";
const char *wifiPassword = "Ssn1!Som2@Sase3#";

//button and other variables
#define registrationButton 32

String GATEWAYMAC = "";


char *Received_data = "";

//urls and content type
//this should be dynamic based on teh suers account,need a lot of work here
//base idea is twi haev 2 urs,oe is company server url and teh other is user instance url
//initally node conencts to teh companys ervera nd later afetr adding it to teh user saccount i pairs to teh uers server

//local url ,find a way to chnage it without having to change teh code againa nd again
String url = "http://10.17.27.94:8000/";

//this is not needed for thsi case,as the gsm funtion is hgenarlized ,we are justa dding this
String contentType = "application/json\"";




//flags for determinign what communication module to use,gsm or wifi
bool wifiStatus = false;
//flag to underatsnd registartion stae of eh node
bool registrationStatus = false;

// Dynamic array to store node data
NodeData *nodeDataArray;
const int maxNodes = 200;  // Maximum number of nodes //this should be abased on testung with multiple nodes,
int arraySize = 0;         // Current size of the array


// Function to send data to server via POST request as JSON
void wifi_http_request(String url, String json_data) {
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
    //need to remove thso later on,
    Serial.println("Maximum number of nodes reached. Cannot add more nodes.");
  }
}


// Function to return node data as JSON string
String getNodeDataAsJson() {
  // Initialize the JSON string
  String jsonString = "{\"gateway_id\":\"" + GATEWAYMAC ;


  jsonString +=  "";

  // Add details of each node as a JSON object
  for (int i = 0; i < arraySize; i++) {
    jsonString += ",\"nodes\":[ + {\"node_mac\":\"" + nodeDataArray[i].node_mac + "\",\"longitude\":\"" + nodeDataArray[i].longitude + "\",\"latitude\":\"" + nodeDataArray[i].latitude + "\",\"activity\":\"" + nodeDataArray[i].activity + "\"}";

    // Add comma if it's not the last element
    if (i < arraySize - 1) {
      jsonString += ",";
    }
  }

  // Close the JSON array
  jsonString += "]}";

  return jsonString;
}

//registre node,when button is pressed,as longa button si pressedw ecan register the node
void registerNode() {
  registrationStatus = true;
  WiFi.disconnect();
  wifiStatus = false;

  WiFi.softAP(apSSID, apPassword);
  while (!digitalRead(registrationButton)) {
    delay(1000);
    digitalWrite(2, HIGH);
  }
  WiFi.softAPdisconnect(true);
  handleWiFi();
  registrationStatus = false;
}

//to conet o wifi,try for five times and if pssoible connect if not dont connect,afetr connection set teh status to true
void handleWiFi(int maxAttempts = 5) {
  if (WiFi.status() != WL_CONNECTED &&  !registrationStatus) {
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
  if(registrationStatus){
    WiFi.disconnect();
  }
}



//send data to da      "application/json\""tabase via http
void send_to_database() {
  //paylaod is teh node table
  String json_data = getNodeDataAsJson();
  String action = "POST";

  //if wifi is connecte duse wifi to send daat else use gsm to send data
  if (wifiStatus && !registrationStatus) {
    wifi_http_request(url, getNodeDataAsJson());  //send data via wifi
  } 
  //else {
  //   gsm_http_request(url, contentType, action, getNodeDataAsJson());  //send data via gsm
  // }
}


//recvibe steh lora paclets and ten makes the ndoe tabel
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

//sending teh node  atbel to the server
void cloudUpdateTask(void *parameter) {
  while (true) {
    handleWiFi();
    send_to_database();
    vTaskDelay(5000 / portTICK_PERIOD_MS);  //increase the delay
  }
}

//regsistartion task
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

  //intitally connect to wifi
  handleWiFi();

  //setup the registrtion button and indicaator led
  pinMode(registrationButton, INPUT);
  pinMode(2, OUTPUT);

  // Initialize the dynamic array
  nodeDataArray = new NodeData[maxNodes];


  //get the gateway id
  GATEWAYMAC = WiFi.macAddress();


  // NEEDS WORK HERE !!
  //setup teh spui to laora module
  setuplora();

  //setup te gsm module
  setupgsm();

  // Create tasks
  //this handles receivng the data form teh lora recivera nd then maing te node table
  xTaskCreatePinnedToCore(loraReceiveTask, "loraReceive", 4096, NULL, 1, &LoraTaskHandle, 1);
  //this will take teh node tael and then uplaod it to the sever
  xTaskCreatePinnedToCore(cloudUpdateTask, "cloudUpdate", 2048, NULL, 2, &CloudTaskHandle, 0);
  //this will be active when the button is pressed or registration of a node
  xTaskCreatePinnedToCore(nodeRegisterTask, "nodeRegister", 2048, NULL, 3, &NodeRegisterTaskHandle, 0);
  // Start scheduler
  vTaskStartScheduler();
}


void loop() {
}
