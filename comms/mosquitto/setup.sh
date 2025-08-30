mkdir -p ./config/certs
cd ./config/certs

# 1. CA
openssl genrsa -out ca.key 2048
openssl req -x509 -new -nodes -key ca.key -days 3650 -out ca.crt -sha256 \
  -subj "/C=SG/ST=Singapore/L=Singapore/O=NUS/OU=MQTT/CN=mqttCA"

# 2. Broker
openssl genrsa -out server.key 2048
openssl req -new -key server.key -out server.csr \
  -subj "/C=SG/ST=Singapore/L=Singapore/O=NUS/OU=MQTT/CN=127.0.0.1"
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
  -out server.crt -days 365 -sha256

# 3. Client
openssl genrsa -out client.key 2048
openssl req -new -key client.key -out client.csr \
  -subj "/C=SG/ST=Singapore/L=Singapore/O=NUS/OU=MQTT/CN=mqttClient"
openssl x509 -req -in client.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
  -out client.crt -days 365 -sha256
