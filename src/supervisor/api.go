package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"net/http"
	"os"
	"os/exec"
	"path/filepath"
	"strings"

	"gopkg.in/yaml.v3"
)

type apiHandler struct {
	pm       *ProcessManager
	cfg      Config
	shutdown chan<- struct{}
}

func NewAPI(pm *ProcessManager, cfg Config, shutdown chan<- struct{}) http.Handler {
	h := &apiHandler{pm: pm, cfg: cfg, shutdown: shutdown}
	mux := http.NewServeMux()

	mux.HandleFunc("GET /health", h.health)
	mux.HandleFunc("GET /api/queries", h.queries)
	mux.HandleFunc("POST /api/reload", h.reload)
	mux.HandleFunc("POST /api/adhoc", h.adhoc)
	mux.HandleFunc("GET /api/stream/{name}", h.streamQuery)
	mux.HandleFunc("GET /api/logs", h.logs)
	mux.HandleFunc("POST /api/stop", h.stop)
	mux.HandleFunc("POST /api/shutdown", h.shutdownHandler)
	mux.HandleFunc("GET /api/querytree", h.queryTree)
	mux.HandleFunc("POST /api/file", h.fileWrite)
	mux.HandleFunc("GET /api/schema/{name}", h.schema)

	return mux
}

func writeJSON(w http.ResponseWriter, code int, v any) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(code)
	json.NewEncoder(w).Encode(v)
}

func (h *apiHandler) health(w http.ResponseWriter, r *http.Request) {
	status := "ok"
	if !h.pm.Running() {
		status = "degraded"
	}
	writeJSON(w, http.StatusOK, map[string]any{
		"status":     status,
		"xretractor": h.pm.Running(),
		"queryFile":  h.pm.CurrentFile(),
	})
}

func (h *apiHandler) queries(w http.ResponseWriter, r *http.Request) {
	out, err := h.pm.RunXqry("--diryaml")
	if err != nil {
		writeJSON(w, http.StatusServiceUnavailable, map[string]string{"error": err.Error(), "output": out})
		return
	}
	var data any
	if err := yaml.Unmarshal([]byte(out), &data); err != nil {
		writeJSON(w, http.StatusInternalServerError, map[string]string{"error": err.Error(), "output": out})
		return
	}
	writeJSON(w, http.StatusOK, data)
}

func (h *apiHandler) reload(w http.ResponseWriter, r *http.Request) {
	var body struct {
		File string `json:"file"`
	}
	if err := json.NewDecoder(r.Body).Decode(&body); err != nil || body.File == "" {
		writeJSON(w, http.StatusBadRequest, map[string]string{"error": "field 'file' required"})
		return
	}
	if err := h.pm.ReloadRetractor(body.File); err != nil {
		writeJSON(w, http.StatusInternalServerError, map[string]string{"error": err.Error()})
		return
	}
	writeJSON(w, http.StatusOK, map[string]string{"status": "reloaded", "file": body.File})
}

func (h *apiHandler) adhoc(w http.ResponseWriter, r *http.Request) {
	var body struct {
		Query string `json:"query"`
	}
	if err := json.NewDecoder(r.Body).Decode(&body); err != nil || body.Query == "" {
		writeJSON(w, http.StatusBadRequest, map[string]string{"error": "field 'query' required"})
		return
	}
	out, err := h.pm.RunXqry("--adhoc", body.Query)
	if err != nil {
		writeJSON(w, http.StatusInternalServerError, map[string]string{"error": err.Error(), "output": out})
		return
	}
	writeJSON(w, http.StatusOK, map[string]string{"output": out})
}

// streamQuery streams xqry --select <name> output as Server-Sent Events.
func (h *apiHandler) streamQuery(w http.ResponseWriter, r *http.Request) {
	name := r.PathValue("name")
	flusher, ok := w.(http.Flusher)
	if !ok {
		http.Error(w, "streaming not supported", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "text/event-stream")
	w.Header().Set("Cache-Control", "no-cache")
	w.Header().Set("Connection", "keep-alive")

	cmd := exec.CommandContext(r.Context(), h.pm.XqryBin(), "--select", name, "--raw")
	stdout, err := cmd.StdoutPipe()
	if err != nil {
		fmt.Fprintf(w, "event: error\ndata: %s\n\n", err)
		return
	}
	if err := cmd.Start(); err != nil {
		fmt.Fprintf(w, "event: error\ndata: %s\n\n", err)
		return
	}
	defer cmd.Wait()

	sc := bufio.NewScanner(stdout)
	for sc.Scan() {
		fmt.Fprintf(w, "data: %s\n\n", sc.Text())
		flusher.Flush()
	}
}

func (h *apiHandler) logs(w http.ResponseWriter, r *http.Request) {
	entries := h.pm.Logs()
	type logLine struct {
		T       string `json:"t"`
		Process string `json:"process"`
		Line    string `json:"line"`
	}
	out := make([]logLine, len(entries))
	for i, e := range entries {
		out[i] = logLine{e.T.Format("15:04:05.000"), e.Process, e.Line}
	}
	writeJSON(w, http.StatusOK, out)
}

func (h *apiHandler) stop(w http.ResponseWriter, r *http.Request) {
	out, err := h.pm.RunXqry("-k")
	if err != nil {
		writeJSON(w, http.StatusInternalServerError, map[string]string{"error": err.Error(), "output": out})
		return
	}
	writeJSON(w, http.StatusOK, map[string]string{"status": "stopped", "output": out})
}

func (h *apiHandler) queryTree(w http.ResponseWriter, r *http.Request) {
	q := r.URL.Query()
	args := []string{"-c", "-d", "--queryfile", h.pm.CurrentFile()}
	if q.Get("fields") == "1" {
		args = append(args, "-f")
	}
	if q.Get("tags") == "1" {
		args = append(args, "-t")
	}
	if q.Get("streamprogs") == "1" {
		args = append(args, "-s")
	}
	if q.Get("rules") == "1" {
		args = append(args, "-u")
	}
	out, err := exec.Command(h.cfg.Xretractor, args...).CombinedOutput()
	if err != nil {
		writeJSON(w, http.StatusInternalServerError, map[string]string{"error": err.Error(), "output": string(out)})
		return
	}
	writeJSON(w, http.StatusOK, map[string]string{"dot": string(out)})
}

func (h *apiHandler) shutdownHandler(w http.ResponseWriter, r *http.Request) {
	writeJSON(w, http.StatusOK, map[string]string{"status": "shutting down"})
	go func() { h.shutdown <- struct{}{} }()
}

func rqlPath(p string) (string, error) {
	if !strings.HasSuffix(p, ".rql") {
		return "", fmt.Errorf("only .rql files allowed")
	}
	clean := filepath.Clean(p)
	if strings.Contains(clean, "..") {
		return "", fmt.Errorf("path traversal not allowed")
	}
	return clean, nil
}

func (h *apiHandler) schema(w http.ResponseWriter, r *http.Request) {
	name := r.PathValue("name")
	out, err := h.pm.RunXqry("--detail", name)
	if err != nil {
		writeJSON(w, http.StatusServiceUnavailable, map[string]string{"error": err.Error(), "output": out})
		return
	}
	var parsed struct {
		Fields map[string]struct {
			Type string `yaml:"type"`
		} `yaml:"fields"`
	}
	if err := yaml.Unmarshal([]byte(out), &parsed); err != nil {
		writeJSON(w, http.StatusInternalServerError, map[string]string{"error": err.Error()})
		return
	}
	type fieldInfo struct {
		Name string `json:"name"`
		Type string `json:"type"`
	}
	fields := make([]fieldInfo, 0, len(parsed.Fields))
	for name, info := range parsed.Fields {
		fields = append(fields, fieldInfo{Name: name, Type: info.Type})
	}
	writeJSON(w, http.StatusOK, fields)
}

func (h *apiHandler) fileWrite(w http.ResponseWriter, r *http.Request) {
	var body struct {
		Path    string `json:"path"`
		Content string `json:"content"`
	}
	if err := json.NewDecoder(r.Body).Decode(&body); err != nil {
		writeJSON(w, http.StatusBadRequest, map[string]string{"error": err.Error()})
		return
	}
	path, err := rqlPath(body.Path)
	if err != nil {
		writeJSON(w, http.StatusBadRequest, map[string]string{"error": err.Error()})
		return
	}
	if err := os.WriteFile(path, []byte(body.Content), 0644); err != nil {
		writeJSON(w, http.StatusInternalServerError, map[string]string{"error": err.Error()})
		return
	}
	writeJSON(w, http.StatusOK, map[string]string{"status": "saved", "path": path})
}
