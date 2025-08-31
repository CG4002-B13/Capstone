# Communications

Capstone AY25/26 S1

## Getting Started

To setup locally, there are a few requirements needed:
1. Static / Known IP address of the machine that is hosting the MQTT broker
2. Generation of self-signed CA, client and server key and cert pairs.

## To export data to ESP32

pio run -t uploadfs

## To Export Dev environment to ESP32

export $(grep -v '^#' .env | xargs)
pio run -e dev -t upload

## To Export Deploy environment to ESP32

export $(grep -v '^#' .env | xargs)
pio run -e deploy -t upload