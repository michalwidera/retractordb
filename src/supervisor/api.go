package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"net/http"
	"os/exec"
)

type apiHandler struct {
	pm  *ProcessManager
	cfg Config
}

func NewAPI(pm *ProcessManager, cfg Config) http.Handler {
	h := &apiHandler{pm: pm, cfg: cfg}
	mux := http.NewServeMux()

	mux.HandleFunc("GET /health", h.health)
	mux.HandleFunc("GET /api/queries", h.queries)
	mux.HandleFunc("GET /api/queries/yaml", h.queriesYAML)
	mux.HandleFunc("POST /api/reload", h.reload)
	mux.HandleFunc("POST /api/adhoc", h.adhoc)
	mux.HandleFunc("GET /api/stream/{name}", h.streamQuery)
	mux.HandleFunc("GET /api/logs", h.logs)
	mux.HandleFunc("GET /api/xtrdb", h.xtrdb)
	mux.HandleFunc("POST /api/stop", h.stop)
	mux.HandleFunc("GET /", h.ui)

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
	out, err := h.pm.RunXqry("--dir")
	if err != nil {
		writeJSON(w, http.StatusServiceUnavailable, map[string]string{"error": err.Error(), "output": out})
		return
	}
	writeJSON(w, http.StatusOK, map[string]string{"output": out})
}

func (h *apiHandler) queriesYAML(w http.ResponseWriter, r *http.Request) {
	out, err := h.pm.RunXqry("--diryaml")
	if err != nil {
		writeJSON(w, http.StatusServiceUnavailable, map[string]string{"error": err.Error(), "output": out})
		return
	}
	w.Header().Set("Content-Type", "application/x-yaml")
	fmt.Fprint(w, out)
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
// Killing xqry when client disconnects via CommandContext.
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

func (h *apiHandler) xtrdb(w http.ResponseWriter, r *http.Request) {
	args := r.URL.Query()["arg"]
	out, err := h.pm.RunXtrdb(args...)
	if err != nil {
		writeJSON(w, http.StatusInternalServerError, map[string]string{"error": err.Error(), "output": out})
		return
	}
	writeJSON(w, http.StatusOK, map[string]string{"output": out})
}

func (h *apiHandler) ui(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "text/html; charset=utf-8")
	fmt.Fprint(w, indexHTML)
}
