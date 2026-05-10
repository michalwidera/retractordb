package main

import (
	"context"
	"log"
	"net"
	"net/http"
	"os"
	"os/signal"
	"syscall"
	"time"

	pb "rsupervisor/pb"

	"google.golang.org/grpc"
)

type Config struct {
	QueryFile  string
	HTTPPort   string
	GRPCPort   string
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
		GRPCPort:   envOr("RDB_GRPC_PORT", "50051"),
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

	// REST
	httpSrv := &http.Server{
		Addr:    ":" + cfg.HTTPPort,
		Handler: NewAPI(pm, cfg, shutdown),
	}
	go func() {
		log.Printf("rsupervisor REST :%s", cfg.HTTPPort)
		if err := httpSrv.ListenAndServe(); err != http.ErrServerClosed {
			log.Fatalf("http: %v", err)
		}
	}()

	// gRPC
	grpcLis, err := net.Listen("tcp", ":"+cfg.GRPCPort)
	if err != nil {
		log.Fatalf("grpc listen: %v", err)
	}
	grpcSrv := grpc.NewServer()
	pb.RegisterSupervisorServer(grpcSrv, NewGRPCServer(pm, cfg, shutdown))
	go func() {
		log.Printf("rsupervisor gRPC :%s", cfg.GRPCPort)
		if err := grpcSrv.Serve(grpcLis); err != nil {
			log.Fatalf("grpc: %v", err)
		}
	}()

	// TCP stream
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

	grpcSrv.GracefulStop()

	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer cancel()
	if err := httpSrv.Shutdown(ctx); err != nil {
		log.Printf("http shutdown: %v", err)
	}
	log.Println("done")
}
