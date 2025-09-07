# Communications

Capstone AY25/26 S1

## Getting Started

### Prerequisites
- Access to Bash / Linux Machine
- Python 3.10.4
- PlatformIO core
- Docker and Docker Compose

To setup locally, there are a few requirements needed: </br>
1. Static / Known IP address of the machine that is hosting the MQTT broker </br>
2. Generation of self-signed CA, client and server key and cert pairs.</br>

The MQTT Broker is configured to alow TLS connection for both ports 8883 (Ultra96) and 8884 (Firebeetle). Each connection requires its own set of self signed certificate. </br>

To generate for Ultra96, execute the following command using the `setup.sh` script in `/secrets`:

```
./setup.sh -device ultra96 -ip <address_of_broker> -cn myUltra96 -san
```
To generate for ESP32, execute the following command using the `setup.sh` script in `/secrets`:
```
./setup.sh -device esp32 -ip <address_of_broker> -cn myESP32
```
- Run `./setup -h` for more information on the script

The `<device>-ca.crt`, `<device>-server.crt`, `<device>-server.key` should be moved into the `/mosquitto/config` folder once generated.

## Ultra96
To run the python test MQTT client, first create a local .venv:
```
python -m venv .venv
source .venv/bin/activate
```
Then run:
```
pip install -r requirements.txt
```

## ESP32
To allow dynamic importing of certificate files into the ESP32, PlatformIO was used to allow ease of storing files on the ESP32 via SPIFFS. PlatformIO also allows the usage of environment variables.

Required variables in `.env`:
```
WIFI_SSID=
WIFI_PASS=
MQTT_USER=
MQTT_PASS=

DEV_MQTT_SERVER=
DEV_MQTT_PORT=

DEPLOY_MQTT_SERVER=
DEPLOY_MQTT_PORT=
```


### To export data to ESP32
After running `setup.sh`, the following files `esp32-ca.crt`, `esp32-client.crt`, `esp32-client.key` should have been generated. Copy these 3 files into `esp32/data`, then run:
```
pio run -t uploadfs
```

### To Export Dev environment to ESP32
```
export $(grep -v '^#' .env | xargs)
pio run -e dev -t upload
```

### To Export Deploy environment to ESP32
```
export $(grep -v '^#' .env | xargs)
pio run -e deploy -t upload
```

## MQTT Broker
To facilitate development and deployment, the mosquitto MQTT broker is started within docker compose to minimise setup issues across machines. Simply run:
```
docker compose up -d 
# omit -d for logs
```