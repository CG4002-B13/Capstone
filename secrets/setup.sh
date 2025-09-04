#!/bin/bash

# Default values
DEFAULT_IP="127.0.0.1"
DEFAULT_CN="localClient"
DEFAULT_DIR="./"

IP_ADDRESS="$DEFAULT_IP"
CLIENT_CN="$DEFAULT_CN"
CERT_DIR="$DEFAULT_DIR"

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

echo "=== SSL Certificate Generation ==="
echo "Server IP Address: $IP_ADDRESS"
echo "Client Common Name: $CLIENT_CN"
echo "Certificate Directory: $CERT_DIR"
echo "=================================="
echo ""

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

rm server.csr client.csr ca.csr ca.srl

echo ""
echo "=== Certificate Generation Complete ==="
echo "Generated files in $CERT_DIR/:"
echo "  - ca.crt, ca.key (Certificate Authority)"
echo "  - server.crt, server.key (Server certificate for $IP_ADDRESS)"
echo "  - client.crt, client.key (Client certificate for $CLIENT_CN)"
echo "========================================"


# # Unknown issue with SAN, unable to get multiple hostname to work
# #!/bin/bash

# # Default values
# DEFAULT_IP="127.0.0.1"
# DEFAULT_CN="localClient"
# DEFAULT_DIR="./"

# IP_ADDRESSES="$DEFAULT_IP"
# CLIENT_CN="$DEFAULT_CN"
# CERT_DIR="$DEFAULT_DIR"

# usage() {
#     echo "Usage: $0 [-ip <ip_address1,ip_address2,...>] [-cn <common_name>] [-dir <directory>]"
#     echo "  -ip <ip_addresses> : Comma-separated IPs for the server certificate SAN (default: $DEFAULT_IP)"
#     echo "  -cn <common_name>  : Common name for the client certificate (default: $DEFAULT_CN)"
#     echo "  -dir <directory>   : Directory to store certificates (default: $DEFAULT_DIR)"
#     echo "  -h, --help         : Display this help message"
#     echo ""
#     echo "Examples:"
#     echo "  $0"
#     echo "  $0 -ip 192.168.1.100,10.0.0.5"
#     echo "  $0 -cn myClient"
#     echo "  $0 -dir ./certs"
#     echo "  $0 -ip 10.0.0.5,127.0.0.1 -cn sensor1 -dir ./certs"
# }

# while [[ $# -gt 0 ]]; do
#     case $1 in
#         -ip)
#             IP_ADDRESSES="$2"
#             shift 2
#             ;;
#         -cn)
#             CLIENT_CN="$2"
#             shift 2
#             ;;
#         -dir)
#             CERT_DIR="$2"
#             shift 2
#             ;;
#         -h|--help)
#             usage
#             exit 0
#             ;;
#         *)
#             echo "Unknown option: $1"
#             usage
#             exit 1
#             ;;
#     esac
# done

# echo "=== SSL Certificate Generation ==="
# echo "Server IP Addresses: $IP_ADDRESSES"
# echo "Client Common Name: $CLIENT_CN"
# echo "Certificate Directory: $CERT_DIR"
# echo "=================================="
# echo ""

# mkdir -p "$CERT_DIR"
# cd "$CERT_DIR"

# echo "1. Generating CA certificate..."
# # 1. Certificate Authority (CA)
# openssl genrsa -out ca.key 2048
# openssl req -x509 -new -nodes -key ca.key -days 3650 -out ca.crt -sha256 \
# -subj "/C=SG/ST=Singapore/L=Singapore/O=NUS/OU=MQTT/CN=mqttCA"

# echo "2. Generating server certificate with SANs..."
# # 2. Server/Broker Certificate
# openssl genrsa -out server.key 2048
# openssl req -new -key server.key -out server.csr \
# -subj "/C=SG/ST=Singapore/L=Singapore/O=NUS/OU=MQTT/CN=$(echo $IP_ADDRESSES | cut -d',' -f1)"

# # Create temporary OpenSSL config for SAN
# SAN_CONFIG=$(mktemp)
# cat > "$SAN_CONFIG" <<EOL
# [ req ]
# distinguished_name = req_distinguished_name
# req_extensions = v3_req
# prompt = no

# [ req_distinguished_name ]
# CN = $(echo $IP_ADDRESSES | cut -d',' -f1)

# [ v3_req ]
# subjectAltName = @alt_names

# [ alt_names ]
# EOL

# # Add IPs to SAN
# count=1
# IFS=',' read -ra ADDR <<< "$IP_ADDRESSES"
# for ip in "${ADDR[@]}"; do
#     echo "IP.$count = $ip" >> "$SAN_CONFIG"
#     count=$((count+1))
# done

# openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
# -out server.crt -days 365 -sha256 -extfile "$SAN_CONFIG" -extensions v3_req

# echo "3. Generating client certificate with CN: $CLIENT_CN..."
# # 3. Client Certificate
# openssl genrsa -out client.key 2048
# openssl req -new -key client.key -out client.csr \
# -subj "/C=SG/ST=Singapore/L=Singapore/O=NUS/OU=MQTT/CN=$CLIENT_CN"
# openssl x509 -req -in client.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
# -out client.crt -days 365 -sha256

# # Cleanup
# rm server.csr client.csr ca.srl "$SAN_CONFIG"

# echo ""
# echo "=== Certificate Generation Complete ==="
# echo "Generated files in $CERT_DIR/:"
# echo "  - ca.crt, ca.key (Certificate Authority)"
# echo "  - server.crt, server.key (Server certificate with SANs: $IP_ADDRESSES)"
# echo "  - client.crt, client.key (Client certificate for $CLIENT_CN)"
# echo "========================================"
