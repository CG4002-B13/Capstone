#ifndef CERTIFICATE_MANAGER_HPP
#define CERTIFICATE_MANAGER_HPP

#include <Arduino.h>
#include <SPIFFS.h>
#include <WiFiClientSecure.h>

class CertificateManager {
private:
    String ca_cert;
    String client_cert;
    String client_key;
    
    bool loadFile(const char* path, String& content);

public:
    CertificateManager();
    ~CertificateManager();
    
    bool loadCertificates();
    void applyCertificates(WiFiClientSecure* client);    
    bool isCertificatesLoaded() const;
    void clearCertificates();
    void printCertificateInfo();
    
    const String& getCACert() const { return ca_cert; }
    const String& getClientCert() const { return client_cert; }
    const String& getClientKey() const { return client_key; }
};

#endif // CERTIFICATE_MANAGER_HPP