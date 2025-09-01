#include "CertificateManager.hpp"

CertificateManager::CertificateManager() {}

CertificateManager::~CertificateManager() { clearCertificates(); }

bool CertificateManager::loadCertificates() {
    Serial.println("Initializing SPIFFS and loading certificates...");

    if (!SPIFFS.begin(true)) {
        Serial.println("ERROR: SPIFFS Mount Failed");
        return false;
    }

    bool success = true;

    if (!loadFile("/ca.crt", ca_cert)) {
        Serial.println("ERROR: Failed to load CA certificate");
        success = false;
    }

    if (!loadFile("/client.crt", client_cert)) {
        Serial.println("ERROR: Failed to load client certificate");
        success = false;
    }

    if (!loadFile("/client.key", client_key)) {
        Serial.println("ERROR: Failed to load private key");
        success = false;
    }

    if (success) {
        Serial.println("SUCCESS: All certificates loaded successfully");
        printCertificateInfo();
    } else {
        Serial.println("ERROR: Failed to load one or more certificates");
        clearCertificates();
    }

    return success;
}

void CertificateManager::applyCertificates(WiFiClientSecure* client) {
    if (!client) {
        Serial.println("ERROR: WiFiClientSecure pointer is null");
        return;
    }

    if (!isCertificatesLoaded()) {
        Serial.println(
            "ERROR: Certificates not loaded. Call loadCertificates() first.");
        return;
    }

    Serial.println("Applying SSL certificates to WiFiClientSecure...");

    client->setCACert(ca_cert.c_str());
    client->setCertificate(client_cert.c_str());
    client->setPrivateKey(client_key.c_str());

    Serial.println("SSL certificates applied successfully");
}

void CertificateManager::printCertificateInfo() {
    Serial.println("=== Certificate Information ===");
    Serial.printf("CA Certificate: %d bytes\n", ca_cert.length());
    Serial.printf("Client Certificate: %d bytes\n", client_cert.length());
    Serial.printf("Private Key: %d bytes\n", client_key.length());
    Serial.printf("Total certificate data: %d bytes\n",
                  ca_cert.length() + client_cert.length() +
                      client_key.length());
    Serial.println("==============================");
}

bool CertificateManager::isCertificatesLoaded() const {
    return (ca_cert.length() > 0 && client_cert.length() > 0 &&
            client_key.length() > 0);
}

void CertificateManager::clearCertificates() {
    ca_cert = "";
    client_cert = "";
    client_key = "";
    Serial.println("Certificate data cleared from memory");
}

bool CertificateManager::loadFile(const char *path, String &content) {
    if (!path) {
        Serial.println("ERROR: File path is null");
        return false;
    }

    File file = SPIFFS.open(path, "r");
    if (!file) {
        Serial.printf("ERROR: Failed to open file: %s\n", path);
        return false;
    }

    content = file.readString();
    file.close();

    if (content.length() == 0) {
        Serial.printf("ERROR: File is empty: %s\n", path);
        return false;
    }

    Serial.printf("SUCCESS: Loaded %s (%d bytes)\n", path, content.length());
    return true;
}