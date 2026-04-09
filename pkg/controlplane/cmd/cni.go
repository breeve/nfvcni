package cmd

import (
	"github.com/breeve/nfvcni/pkg/controlplane/types"
)

type CNIExecutor interface {
	Add(args *AddArgs) error
	Delete(args *DeleteArgs) error
	Check(args *CheckArgs) error
	Version() string
}

type AddArgs struct {
	ContainerID string
	NetNS       string
	IfName      string
	Args        string
	CNIConf     []byte
}

type DeleteArgs struct {
	ContainerID string
	NetNS       string
	IfName      string
	Args        string
	CNIConf     []byte
}

type CheckArgs struct {
	ContainerID string
	NetNS       string
	IfName      string
}

type cniExecutor struct {
	portManager PortManager
}

type PortManager interface {
	AddPort(name string, config *types.PortConfig) error
	DeletePort(name string) error
	GetPort(name string) (*types.PortConfig, error)
	ListPorts() []*types.PortConfig
}

func NewCNIExecutor(pm PortManager) CNIExecutor {
	return &cniExecutor{
		portManager: pm,
	}
}

func (e *cniExecutor) Add(args *AddArgs) error {
	if args == nil || args.IfName == "" {
		return types.NewInvalidArgumentError("interface name is required")
	}

	portCfg := &types.PortConfig{
		Name: args.IfName,
		Mode: types.InterfaceModeLayer2,
	}

	return e.portManager.AddPort(args.IfName, portCfg)
}

func (e *cniExecutor) Delete(args *DeleteArgs) error {
	if args == nil || args.IfName == "" {
		return types.NewInvalidArgumentError("interface name is required")
	}

	return e.portManager.DeletePort(args.IfName)
}

func (e *cniExecutor) Check(args *CheckArgs) error {
	return nil
}

func (e *cniExecutor) Version() string {
	return "0.1.0"
}