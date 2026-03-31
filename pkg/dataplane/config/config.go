// Package config...
package config

import (
	yaml "gopkg.in/yaml.v3"
)

type Config struct {
	Ports []*PortConfig `yaml:"ports"`
}

type PortConfig struct {
	Name string        `yaml:"name"`
	Mode InterfaceMode `yaml:"mode"`
	L2   *L2PortConfig `yaml:"l2,omitempty"`
}

type InterfaceMode string

const (
	InterfaceModeLayer2 InterfaceMode = "l2"
	InterfaceModeLayer3 InterfaceMode = "l3"
	InterfaceModeVlanIF InterfaceMode = "vlanif"
)

type L2VlanMode string

const (
	L2VlanModeAccess L2VlanMode = "access"
	L2VlanModeTrunk  L2VlanMode = "trunk"
)

type L2PortConfig struct {
	VlanMode    L2VlanMode `yaml:"vlanMode"`
	VlanID      int        `yaml:"vlanId,omitempty"`
	StartVlanID int        `yaml:"startVlanId,omitempty"`
	EndVlanID   int        `yaml:"endVlanId,omitempty"`
}

func ParseConfig(data []byte) (*Config, error) {
	cfg := &Config{}
	err := yaml.Unmarshal(data, cfg)
	if err != nil {
		return nil, err
	}
	return cfg, nil
}
