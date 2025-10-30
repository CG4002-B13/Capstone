package s3

import (
	"context"
	"fmt"
	"strings"
	"time"

	"github.com/aws/aws-sdk-go-v2/aws"
	"github.com/aws/aws-sdk-go-v2/service/s3"
)

const EXPIRE_TIME time.Duration = 5 * time.Minute

func (s *S3Service) GeneratePresignedUploadURL(ctx context.Context, fileName string) (string, error) {
	if !strings.HasSuffix(fileName, ".jpg") {
		fileName = fileName + ".jpg"
	}
	key := fileName

	params := &s3.PutObjectInput{
		Bucket: aws.String(s.bucketName),
		Key:    aws.String(key),
	}

	presignedReq, err := s.presigner.PresignPutObject(ctx, params, s3.WithPresignExpires(EXPIRE_TIME))
	if err != nil {
		return "", fmt.Errorf("failed to presign put object request: %w", err)
	}

	return presignedReq.URL, nil
}

func (s *S3Service) GeneratePresignedDownloadURL(ctx context.Context, key string) (string, error) {
	params := &s3.GetObjectInput{
		Bucket: aws.String(s.bucketName),
		Key:    aws.String(key),
	}

	presignedReq, err := s.presigner.PresignGetObject(ctx, params, s3.WithPresignExpires(EXPIRE_TIME))
	if err != nil {
		return "", fmt.Errorf("failed to presign get object request: %w", err)
	}
	return presignedReq.URL, nil
}

func (s *S3Service) GeneratePresignedDeleteURL(ctx context.Context, key string) (string, error) {
	params := &s3.DeleteObjectInput{
		Bucket: aws.String(s.bucketName),
		Key:    aws.String(key),
	}

	presignedReq, err := s.presigner.PresignDeleteObject(ctx, params, s3.WithPresignExpires(EXPIRE_TIME))
	if err != nil {
		return "", fmt.Errorf("failed to presign get object request: %w", err)
	}
	return presignedReq.URL, nil
}

func (s *S3Service) ListUserImages(ctx context.Context, userName string) ([]string, error) {
	input := &s3.ListObjectsV2Input{
		Bucket: aws.String(s.bucketName),
		Prefix: aws.String(userName),
	}

	var allKeys []string
	paginator := s3.NewListObjectsV2Paginator(s.client, input)

	for paginator.HasMorePages() {
		page, err := paginator.NextPage(ctx)
		if err != nil {
			return nil, fmt.Errorf("failed to get page of objects from s3: %w", err)
		}

		for _, obj := range page.Contents {
			key := strings.ReplaceAll(*obj.Key, "⁄", "/")
			allKeys = append(allKeys, key)
		}
	}

	return allKeys, nil
}

func (s *S3Service) CompareWithS3(localFiles, s3Files []string) ([]string, []string) {
	s3Set := make(map[string]struct{})
	for _, key := range s3Files {
		s3Set[key] = struct{}{}
	}

	localSet := make(map[string]struct{})
	for _, key := range localFiles {
		localSet[key] = struct{}{}
	}

	var missingInS3, missingOnLocal []string

	for _, file := range localFiles {
		if _, found := s3Set[file]; !found {
			missingInS3 = append(missingInS3, file)
		}
	}

	for _, file := range s3Files {
		if _, found := localSet[file]; !found {
			missingOnLocal = append(missingOnLocal, file)
		}
	}

	return missingInS3, missingOnLocal
}
