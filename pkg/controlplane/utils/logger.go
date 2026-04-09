package utils

import (
	"log/slog"
	"os"
)

func InitLogger(level string) {
	var lvl slog.Level
	switch level {
	case "debug":
		lvl = slog.LevelDebug
	case "info":
		lvl = slog.LevelInfo
	case "warn":
		lvl = slog.LevelWarn
	case "error":
		lvl = slog.LevelError
	default:
		lvl = slog.LevelInfo
	}

	slog.SetDefault(slog.New(slog.NewJSONHandler(os.Stdout, &slog.HandlerOptions{
		Level: lvl,
	})))
}

func WithAttrs(attrs ...any) []any {
	return attrs
}