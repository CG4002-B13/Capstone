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
./setup.sh -ip <ip-addr1>,<ip-addr2> -device devices -cn myDevices -dir ./deploy -dns <dns1>,<dns2> -san
# Multiple addresses / dns are optional
```
To generate for ESP32, execute the following command using the `setup.sh` script in `/secrets`:
```
./setup.sh -ip <broker-ip> -device esp32 -cn myESP32 -dir ./deploy
```
- Run `./setup -h` for more information on the script

A passwordfile needs to be generated for MQTT Server as well:
```
mosquitto_passwd -c <target-dir> <username>
```
- You might need to install mosquitto on your local machine using apt in order to run this command. Remember to disable it before starting the broker on docker compose.

The `<device>-ca.crt`, `<device>-server.crt`, `<device>-server.key` should be moved into the `/mosquitto/config` folder once generated.

## Ultra96
To run the python test MQTT client, first create a venv using pyenv:
```
pyenv virtualenv 3.10.4 capstone
pyenv activate capstone
```
Then run:
```
pip install -r requirements.txt
```
- For VSCode IDE to detect the interpreter, activate the venv, then run `code .` in root directory.

Required variables in `.env`:
```
MQTT_HOST=
MQTT_PORT=
MQTT_USER=
MQTT_PASS=
CERT_NAME= # in the format "<name>-"
MODE= # can be dev or deploy
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

## Golang Service
TBC

## Docker Compose
To facilitate development and deployment, the mosquitto MQTT broker is started together with the golang service within docker compose to minimise setup issues across machines. Simply run:
```
docker compose up -d 
# omit -d for logs
```

# FAQ
## Known Errors

- If running `setup.sh` gives the following errors, run the command to fix the script.
```
# Error
-bash: ./setup.sh: /bin/bash^M: bad interpreter: No such file or directory

# Fix
sed -i 's/\r$//' setup.sh
```