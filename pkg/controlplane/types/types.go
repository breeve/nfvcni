package types

import (
	yaml "gopkg.in/yaml.v3"
)

type InterfaceMode string

const (
	InterfaceModeLayer2  InterfaceMode = "l2"
	InterfaceModeLayer3  InterfaceMode = "l3"
	InterfaceModeVlanIF  InterfaceMode = "vlanif"
)

type L2VlanMode string

const (
	L2VlanModeAccess L2VlanMode = "access"
	L2VlanModeTrunk  L2VlanMode = "trunk"
)

type PortConfig struct {
	Name string        `yaml:"name" json:"name"`
	Mode InterfaceMode `yaml:"mode" json:"mode"`
	L2   *L2PortConfig `yaml:"l2,omitempty" json:"l2,omitempty"`
}

type L2PortConfig struct {
	VlanMode    L2VlanMode `yaml:"vlanMode" json:"vlanMode"`
	VlanID      int        `yaml:"vlanId,omitempty" json:"vlanId,omitempty"`
	StartVlanID int        `yaml:"startVlanId,omitempty" json:"startVlanId,omitempty"`
	EndVlanID   int        `yaml:"endVlanId,omitempty" json:"endVlanId,omitempty"`
}

type Config struct {
	Ports []*PortConfig `yaml:"ports" json:"ports"`
}

func ParseConfig(data []byte) (*Config, error) {
	cfg := &Config{}
	err := yaml.Unmarshal(data, cfg)
	if err != nil {
		return nil, err
	}
	return cfg, nil
}

type FdbEntry struct {
	Mac      string `json:"mac"`
	VlanID   int    `json:"vlanId"`
	Ifindex  uint32 `json:"ifindex"`
	LastSeen int64  `json:"lastSeen"`
}

type PortStatus struct {
	Name      string        `json:"name"`
	Mode      InterfaceMode `json:"mode"`
	Ifindex   uint32        `json:"ifindex"`
	Mac       string        `json:"mac"`
	Status    string        `json:"status"`
	L2        *L2PortConfig `json:"l2,omitempty"`
}

type Operation string

const (
	OperationAdd    Operation = "add"
	OperationDelete Operation = "delete"
	OperationUpdate Operation = "update"
)

type Event struct {
	Type      Operation   `json:"type"`
	Port      *PortConfig `json:"port,omitempty"`
	Timestamp int64        `json:"timestamp"`
}

type Error struct {
	Code    int    `json:"code"`
	Message string `json:"message"`
}

func (e *Error) Error() string {
	return e.Message
}

const (
	ErrCodeNotFound         int = 404
	ErrCodeAlreadyExists   int = 409
	ErrCodeInvalidArgument int = 400
	ErrCodeInternal        int = 500
)

func NewNotFoundError(msg string) *Error {
	return &Error{Code: ErrCodeNotFound, Message: msg}
}

func NewAlreadyExistsError(msg string) *Error {
	return &Error{Code: ErrCodeAlreadyExists, Message: msg}
}

func NewInvalidArgumentError(msg string) *Error {
	return &Error{Code: ErrCodeInvalidArgument, Message: msg}
}

func NewInternalError(msg string) *Error {
	return &Error{Code: ErrCodeInternal, Message: msg}
}