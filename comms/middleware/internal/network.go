package internal

import (
	"crypto/tls"
	"crypto/x509"
	"fmt"
	"os"
)

func LoadCACert(caCertFile string) (*x509.CertPool, error) {
	caCert, err := os.ReadFile(caCertFile)
	if err != nil {
		return nil, fmt.Errorf("failed to read CA certificate: %v", err)
	}

	caCertPool := x509.NewCertPool()
	if !caCertPool.AppendCertsFromPEM(caCert) {
		return nil, fmt.Errorf("failed to parse CA certificate")
	}
	return caCertPool, nil
}

func LoadKeyPair(certFile, keyFile string) (tls.Certificate, error) {
	return tls.LoadX509KeyPair(certFile, keyFile)
}
