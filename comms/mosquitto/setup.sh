#!/bin/bash

# Default values
DEFAULT_IP="127.0.0.1"
DEFAULT_CN="localClient"
DEFAULT_DIR="./config/certs"

# Initialize variables with defaults
IP_ADDRESS="$DEFAULT_IP"
CLIENT_CN="$DEFAULT_CN"
CERT_DIR="$DEFAULT_DIR"

# Function to display usage
usage() {
    echo "Usage: $0 [-ip <ip_address>] [-cn <common_name>] [-dir <directory>]"
    echo "  -ip <ip_address>   : IP address for the server certificate (default: $DEFAULT_IP)"
    echo "  -cn <common_name>  : Common name for the client certificate (default: $DEFAULT_CN)"
    echo "  -dir <directory>   : Directory to store certificates (default: $DEFAULT_DIR)"
    echo "  -h, --help         : Display this help message"
    echo ""
    echo "Examples:"
    echo "  $0                                    # Use defaults"
    echo "  $0 -ip 192.168.1.100                 # Custom IP, default client CN and dir"
    echo "  $0 -cn myClient                      # Default IP, custom client CN, default dir"
    echo "  $0 -dir /etc/ssl/mqtt                # Default IP and CN, custom directory"
    echo "  $0 -ip 10.0.0.5 -cn sensor1 -dir ./certs  # All custom values"
}

# Parse command line arguments
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

# Display configuration
echo "=== SSL Certificate Generation ==="
echo "Server IP Address: $IP_ADDRESS"
echo "Client Common Name: $CLIENT_CN"
echo "Certificate Directory: $CERT_DIR"
echo "=================================="
echo ""

# Create directory structure
mkdir -p "$CERT_DIR"
cd "$CERT_DIR"

echo "1. Generating CA certificate..."
# 1. Certificate Authority (CA)
openssl genrsa -out ca.key 2048
openssl req -x509 -new -nodes -key ca.key -days 3650 -out ca.crt -sha256 \
-subj "/C=SG/ST=Singapore/L=Singapore/O=NUS/OU=MQTT/CN=mqttCA"

echo "2. Generating server certificate for IP: $IP_ADDRESS..."
# 2. Server/Broker Certificate
openssl genrsa -out server.key 2048
openssl req -new -key server.key -out server.csr \
-subj "/C=SG/ST=Singapore/L=Singapore/O=NUS/OU=MQTT/CN=$IP_ADDRESS"
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
-out server.crt -days 365 -sha256

echo "3. Generating client certificate with CN: $CLIENT_CN..."
# 3. Client Certificate
openssl genrsa -out client.key 2048
openssl req -new -key client.key -out client.csr \
-subj "/C=SG/ST=Singapore/L=Singapore/O=NUS/OU=MQTT/CN=$CLIENT_CN"
openssl x509 -req -in client.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
-out client.crt -days 365 -sha256

# Clean up CSR files
rm server.csr client.csr

echo ""
echo "=== Certificate Generation Complete ==="
echo "Generated files in $CERT_DIR/:"
echo "  - ca.crt, ca.key (Certificate Authority)"
echo "  - server.crt, server.key (Server certificate for $IP_ADDRESS)"
echo "  - client.crt, client.key (Client certificate for $CLIENT_CN)"
echo "========================================"