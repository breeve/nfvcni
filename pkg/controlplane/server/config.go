package server

import (
	"sync"

	"github.com/breeve/nfvcni/pkg/controlplane/types"
)

type ConfigManager interface {
	AddPort(name string, config *types.PortConfig) error
	DeletePort(name string) error
	UpdatePort(name string, config *types.PortConfig) error
	GetPort(name string) (*types.PortConfig, error)
	ListPorts() []*types.PortConfig
}

type configManager struct {
	mu    sync.RWMutex
	ports map[string]*types.PortConfig
}

func NewConfigManager() ConfigManager {
	return &configManager{
		ports: make(map[string]*types.PortConfig),
	}
}

func (m *configManager) AddPort(name string, config *types.PortConfig) error {
	if name == "" {
		return types.NewInvalidArgumentError("port name cannot be empty")
	}
	if config == nil {
		return types.NewInvalidArgumentError("port config cannot be nil")
	}

	m.mu.Lock()
	defer m.mu.Unlock()

	if _, exists := m.ports[name]; exists {
		return types.NewAlreadyExistsError("port already exists: " + name)
	}

	m.ports[name] = config
	return nil
}

func (m *configManager) DeletePort(name string) error {
	if name == "" {
		return types.NewInvalidArgumentError("port name cannot be empty")
	}

	m.mu.Lock()
	defer m.mu.Unlock()

	if _, exists := m.ports[name]; !exists {
		return types.NewNotFoundError("port not found: " + name)
	}

	delete(m.ports, name)
	return nil
}

func (m *configManager) UpdatePort(name string, config *types.PortConfig) error {
	if name == "" {
		return types.NewInvalidArgumentError("port name cannot be empty")
	}
	if config == nil {
		return types.NewInvalidArgumentError("port config cannot be nil")
	}

	m.mu.Lock()
	defer m.mu.Unlock()

	if _, exists := m.ports[name]; !exists {
		return types.NewNotFoundError("port not found: " + name)
	}

	m.ports[name] = config
	return nil
}

func (m *configManager) GetPort(name string) (*types.PortConfig, error) {
	if name == "" {
		return nil, types.NewInvalidArgumentError("port name cannot be empty")
	}

	m.mu.RLock()
	defer m.mu.RUnlock()

	if port, exists := m.ports[name]; exists {
		return port, nil
	}

	return nil, types.NewNotFoundError("port not found: " + name)
}

func (m *configManager) ListPorts() []*types.PortConfig {
	m.mu.RLock()
	defer m.mu.RUnlock()

	ports := make([]*types.PortConfig, 0, len(m.ports))
	for _, port := range m.ports {
		ports = append(ports, port)
	}

	return ports
}

type PortManager struct {
	configManager ConfigManager
}

func NewPortManager(cm ConfigManager) *PortManager {
	return &PortManager{
		configManager: cm,
	}
}

func (p *PortManager) AddPort(name string, config *types.PortConfig) error {
	return p.configManager.AddPort(name, config)
}

func (p *PortManager) DeletePort(name string) error {
	return p.configManager.DeletePort(name)
}

func (p *PortManager) UpdatePort(name string, config *types.PortConfig) error {
	return p.configManager.UpdatePort(name, config)
}

func (p *PortManager) GetPort(name string) (*types.PortConfig, error) {
	return p.configManager.GetPort(name)
}

func (p *PortManager) ListPorts() []*types.PortConfig {
	return p.configManager.ListPorts()
}