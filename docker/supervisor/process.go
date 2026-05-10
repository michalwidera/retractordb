package main

import (
	"bufio"
	"fmt"
	"io"
	"log"
	"os/exec"
	"strings"
	"sync"
	"syscall"
	"time"
)

const maxLogLines = 500

type LogEntry struct {
	T       time.Time
	Process string
	Line    string
}

type retractorProc struct {
	cmd  *exec.Cmd
	done chan struct{}
}

type ProcessManager struct {
	cfg Config
	mu  sync.Mutex
	rp  *retractorProc
	log []LogEntry
}

func NewProcessManager(cfg Config) *ProcessManager {
	return &ProcessManager{cfg: cfg}
}

func (pm *ProcessManager) StartRetractor(queryFile string) error {
	pm.mu.Lock()
	if pm.rp != nil {
		pm.mu.Unlock()
		return fmt.Errorf("xretractor already running")
	}

	cmd := exec.Command(pm.cfg.Xretractor, "--queryfile", queryFile)
	stdout, err := cmd.StdoutPipe()
	if err != nil {
		pm.mu.Unlock()
		return err
	}
	stderr, err := cmd.StderrPipe()
	if err != nil {
		pm.mu.Unlock()
		return err
	}
	if err := cmd.Start(); err != nil {
		pm.mu.Unlock()
		return fmt.Errorf("start xretractor: %w", err)
	}

	done := make(chan struct{})
	pm.rp = &retractorProc{cmd: cmd, done: done}
	pm.cfg.QueryFile = queryFile
	pm.mu.Unlock()

	go pm.scanLines("xretractor", stdout)
	go pm.scanLines("xretractor", stderr)
	go func() {
		cmd.Wait()
		close(done)
		pm.mu.Lock()
		if pm.rp != nil && pm.rp.done == done {
			pm.rp = nil
		}
		pm.mu.Unlock()
		pm.addLog("xretractor", "process exited")
	}()

	log.Printf("xretractor started: %s", queryFile)
	return nil
}

func (pm *ProcessManager) StopRetractor() error {
	pm.mu.Lock()
	rp := pm.rp
	pm.rp = nil
	pm.mu.Unlock()

	if rp == nil {
		return fmt.Errorf("xretractor not running")
	}

	exec.Command(pm.cfg.Xqry, "-k").Run()
	select {
	case <-rp.done:
		return nil
	case <-time.After(5 * time.Second):
	}
	rp.cmd.Process.Signal(syscall.SIGTERM)
	select {
	case <-rp.done:
	case <-time.After(5 * time.Second):
		rp.cmd.Process.Kill()
		<-rp.done
	}
	return nil
}

func (pm *ProcessManager) ReloadRetractor(queryFile string) error {
	_ = pm.StopRetractor()
	return pm.StartRetractor(queryFile)
}

func (pm *ProcessManager) Running() bool {
	pm.mu.Lock()
	defer pm.mu.Unlock()
	return pm.rp != nil
}

func (pm *ProcessManager) CurrentFile() string {
	pm.mu.Lock()
	defer pm.mu.Unlock()
	return pm.cfg.QueryFile
}

func (pm *ProcessManager) RunXqry(args ...string) (string, error) {
	return pm.runAndLog("xqry", pm.cfg.Xqry, args)
}

func (pm *ProcessManager) XqryBin() string { return pm.cfg.Xqry }

func (pm *ProcessManager) StopAll() { _ = pm.StopRetractor() }

func (pm *ProcessManager) Logs() []LogEntry {
	pm.mu.Lock()
	defer pm.mu.Unlock()
	cp := make([]LogEntry, len(pm.log))
	copy(cp, pm.log)
	return cp
}

func (pm *ProcessManager) addLog(process, line string) {
	pm.mu.Lock()
	defer pm.mu.Unlock()
	pm.log = append(pm.log, LogEntry{time.Now(), process, line})
	if len(pm.log) > maxLogLines {
		pm.log = pm.log[len(pm.log)-maxLogLines:]
	}
}

func (pm *ProcessManager) runAndLog(name, bin string, args []string) (string, error) {
	out, err := exec.Command(bin, args...).CombinedOutput()
	s := string(out)
	for _, line := range strings.Split(strings.TrimRight(s, "\n"), "\n") {
		if line != "" {
			pm.addLog(name, line)
		}
	}
	return s, err
}

func (pm *ProcessManager) scanLines(name string, r io.Reader) {
	sc := bufio.NewScanner(r)
	for sc.Scan() {
		line := sc.Text()
		log.Printf("[%s] %s", name, line)
		pm.addLog(name, line)
	}
}
