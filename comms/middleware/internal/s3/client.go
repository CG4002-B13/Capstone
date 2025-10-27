package s3

import (
	"context"
	"fmt"
	"log"

	"github.com/aws/aws-sdk-go-v2/config"
	"github.com/aws/aws-sdk-go-v2/service/s3"
)

type S3Service struct {
	client     *s3.Client
	presigner  *s3.PresignClient
	bucketName string
}

func NewS3Service(bucket string) (*S3Service, error) {
	// Load AWS Config
	cfg, err := config.LoadDefaultConfig(context.TODO())
	if err != nil {
		return nil, fmt.Errorf("unable to load AWS SDK Config, %w", err)
	}

	creds, err := cfg.Credentials.Retrieve(context.TODO())
	if err != nil {
		log.Fatal("Failed to retrieve credentials:", err)
	}

	fmt.Printf("Loaded credentials from %s\n", creds.Source)

	s3Client := s3.NewFromConfig(cfg)
	presigner := s3.NewPresignClient(s3Client)

	return &S3Service{
		client:     s3Client,
		presigner:  presigner,
		bucketName: bucket,
	}, nil
}
