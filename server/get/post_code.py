import requests
import json

def send_post_request(json_data):
    url = "http://localhost:8000"  # Adjust the URL based on your setup

    # Convert the JSON data to a string
    json_string = json.dumps(json_data)

    # Set the Content-Type header to application/json
    headers = {'Content-Type': 'application/json'}

    # Send the POST request with JSON data
    response = requests.post(url, data=json_string, headers=headers)

    # Print the response from the server
    if response.status_code == 200:
        print("POST request sent successfully.")
        print("Response from server:")
        print(response.text)
    else:
        print(f"Error sending POST request. Status code: {response.status_code}")

# Example usage:

jsonData1 = {
        "function" : "gateway_registration" ,
        "user_id" : 1112,
        "gateway_mac" : "E86BEACFC3E5"
    }
send_post_request(jsonData1)
