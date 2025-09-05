#!/bin/bash

# Default values
DEFAULT_IP="127.0.0.1"
DEFAULT_DIR="./"
DEFAULT_DEVICE=""
DEFAULT_CN="myCN"

IP_ADDRESS="$DEFAULT_IP"
CERT_DIR="$DEFAULT_DIR"
DEVICE_PREFIX="$DEFAULT_DEVICE"
CLIENT_CN=""

if [[ -d "$CERT_DIR" && $(ls -A "$CERT_DIR") ]]; then
    read -p "The directory '$CERT_DIR' already contains files. Do you want to clear existing certificates? [y/N]: " CONFIRM
    case "$CONFIRM" in
        [yY][eE][sS]|[yY])
            echo "Clearing existing certificates in $CERT_DIR"
            rm -rf "$CERT_DIR"/*.key "$CERT_DIR"/*.crt
            ;;
        *)
            echo "Exiting. Existing certificates were not cleared."
            exit 0
            ;;
    esac
fi

usage() {
    echo "Usage: $0 [-ip <ip_address>] [-cn <common_name>] [-dir <directory>] [-device <device_name>]"
    echo "  -ip <ip_address>   : IP address for the server certificate (default: $DEFAULT_IP)"
    echo "  -cn <common_name>  : Common name for the client certificate (default: $DEFAULT_CN)"
    echo "  -dir <directory>   : Directory to store certificates (default: $DEFAULT_DIR)"
    echo "  -device <name>     : Device Prefix to store certificates (default: $DEFAULT_DEVICE)"
    echo "  -h, --help         : Display this help message"
    echo ""
    echo "Examples:"
    echo "  $0                                   # Use defaults"
    echo "  $0 -ip 192.168.1.100                 # Custom IP, default client CN and dir"
    echo "  $0 -cn myClient                      # Default IP, custom client CN, default dir"
    echo "  $0 -dir /etc/ssl/mqtt                # Default IP and CN, custom directory"
    echo "  $0 -device esp32"
    echo "  $0 -device ultra96 -ip 10.0.0.5 -cn myUltra96 -dir ./certs"
}

while [[ $# -gt 0 ]]; do
    case $1 in
        -ip)
            IP_ADDRESS="$2"
            shift 2
            ;;
        -cn)
            CLIENT_CN="$2"
            shift 2
            ;;
        -dir)
            CERT_DIR="$2"
            shift 2
            ;;
        -device)
            DEVICE_PREFIX="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

if [[ -z "$CLIENT_CN" && -n "$DEVICE_PREFIX" ]]; then
    CLIENT_CN="my-${DEVICE_PREFIX}"
fi

echo "=== SSL Certificate Generation ==="
echo "Device Prefix: $DEVICE_PREFIX"
echo "Server IP Address: $IP_ADDRESS"
echo "Client Common Name: $CLIENT_CN"
echo "Certificate Directory: $CERT_DIR"
echo "=================================="
echo ""

mkdir -p "$CERT_DIR"
cd "$CERT_DIR"

CA_KEY="${DEVICE_PREFIX}-ca.key"
CA_CRT="${DEVICE_PREFIX}-ca.crt"
SERVER_KEY="${DEVICE_PREFIX}-server.key"
SERVER_CRT="${DEVICE_PREFIX}-server.crt"
CLIENT_KEY="${DEVICE_PREFIX}-client.key"
CLIENT_CRT="${DEVICE_PREFIX}-client.crt"

echo "1. Generating CA certificate..."
openssl genrsa -out "$CA_KEY" 2048
openssl req -x509 -new -nodes -key "$CA_KEY" -days 3650 -out "$CA_CRT" -sha256 \
-subj "/C=SG/ST=Singapore/L=Singapore/O=NUS/OU=MQTT/CN=${DEVICE_PREFIX}-CA"

echo "2. Generating server certificate for IP: $IP_ADDRESS..."
openssl genrsa -out "$SERVER_KEY" 2048
openssl req -new -key "$SERVER_KEY" -out server.csr \
-subj "/C=SG/ST=Singapore/L=Singapore/O=NUS/OU=MQTT/CN=$IP_ADDRESS"
openssl x509 -req -in server.csr -CA "$CA_CRT" -CAkey "$CA_KEY" -CAcreateserial \
-out "$SERVER_CRT" -days 365 -sha256


echo "3. Generating client certificate with CN: $CLIENT_CN..."
openssl genrsa -out "$CLIENT_KEY" 2048
openssl req -new -key "$CLIENT_KEY" -out client.csr \
-subj "/C=SG/ST=Singapore/L=Singapore/O=NUS/OU=MQTT/CN=$CLIENT_CN"
openssl x509 -req -in client.csr -CA "$CA_CRT" -CAkey "$CA_KEY" -CAcreateserial \
-out "$CLIENT_CRT" -days 365 -sha256

rm *.csr *.srl

echo ""
echo "=== Certificate Generation Complete ==="
echo "Generated files in $CERT_DIR/:"
echo "  - ca.crt, ca.key (Certificate Authority)"
echo "  - server.crt, server.key (Server certificate for $IP_ADDRESS)"
echo "  - client.crt, client.key (Client certificate for $CLIENT_CN)"
echo "========================================"