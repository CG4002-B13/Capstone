#include <SPIFFS.h>
#include <WiFiClientSecure.h>

class CertificateManager
{
private:
    String ca_cert;
    String client_cert;
    String client_key;

public:
    bool loadCertificates()
    {
        if (!SPIFFS.begin(true))
        {
            Serial.println("SPIFFS Mount Failed");
            return false;
        }

        // Load CA certificate
        File file = SPIFFS.open("/ca.crt", "r");
        if (!file)
        {
            Serial.println("Failed to open ca.crt");
            return false;
        }
        ca_cert = file.readString();
        file.close();

        // Load client certificate
        file = SPIFFS.open("/client.crt", "r");
        if (!file)
        {
            Serial.println("Failed to open client.crt");
            return false;
        }
        client_cert = file.readString();
        file.close();

        // Load private key
        file = SPIFFS.open("/client.key", "r");
        if (!file)
        {
            Serial.println("Failed to open client.key");
            return false;
        }
        client_key = file.readString();
        file.close();

        Serial.println("All certificates loaded successfully");
        return true;
    }

    void applyCertificates(WiFiClientSecure &client)
    {
        client.setCACert(ca_cert.c_str());
        client.setCertificate(client_cert.c_str());
        client.setPrivateKey(client_key.c_str());
    }

    void printCertificateInfo()
    {
        Serial.printf("CA Cert size: %d bytes\n", ca_cert.length());
        Serial.printf("Client Cert size: %d bytes\n", client_cert.length());
        Serial.printf("Private Key size: %d bytes\n", client_key.length());
    }

private:
    bool loadFile(const char *path, String &content)
    {
        File file = SPIFFS.open(path, "r");
        if (!file)
        {
            Serial.printf("Failed to open: %s\n", path);
            return false;
        }

        content = file.readString();
        file.close();

        if (content.length() == 0)
        {
            Serial.printf("Empty file: %s\n", path);
            return false;
        }

        Serial.printf("Loaded %s (%d bytes)\n", path, content.length());
        return true;
    }
};