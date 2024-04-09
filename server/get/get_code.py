import requests

def get_gateway(value, gateway_id):
    url = "http://localhost:8000"  # Adjust the URL based on your setup
    params = {
        'function': value,
        'gateway_id': gateway_id
    }
    response = requests.get(url, params=params)
    
    if response.status_code == 200:
        print("GET request sent successfully.")
        print("Response from server:")
        print(response.text)
    else:
        print(f"Error sending GET request. Status code: {response.status_code}")

def get_cow(fvalue, gateway_id, cwo_mac):
    url = "http://localhost:8000"  # Adjust the URL based on your setup
    params = {
        'function': fvalue,
        'gateway_id': gateway_id,
        'cow_mac' : cwo_mac
    }
    response = requests.get(url, params=params)
    
    if response.status_code == 200:
        print("GET request sent successfully.")
        print("Response from server:")
        print(response.text)
    else:
        print(f"Error sending GET request. Status code: {response.status_code}")

# Example usage:
#value_to_send = "get_gateway_id"
#get_gateway("get_gateway_id", "gateway_12345")
get_cow("get_cow_history", "gateway_12345", "C11")
