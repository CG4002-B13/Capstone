# Communications

Capstone AY25/26 S1

## Getting Started

Insert installation steps here

## To export data to ESP32

pio run -t uploadfs

## To Export Dev environment to ESP32

export $(grep -v '^#' .env | xargs)
pio run -e dev -t upload

## To Export Deploy environment to ESP32

export $(grep -v '^#' .env | xargs)
pio run -e deploy -t upload