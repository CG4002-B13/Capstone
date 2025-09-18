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
SAN_ENABLED=false
CUSTOM_DNS=""

usage() {
    echo "Usage: $0 [-ip <ip_address>] [-cn <common_name>] [-dir <directory>] [-device <device_name>] [-san]"
    echo "  -ip <ip_address>   : IP address for the server certificate (default: $DEFAULT_IP)"
    echo "  -cn <common_name>  : Common name for the client certificate (default: $DEFAULT_CN)"
    echo "  -dir <directory>   : Directory to store certificates (default: $DEFAULT_DIR)"
    echo "  -device <name>     : Device Prefix to store certificates (default: $DEFAULT_DEVICE)"
    echo "  -san               : Add Subject Alternative Name with the given IP and localhost DNS"
    echo "  -h, --help         : Display this help message"
    echo ""
    echo "Examples:"
    echo "  $0                                   # Use defaults, CN only"
    echo "  $0 -ip 192.168.1.100                 # Custom IP in CN only"
    echo "  $0 -cn myClient                      # Default IP, custom client CN"
    echo "  $0 -dir /etc/ssl/mqtt                # Default IP and CN, custom directory"
    echo "  $0 -device esp32"
    echo "  $0 -device ultra96 -ip 10.0.0.5 -cn myUltra96 -dir ./certs -san  # With SAN extension"
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
        -san)
            SAN_ENABLED=true
            shift 1
            ;;
        -dns)
            CUSTOM_DNS="$2"
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
echo "SAN Extension: $SAN_ENABLED"
if [ "$SAN_ENABLED" = true ] && [ -n "$CUSTOM_DNS" ]; then
    echo "Custom DNS Names: $CUSTOM_DNS"
fi
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

if [ "$SAN_ENABLED" = true ]; then
    echo "Adding SAN extension with IP: $IP_ADDRESS"
    
    # Start building the SAN configuration
    cat > san.cnf <<EOF
[ v3_req ]
subjectAltName = @alt_names

[ alt_names ]
IP.1 = $IP_ADDRESS
EOF

    # Add custom DNS names if provided
    if [ -n "$CUSTOM_DNS" ]; then
        echo "Adding custom DNS names: $CUSTOM_DNS"
        IFS=',' read -ra DNS_ARRAY <<< "$CUSTOM_DNS"
        dns_counter=1
        for dns_name in "${DNS_ARRAY[@]}"; do
            # Trim whitespace
            dns_name=$(echo "$dns_name" | xargs)
            echo "DNS.$dns_counter = $dns_name" >> san.cnf
            ((dns_counter++))
        done
    else
        # Default to localhost if no custom DNS provided
        echo "DNS.1 = localhost" >> san.cnf
        echo "Adding default DNS: localhost"
    fi

    openssl x509 -req -in server.csr -CA "$CA_CRT" -CAkey "$CA_KEY" -CAcreateserial \
    -out "$SERVER_CRT" -days 365 -sha256 -extfile san.cnf -extensions v3_req
    rm san.cnf
else
    echo "Using CN only (no SAN extension)"
    openssl x509 -req -in server.csr -CA "$CA_CRT" -CAkey "$CA_KEY" -CAcreateserial \
    -out "$SERVER_CRT" -days 365 -sha256
fi

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
echo "  - ${DEVICE_PREFIX}-ca.crt, ${DEVICE_PREFIX}-ca.key (Certificate Authority)"
echo "  - ${DEVICE_PREFIX}-server.crt, ${DEVICE_PREFIX}-server.key (Server certificate for $IP_ADDRESS)"
if [ "$SAN_ENABLED" = true ]; then
    if [ -n "$CUSTOM_DNS" ]; then
        echo "    (with SAN extension: IP:$IP_ADDRESS, DNS:$CUSTOM_DNS)"
    else
        echo "    (with SAN extension: IP:$IP_ADDRESS, DNS:localhost)"
    fi
else
    echo "    (CN only, no SAN extension)"
fi
echo "  - ${DEVICE_PREFIX}-client.crt, ${DEVICE_PREFIX}-client.key (Client certificate for $CLIENT_CN)"
echo "========================================"