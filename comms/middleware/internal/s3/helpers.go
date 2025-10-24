package s3

import (
	"context"
	"fmt"
	"time"

	"github.com/aws/aws-sdk-go-v2/aws"
	"github.com/aws/aws-sdk-go-v2/service/s3"
	"github.com/aws/aws-sdk-go-v2/service/s3/types"
)

const EXPIRE_TIME time.Duration = 5 * time.Minute

func (s *S3Service) GeneratePresignedUploadURL(ctx context.Context, username string, fileNumber int) (string, error) {
	key := fmt.Sprintf("%s/%d.%s", username, fileNumber, "jpg")

	params := &s3.PutObjectInput{
		Bucket: aws.String(s.bucketName),
		Key:    aws.String(key),
	}

	presignedReq, err := s.presigner.PresignPutObject(ctx, params, s3.WithPresignExpires(EXPIRE_TIME))
	if err != nil {
		// <add send error packet to client here>
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
		// <add send error packet to client here>
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
		// <add send error packet to client here>
		return "", fmt.Errorf("failed to presign get object request: %w", err)
	}
	return presignedReq.URL, nil
}

func (s *S3Service) ListUserImages(ctx context.Context, fileName string) ([]types.Object, error) {
	prefix := fmt.Sprintf("%s/", fileName)
	input := &s3.ListObjectsV2Input{
		Bucket: aws.String(s.bucketName),
		Prefix: aws.String(prefix),
	}

	var allObjects []types.Object
	paginator := s3.NewListObjectsV2Paginator(s.client, input)
	for paginator.HasMorePages() {
		page, err := paginator.NextPage(ctx)
		if err != nil {
			return nil, fmt.Errorf("failed to get page of objects: %w", err)
		}
		allObjects = append(allObjects, page.Contents...)
	}

	return allObjects, nil
}
