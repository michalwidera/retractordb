package main

import (
	"context"
	"log"
	"net/http"
	"os"
	"os/signal"
	"syscall"
	"time"
)

type Config struct {
	QueryFile  string
	HTTPPort   string
	TCPPort    string
	Xretractor string
	Xqry       string
	Xtrdb      string
}

func envOr(key, def string) string {
	if v := os.Getenv(key); v != "" {
		return v
	}
	return def
}

func configFromEnv() Config {
	return Config{
		QueryFile:  envOr("RDB_QUERY_FILE", "startup.rql"),
		HTTPPort:   envOr("RDB_HTTP_PORT", "8080"),
		TCPPort:    envOr("RDB_TCP_PORT", "9090"),
		Xretractor: envOr("RDB_XRETRACTOR", "xretractor"),
		Xqry:       envOr("RDB_XQRY", "xqry"),
		Xtrdb:      envOr("RDB_XTRDB", "xtrdb"),
	}
}

func main() {
	cfg := configFromEnv()
	pm := NewProcessManager(cfg)

	if err := pm.StartRetractor(cfg.QueryFile); err != nil {
		log.Printf("start xretractor: %v", err)
	}

	shutdown := make(chan struct{}, 1)

	srv := &http.Server{
		Addr:    ":" + cfg.HTTPPort,
		Handler: NewAPI(pm, cfg, shutdown),
	}

	go func() {
		log.Printf("rsupervisor HTTP :%s  TCP :%s", cfg.HTTPPort, cfg.TCPPort)
		if err := srv.ListenAndServe(); err != http.ErrServerClosed {
			log.Fatalf("http: %v", err)
		}
	}()

	if cfg.TCPPort != "" {
		go RunTCPStream(pm, cfg.TCPPort)
	}

	quit := make(chan os.Signal, 1)
	signal.Notify(quit, syscall.SIGTERM, syscall.SIGINT)
	select {
	case <-quit:
	case <-shutdown:
	}

	log.Println("shutting down...")
	pm.StopAll()

	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer cancel()
	if err := srv.Shutdown(ctx); err != nil {
		log.Printf("http shutdown: %v", err)
	}
	log.Println("done")
}
