package client

import (
	"context"
	"time"

	"github.com/breeve/nfvcni/pkg/controlplane/types"
)

type DataplaneClient interface {
	AttachPort(ctx context.Context, ifindex uint32, config *types.PortConfig) error
	DetachPort(ctx context.Context, ifindex uint32) error
	AddFdbEntry(ctx context.Context, mac string, vlan int, ifindex uint32) error
	DelFdbEntry(ctx context.Context, mac string, vlan int) error
	GetFdbTable(ctx context.Context) ([]*types.FdbEntry, error)
	Close() error
}

type dataplaneClient struct {
	addr    string
	timeout time.Duration
}

func NewDataplaneClient(addr string, timeout time.Duration) (DataplaneClient, error) {
	if addr == "" {
		addr = "localhost:50051"
	}
	if timeout == 0 {
		timeout = 5 * time.Second
	}

	return &dataplaneClient{
		addr:    addr,
		timeout: timeout,
	}, nil
}

func (c *dataplaneClient) AttachPort(ctx context.Context, ifindex uint32, config *types.PortConfig) error {
	ctx, cancel := context.WithTimeout(ctx, c.timeout)
	defer cancel()

	return c.attachPortInternal(ctx, ifindex, config)
}

func (c *dataplaneClient) DetachPort(ctx context.Context, ifindex uint32) error {
	ctx, cancel := context.WithTimeout(ctx, c.timeout)
	defer cancel()

	return c.detachPortInternal(ctx, ifindex)
}

func (c *dataplaneClient) AddFdbEntry(ctx context.Context, mac string, vlan int, ifindex uint32) error {
	ctx, cancel := context.WithTimeout(ctx, c.timeout)
	defer cancel()

	return c.addFdbEntryInternal(ctx, mac, vlan, ifindex)
}

func (c *dataplaneClient) DelFdbEntry(ctx context.Context, mac string, vlan int) error {
	ctx, cancel := context.WithTimeout(ctx, c.timeout)
	defer cancel()

	return c.delFdbEntryInternal(ctx, mac, vlan)
}

func (c *dataplaneClient) GetFdbTable(ctx context.Context) ([]*types.FdbEntry, error) {
	ctx, cancel := context.WithTimeout(ctx, c.timeout)
	defer cancel()

	return c.getFdbTableInternal(ctx)
}

func (c *dataplaneClient) Close() error {
	return nil
}

func (c *dataplaneClient) attachPortInternal(ctx context.Context, ifindex uint32, config *types.PortConfig) error {
	return nil
}

func (c *dataplaneClient) detachPortInternal(ctx context.Context, ifindex uint32) error {
	return nil
}

func (c *dataplaneClient) addFdbEntryInternal(ctx context.Context, mac string, vlan int, ifindex uint32) error {
	return nil
}

func (c *dataplaneClient) delFdbEntryInternal(ctx context.Context, mac string, vlan int) error {
	return nil
}

func (c *dataplaneClient) getFdbTableInternal(ctx context.Context) ([]*types.FdbEntry, error) {
	return nil, nil
}