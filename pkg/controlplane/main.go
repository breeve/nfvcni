package main

import (
	"context"
	"os"
	"os/signal"
	"syscall"

	"github.com/breeve/nfvcni/pkg/controlplane/client"
	"github.com/breeve/nfvcni/pkg/controlplane/server"
	"log/slog"
	"github.com/spf13/cobra"
)

var (
	addr      string
	dataplane string
)

var rootCmd = &cobra.Command{
	Use:   "controlplane",
	Short: "nfvcni controlplane",
	Long:  "Control plane for nfvcni, provides dynamic configuration management",
	Run: func(cmd *cobra.Command, args []string) {
		run()
	},
}

func run() {
	slog.Info("Starting controlplane", "addr", addr, "dataplane", dataplane)

	configMgr := server.NewConfigManager()

	var dpClient client.DataplaneClient
	var err error
	if dataplane != "" {
		dpClient, err = client.NewDataplaneClient(dataplane, 0)
		if err != nil {
			slog.Error("Failed to create dataplane client", "error", err)
			os.Exit(1)
		}
		defer dpClient.Close()
	}

	srv := server.NewServer(configMgr, dpClient)

	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	go func() {
		if err := srv.Start(addr); err != nil {
			slog.Error("Server error", "error", err)
		}
	}()

	slog.Info("Controlplane started", "addr", addr)

	sigCh := make(chan os.Signal, 1)
	signal.Notify(sigCh, syscall.SIGINT, syscall.SIGTERM)
	<-sigCh

	slog.Info("Shutting down controlplane...")
	if err := srv.Stop(ctx); err != nil {
		slog.Error("Error stopping server", "error", err)
	}
}

func main() {
	rootCmd.Flags().StringVarP(&addr, "addr", "a", ":8080", "server address")
	rootCmd.Flags().StringVarP(&dataplane, "dataplane", "d", "", "dataplane address")

	if err := rootCmd.Execute(); err != nil {
		slog.Error("Error executing command", "error", err)
		os.Exit(1)
	}
}