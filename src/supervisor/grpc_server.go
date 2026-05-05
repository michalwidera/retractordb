package main

import (
	"bufio"
	"context"
	"os/exec"

	pb "rsupervisor/pb"

	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
)

type grpcServer struct {
	pb.UnimplementedSupervisorServer
	pm       *ProcessManager
	cfg      Config
	shutdown chan<- struct{}
}

func NewGRPCServer(pm *ProcessManager, cfg Config, shutdown chan<- struct{}) *grpcServer {
	return &grpcServer{pm: pm, cfg: cfg, shutdown: shutdown}
}

func (s *grpcServer) Health(_ context.Context, _ *pb.Empty) (*pb.HealthResponse, error) {
	st := "ok"
	if !s.pm.Running() {
		st = "degraded"
	}
	return &pb.HealthResponse{
		Status:            st,
		XretractorRunning: s.pm.Running(),
		QueryFile:         s.pm.CurrentFile(),
	}, nil
}

func (s *grpcServer) ListQueries(_ context.Context, _ *pb.Empty) (*pb.TextResponse, error) {
	out, err := s.pm.RunXqry("--dir")
	if err != nil {
		return &pb.TextResponse{Error: err.Error(), Output: out}, nil
	}
	return &pb.TextResponse{Output: out}, nil
}

func (s *grpcServer) GetQueryTree(_ context.Context, req *pb.QueryTreeRequest) (*pb.TextResponse, error) {
	args := []string{"-c", "-d", "--queryfile", s.pm.CurrentFile()}
	if req.Fields {
		args = append(args, "-f")
	}
	if req.Tags {
		args = append(args, "-t")
	}
	if req.Streamprogs {
		args = append(args, "-s")
	}
	if req.Rules {
		args = append(args, "-u")
	}
	out, err := exec.Command(s.cfg.Xretractor, args...).CombinedOutput()
	if err != nil {
		return &pb.TextResponse{Error: err.Error(), Output: string(out)}, nil
	}
	return &pb.TextResponse{Output: string(out)}, nil
}

func (s *grpcServer) Reload(_ context.Context, req *pb.ReloadRequest) (*pb.StatusResponse, error) {
	if req.File == "" {
		return nil, status.Error(codes.InvalidArgument, "file required")
	}
	if err := s.pm.ReloadRetractor(req.File); err != nil {
		return &pb.StatusResponse{Status: "error", Error: err.Error()}, nil
	}
	return &pb.StatusResponse{Status: "reloaded", Output: req.File}, nil
}

func (s *grpcServer) Adhoc(_ context.Context, req *pb.AdhocRequest) (*pb.StatusResponse, error) {
	if req.Query == "" {
		return nil, status.Error(codes.InvalidArgument, "query required")
	}
	out, err := s.pm.RunXqry("--adhoc", req.Query)
	if err != nil {
		return &pb.StatusResponse{Status: "error", Output: out, Error: err.Error()}, nil
	}
	return &pb.StatusResponse{Status: "ok", Output: out}, nil
}

func (s *grpcServer) StopRetractor(_ context.Context, _ *pb.Empty) (*pb.StatusResponse, error) {
	out, err := s.pm.RunXqry("-k")
	if err != nil {
		return &pb.StatusResponse{Status: "error", Output: out, Error: err.Error()}, nil
	}
	return &pb.StatusResponse{Status: "stopped", Output: out}, nil
}

func (s *grpcServer) Shutdown(_ context.Context, _ *pb.Empty) (*pb.StatusResponse, error) {
	go func() { s.shutdown <- struct{}{} }()
	return &pb.StatusResponse{Status: "shutting down"}, nil
}

func (s *grpcServer) GetLogs(_ context.Context, _ *pb.Empty) (*pb.LogsResponse, error) {
	entries := s.pm.Logs()
	pbEntries := make([]*pb.LogEntry, len(entries))
	for i, e := range entries {
		pbEntries[i] = &pb.LogEntry{
			Time:    e.T.Format("15:04:05.000"),
			Process: e.Process,
			Line:    e.Line,
		}
	}
	return &pb.LogsResponse{Entries: pbEntries}, nil
}

func (s *grpcServer) StreamData(req *pb.StreamRequest, stream pb.Supervisor_StreamDataServer) error {
	if req.Name == "" {
		return status.Error(codes.InvalidArgument, "name required")
	}
	cmd := exec.CommandContext(stream.Context(), s.pm.XqryBin(), "--select", req.Name, "--raw")
	stdout, err := cmd.StdoutPipe()
	if err != nil {
		return status.Errorf(codes.Internal, "pipe: %v", err)
	}
	if err := cmd.Start(); err != nil {
		return status.Errorf(codes.Internal, "start xqry: %v", err)
	}
	defer cmd.Wait()

	sc := bufio.NewScanner(stdout)
	for sc.Scan() {
		if err := stream.Send(&pb.DataChunk{Line: sc.Text()}); err != nil {
			return err
		}
	}
	return nil
}
