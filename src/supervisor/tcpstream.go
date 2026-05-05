package main

import (
	"bufio"
	"fmt"
	"log"
	"net"
	"os/exec"
)

// RunTCPStream listens on port. Each connection reads a stream name then pipes
// xqry --select <name> --raw output back. Compatible with: nc <host> <port>.
func RunTCPStream(pm *ProcessManager, port string) {
	ln, err := net.Listen("tcp", ":"+port)
	if err != nil {
		log.Printf("tcp stream listen :%s: %v", port, err)
		return
	}
	log.Printf("rsupervisor TCP :%s", port)

	for {
		conn, err := ln.Accept()
		if err != nil {
			log.Printf("tcp accept: %v", err)
			continue
		}
		go handleTCPConn(pm, conn)
	}
}

func handleTCPConn(pm *ProcessManager, conn net.Conn) {
	defer conn.Close()

	fmt.Fprint(conn, "stream> ")
	sc := bufio.NewScanner(conn)
	if !sc.Scan() {
		return
	}
	name := sc.Text()
	if name == "" {
		fmt.Fprintln(conn, "error: empty stream name")
		return
	}

	cmd := exec.Command(pm.XqryBin(), "--select", name, "--raw")
	cmd.Stdout = conn
	cmd.Stderr = conn
	if err := cmd.Run(); err != nil {
		fmt.Fprintf(conn, "\nerror: %v\n", err)
	}
}
