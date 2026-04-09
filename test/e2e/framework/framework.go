package framework

import (
	"context"
	"fmt"
	"io"
	"net/http"
	"os"
	"strconv"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
)

type Config struct {
	ControlPlaneHost string
	ControlPlanePort int
	DataplaneHost    string
	DataplanePort    int
}

func GetConfig() *Config {
	host := getEnv("NFV_CNI_CONTROLPLANE_HOST", "localhost")
	port := getEnvInt("NFV_CNI_CONTROLPLANE_PORT", 8080)
	dataplaneHost := getEnv("NFV_CNI_DATAPLANE_GRPC_HOST", "localhost")
	dataplanePort := getEnvInt("NFV_CNI_DATAPLANE_GRPC_PORT", 9090)

	return &Config{
		ControlPlaneHost: host,
		ControlPlanePort: port,
		DataplaneHost:    dataplaneHost,
		DataplanePort:    dataplanePort,
	}
}

func getEnv(key, defaultValue string) string {
	if value := os.Getenv(key); value != "" {
		return value
	}
	return defaultValue
}

func getEnvInt(key string, defaultValue int) int {
	if value := os.Getenv(key); value != "" {
		if intVal, err := strconv.Atoi(value); err == nil {
			return intVal
		}
	}
	return defaultValue
}

func (c *Config) ControlPlaneURL() string {
	return fmt.Sprintf("http://%s:%d", c.ControlPlaneHost, c.ControlPlanePort)
}

func (c *Config) DataplaneAddr() string {
	return fmt.Sprintf("%s:%d", c.DataplaneHost, c.DataplanePort)
}

type HTTPClient struct {
	client *http.Client
	baseURL string
}

func NewHTTPClient(cfg *Config) *HTTPClient {
	return &HTTPClient{
		client: &http.Client{
			Timeout: 10 * time.Second,
		},
		baseURL: cfg.ControlPlaneURL(),
	}
}

func (c *HTTPClient) Get(path string) (*http.Response, error) {
	url := c.baseURL + path
	return c.client.Get(url)
}

func (c *HTTPClient) Post(path string, body interface{}) (*http.Response, error) {
	url := c.baseURL + path
	req, err := http.NewRequest(http.MethodPost, url, nil)
	if err != nil {
		return nil, err
	}
	return c.client.Do(req)
}

func (c *HTTPClient) PostWithBody(path string, body io.Reader) (*http.Response, error) {
	url := c.baseURL + path
	req, err := http.NewRequest(http.MethodPost, url, body)
	if err != nil {
		return nil, err
	}
	req.Header.Set("Content-Type", "application/json")
	return c.client.Do(req)
}

func (c *HTTPClient) Put(path string, body interface{}) (*http.Response, error) {
	url := c.baseURL + path
	req, err := http.NewRequest(http.MethodPut, url, nil)
	if err != nil {
		return nil, err
	}
	return c.client.Do(req)
}

func (c *HTTPClient) Delete(path string) (*http.Response, error) {
	url := c.baseURL + path
	req, err := http.NewRequest(http.MethodDelete, url, nil)
	if err != nil {
		return nil, err
	}
	return c.client.Do(req)
}

type GrpcClient struct {
	conn *grpc.ClientConn
	addr string
}

func NewGrpcClient(cfg *Config) (*GrpcClient, error) {
	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer cancel()

	conn, err := grpc.DialContext(
		ctx,
		cfg.DataplaneAddr(),
		grpc.WithTransportCredentials(insecure.NewCredentials()),
	)
	if err != nil {
		return nil, fmt.Errorf("failed to connect to %s: %w", cfg.DataplaneAddr(), err)
	}

	return &GrpcClient{
		conn: conn,
		addr: cfg.DataplaneAddr(),
	}, nil
}

func (c *GrpcClient) Close() error {
	return c.conn.Close()
}

func (c *GrpcClient) Conn() *grpc.ClientConn {
	return c.conn
}