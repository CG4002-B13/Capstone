# CG4002

Capstone AY25/26 S1

This project consists of several components:

- [Communications](./comms/README.md)
- [Hardware](./hardware/README.md)
- [HWAI](./hwai/README.md)
- [Visualiser](./visualiser/README.md)

## Getting Started

Insert installation steps here

## To export data

pio run -t uploadfs

## To Export Dev

export $(grep -v '^#' .env | xargs)
pio run -e dev -t upload
