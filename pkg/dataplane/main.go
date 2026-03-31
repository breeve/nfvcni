package main

import (
	"log/slog"
	"os"

	"github.com/breeve/nfvcni/pkg/dataplane/bpf"
	"github.com/breeve/nfvcni/pkg/dataplane/config"
	"github.com/spf13/cobra"
	"gopkg.in/yaml.v3"
)

var (
	cfgFile string
	cfg     config.Config
)

var rootCmd = &cobra.Command{
	Use:   "dataplane",
	Short: "l3 switch dataplane",
	Run: func(cmd *cobra.Command, args []string) {
		loadConfig(cfgFile)
		bpf.Agent(&cfg)
	},
}

func loadConfig(path string) {
	content, err := os.ReadFile(path)
	if err != nil {
		slog.Error("Error reading config file", "error", err)
		os.Exit(1)
	}

	err = yaml.Unmarshal(content, &cfg)
	if err != nil {
		slog.Error("Error parsing yaml", "error", err)
		os.Exit(1)
	}

	slog.Info("Loaded config", "config", cfg)
}

func main() {
	rootCmd.PersistentFlags().StringVarP(&cfgFile, "config", "c", "config.yaml", "config file (default is config.yaml)")
	if err := rootCmd.Execute(); err != nil {
		slog.Error("Error executing command", "error", err)
		os.Exit(1)
	}
}
