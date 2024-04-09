<?php

error_reporting(E_ALL);
ini_set('display_errors', 1);

// Enable CORS
header("Access-Control-Allow-Origin: *"); // Allow requests from any origin
header("Access-Control-Allow-Methods: GET, POST, OPTIONS"); // Allow GET, POST, and OPTIONS requests
header("Access-Control-Allow-Headers: Content-Type, Authorization"); // Allow specified headers
header("Access-Control-Max-Age: 86400"); // Cache preflight response for 24 hours

$servername = "127.0.0.1"; // Change this if your MySQL server is hosted elsewhere
$database = "trial";
$username = "rushi";
$password = "dragons369";

// Create connection
$conn = new mysqli($servername, $username, $password, $database);

// Check connection
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

$cow_entries = array(); // Define as a global variable

function handle_request($conn)
{
    global $cow_entries; // Access the global variable

    // Check if it's a POST request
    if ($_SERVER['REQUEST_METHOD'] === 'POST') {
        // Get the raw POST data
        $json_data = file_get_contents('php://input');

        // Decode the JSON data
        $data = json_decode($json_data, true);

        // Check if the 'function' field is set in the POST request data
        if (isset($data['function'])) {
            // Determine which function to call based on the 'function' field
            switch ($data['function']) {
                case 'gateway_registration':
                    gateway_registration($conn, $data);
                    break;
                case 'node_registration':
                    node_registration($conn, $data);
                    break;
                case 'publish_data':
                    publish_data($conn, $data);
                    break;
                default:
                    echo "Unknown function: " . $data['function'];
            }
        } else {
            echo "Error: 'function' field is missing in the POST request";
        }
    }
    // Check if it's a GET request
    elseif ($_SERVER['REQUEST_METHOD'] === 'GET') {
        // Check if the 'function' parameter is set in the GET request
        if (isset($_GET['function'])) {
            // Get the value of 'function' field from GET request
            $function_value = $_GET['function'];

            // Return the gateway ID if the function is 'get_gateway_id'
            if ($function_value === 'get_gateway_id') {
                // Check if the 'gateway_id' parameter is set
                if (isset($_GET['gateway_id'])) {
                    if (check_gateway_id($conn, $_GET['gateway_id'])) {
                        //echo json_encode(array("message" => "Gateway ID exists in the database"));
                        $cows_array = array();
                        $gateway_table = 'g' . $_GET['gateway_id'];

                        $sql_read = "SELECT * FROM $gateway_table";
                        $result = $conn->query($sql_read);

                        if ($result->num_rows > 0) {
                            // output data of each row
                            //echo $result->num_rows;

                            while ($row = $result->fetch_assoc()) {
                                //echo "ID: " . $row["node_mac"] . " - Name: " . $row["cow_name"] . "<br>";

                                $cow_table = $gateway_table . 'c' . $row["node_mac"];
                                $sql = "SELECT * FROM $cow_table ORDER BY time_stamp DESC LIMIT 1";

                                $result2 = $conn->query($sql);

                                if ($result2->num_rows > 0) {
                                    // Output data of each row
                                    while ($row2 = $result2->fetch_assoc()) {
                                        $cow_entry = array(
                                            "cow_mac" => $row["node_mac"],
                                            "cow_name" => $row["cow_name"],
                                            "latitude" => $row2["latitude"],
                                            "longitude" => $row2["longitude"],
                                            "time_stamp" => $row2["time_stamp"],
                                            "activity" => $row2["activity"]
                                        );
                                        array_push($cow_entries, $cow_entry);
                                    }
                                } else {
                                    ;
                                    // no cow data for this cow
                                }
                            }

                        } else {
                            // now cows
                            ;
                        }


                        $json_response = json_encode($cow_entries);

                        // Set the Content-Type header to application/json
                        header('Content-Type: application/json');

                        // Send the JSON response to the user
                        echo $json_response;
                    }
                } else {
                    // If 'gateway_id' parameter is missing, return an error message
                    echo json_encode(array("error" => "Error: 'gateway_id' parameter is missing in the request"));
                }
            }
            if ($function_value === 'get_cow_history') {

                if (isset($_GET['gateway_id']) && isset($_GET['cow_mac'])) {
                    $tablename = 'g' . $_GET['gateway_id'] . 'c' . $_GET['cow_mac'];
                    $sql = "SELECT * FROM $tablename ORDER BY time_stamp DESC";

                    $result2 = $conn->query($sql);

                    if ($result2->num_rows > 0) {
                        // Output data of each row
                        while ($row2 = $result2->fetch_assoc()) {
                            $cow_entry = array(
                                "latitude" => $row2["latitude"],
                                "longitude" => $row2["longitude"],
                                "time_stamp" => $row2["time_stamp"],
                                "activity" => $row2["activity"]
                            );
                            array_push($cow_entries, $cow_entry);
                        }
                        $json_response = json_encode($cow_entries);

                        // Set the Content-Type header to application/json
                        header('Content-Type: application/json');

                        // Send the JSON response to the user
                        echo $json_response;
                    } else {
                        ;
                        // no cow data for this cow
                    }


                } else {
                    // If 'gateway_id' parameter is missing, return an error message
                    echo json_encode(array("error" => "Error: parameters is/are missing in the request"));
                }


            } else {
                // If function is not 'get_gateway_id', return an error message
                echo json_encode(array("error" => "Error: Unsupported function"));
            }
        } else {
            // If 'function' field is not set, display an error message
            echo json_encode(array("error" => "Error: 'function' field is missing in the request"));
        }
    } else {
        // Unsupported HTTP method
        echo json_encode(array("error" => "Error: Unsupported HTTP method"));
    }
}

handle_request($conn);


function gateway_registration($conn, $jsonData1)
{
    echo "gateway registration now" . '<br>';
    echo "function: " . $jsonData1['function'] . '<br>';
    echo "received gateway id to publish: " . $jsonData1['gateway_mac'] . '<br>';
    echo "received user_id id to publish: " . $jsonData1['user_id'] . '<br>';

    $stmt = $conn->prepare("INSERT INTO GATEWAYS (gateway_mac, user_id) VALUES (?, ?)");
    $stmt->bind_param("si", $jsonData1['gateway_mac'], $jsonData1['user_id']);

    if ($stmt->execute() === TRUE) {
        echo "New record inserted successfully<br>";
    } else {
        echo "Error: " . $stmt->error . "<br>";
    }

    $tablename = 'g' . $jsonData1['gateway_mac'];
    $sql = "CREATE TABLE IF NOT EXISTS $tablename (
    node_mac VARCHAR(20) PRIMARY KEY,
    cow_name VARCHAR(30)
    )";

    // Prepare and execute the SQL statement
    if ($conn->query($sql) === TRUE) {
        echo "Table $tablename created successfully";
    } else {
        echo "Error creating table: " . $conn->error;
    }

}

function node_registration($conn, $jsondata2)
{
    echo "node registration" . '<br>';
    $tablename = 'g' . $jsondata2['gateway_mac'] . 'c' . $jsondata2['node_mac'];
    echo "to create " . $tablename . '<br>';
    $sql = "CREATE TABLE IF NOT EXISTS $tablename (
        latitude float,
        longitude float,
        time_stamp timestamp,
        activity int
    )";

    if ($conn->query($sql) === TRUE) {
        echo "Table $tablename created successfully";
    } else {
        echo "Error: " . $conn->error;
    }

    $tablename = 'g' . $jsondata2['gateway_mac'];
    $newCowName = "new_cow";
    $sql = "INSERT INTO $tablename (node_mac, cow_name) VALUES ('{$jsondata2['node_mac']}', '$newCowName')";

    if ($conn->query($sql) === TRUE) {
        echo "Record inserted successfully into table $tablename";
    } else {
        echo "Error inserting record into table $tablename: " . $conn->error;
    }
}

function publish_data($conn, $jsondata3)
{
    echo "function:" . $jsondata3['function'] . '<br>';
    echo "gateway mac:" . $jsondata3['gateway_mac'] . '<br>';

    foreach ($jsondata3['data'] as $data) {
        echo "Cow MAC: " . $data['cow_mac'] . "<br>";
        echo "Latitude: " . $data['latitude'] . "<br>";
        echo "Longitude: " . $data['longitude'] . "<br>";
        echo "Activity: " . $data['activity'] . "<br>";
        $currentTimestamp = date('Y-m-d H:i:s');
        $tablename = 'g' . $jsondata3['gateway_mac'] . 'c' . $data['cow_mac'];

        $sql = "INSERT INTO $tablename (latitude, longitude, time_stamp, activity) VALUES (?, ?, ?, ?)";
        $stmt = $conn->prepare($sql);
        $stmt->bind_param("ddsi", $data['latitude'], $data['longitude'], $currentTimestamp, $data['activity']);

        if ($stmt->execute() === TRUE) {
            echo "Data inserted into table $tablename successfully<br>";
        } else {
            echo "Error inserting data into table $tablename: " . $conn->error . "<br>";
        }

        $stmt->close();
    }
}

function check_gateway_id($conn, $gateway_id)
{
    $count = 0;

    // Prepare SQL statement to check if the gateway ID exists in the database
    $stmt = $conn->prepare("SELECT COUNT(*) FROM GATEWAYS WHERE gateway_mac = ?");
    $stmt->bind_param("s", $gateway_id);

    // Execute the prepared statement
    if ($stmt->execute()) {
        $stmt->bind_result($count);
        $stmt->fetch();
        $stmt->close();

        // Check if the count is greater than 0
        if ($count > 0) {
            return true;
        } else {
            echo json_encode(array("error" => "Gateway ID does not exist in the database"));
            return false;
        }
    } else {
        echo json_encode(array("error" => "Error executing SQL statement"));
        return false;
    }

}


$conn->close();
?>