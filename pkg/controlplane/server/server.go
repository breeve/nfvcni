package server

import (
	"context"
	"encoding/json"
	"io"
	"net"
	"net/http"
	"sync"

	"github.com/breeve/nfvcni/pkg/controlplane/client"
	"github.com/breeve/nfvcni/pkg/controlplane/types"
	"log/slog"
)

type Server struct {
	httpServer *http.Server
	configMgr  ConfigManager
	dpClient   client.DataplaneClient

	mu         sync.RWMutex
	portStatus map[string]*types.PortStatus
}

func NewServer(cfgMgr ConfigManager, dpClient client.DataplaneClient) *Server {
	return &Server{
		configMgr:  cfgMgr,
		dpClient:   dpClient,
		portStatus: make(map[string]*types.PortStatus),
	}
}

func (s *Server) Start(addr string) error {
	mux := http.NewServeMux()

	mux.HandleFunc("/admin/ports", s.handlePorts)
	mux.HandleFunc("/admin/ports/", s.handlePortDetail)
	mux.HandleFunc("/admin/fdb", s.handleFdb)
	mux.HandleFunc("/health", s.handleHealth)

	s.httpServer = &http.Server{
		Addr:    addr,
		Handler: mux,
	}

	slog.Info("Starting controlplane server", "addr", addr)
	return s.httpServer.ListenAndServe()
}

func (s *Server) Stop(ctx context.Context) error {
	return s.httpServer.Shutdown(ctx)
}

func (s *Server) handlePorts(w http.ResponseWriter, r *http.Request) {
	switch r.Method {
	case http.MethodGet:
		s.listPorts(w, r)
	case http.MethodPost:
		s.addPort(w, r)
	default:
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
	}
}

func (s *Server) handlePortDetail(w http.ResponseWriter, r *http.Request) {
	name := r.URL.Path[len("/admin/ports/"):]
	if name == "" {
		http.Error(w, "Port name required", http.StatusBadRequest)
		return
	}

	switch r.Method {
	case http.MethodGet:
		s.getPort(w, r, name)
	case http.MethodDelete:
		s.deletePort(w, r, name)
	case http.MethodPut:
		s.updatePort(w, r, name)
	default:
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
	}
}

func (s *Server) handleFdb(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	ctx := context.Background()
	entries, err := s.dpClient.GetFdbTable(ctx)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	if entries == nil {
		entries = []*types.FdbEntry{}
	}

	data, _ := json.Marshal(entries)
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	w.Write(data)
}

func (s *Server) handleHealth(w http.ResponseWriter, r *http.Request) {
	w.WriteHeader(http.StatusOK)
	w.Write([]byte("OK"))
}

func (s *Server) listPorts(w http.ResponseWriter, r *http.Request) {
	ports := s.configMgr.ListPorts()

	data, _ := json.Marshal(ports)
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	w.Write(data)
}

func (s *Server) addPort(w http.ResponseWriter, r *http.Request) {
	body, err := io.ReadAll(r.Body)
	if err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	var portCfg types.PortConfig
	if err := json.Unmarshal(body, &portCfg); err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	if err := s.configMgr.AddPort(portCfg.Name, &portCfg); err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusCreated)
	w.Write([]byte("{}"))
}

func (s *Server) getPort(w http.ResponseWriter, r *http.Request, name string) {
	port, err := s.configMgr.GetPort(name)
	if err != nil {
		http.Error(w, err.Error(), http.StatusNotFound)
		return
	}

	data, _ := json.Marshal(port)
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	w.Write(data)
}

func (s *Server) updatePort(w http.ResponseWriter, r *http.Request, name string) {
	body, err := io.ReadAll(r.Body)
	if err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	var portCfg types.PortConfig
	if err := json.Unmarshal(body, &portCfg); err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	if err := s.configMgr.UpdatePort(name, &portCfg); err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	w.Write([]byte("{}"))
}

func (s *Server) deletePort(w http.ResponseWriter, r *http.Request, name string) {
	if err := s.configMgr.DeletePort(name); err != nil {
		http.Error(w, err.Error(), http.StatusNotFound)
		return
	}

	w.WriteHeader(http.StatusNoContent)
}

type CNIServer struct {
	configMgr ConfigManager
	dpClient  client.DataplaneClient
}

func NewCNIServer(cfgMgr ConfigManager, dpClient client.DataplaneClient) *CNIServer {
	return &CNIServer{
		configMgr: cfgMgr,
		dpClient:  dpClient,
	}
}

func (s *CNIServer) AddNetwork(ctx context.Context, req *AddNetworkRequest) (*AddNetworkResponse, error) {
	if req == nil || req.PortName == "" {
		return nil, types.NewInvalidArgumentError("port name is required")
	}

	portCfg := &types.PortConfig{
		Name: req.PortName,
		Mode: types.InterfaceModeLayer2,
	}

	if err := s.configMgr.AddPort(req.PortName, portCfg); err != nil {
		return nil, err
	}

	return &AddNetworkResponse{
		Success: true,
	}, nil
}

func (s *CNIServer) DelNetwork(ctx context.Context, req *DelNetworkRequest) (*DelNetworkResponse, error) {
	if req == nil || req.PortName == "" {
		return nil, types.NewInvalidArgumentError("port name is required")
	}

	if err := s.configMgr.DeletePort(req.PortName); err != nil {
		return nil, err
	}

	return &DelNetworkResponse{
		Success: true,
	}, nil
}

type AddNetworkRequest struct {
	PortName    string
	NetNS       string
	ContainerID string
}

type AddNetworkResponse struct {
	Success bool
}

type DelNetworkRequest struct {
	PortName    string
	NetNS       string
	ContainerID string
}

type DelNetworkResponse struct {
	Success bool
}

func ListenAndServe(addr string, handler http.Handler) error {
	ln, err := net.Listen("tcp", addr)
	if err != nil {
		return err
	}
	return http.Serve(ln, handler)
}