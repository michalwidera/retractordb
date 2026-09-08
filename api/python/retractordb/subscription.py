import json
import queue
import subprocess
import threading
import time

from .models import Error, ReadTimeout


class Process:
    """Wlasny proces, ograniczone bufory i niezalezny odczyt obu potokow."""

    def __init__(self, args, capacity):
        if capacity < 1:
            raise ValueError("capacity must be positive")
        self.messages = queue.Queue(capacity)
        self.finished = threading.Event()
        self.error = None
        self.stderr = b""
        self.closed = False
        try:
            self.child = subprocess.Popen(args, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                                          stderr=subprocess.PIPE, start_new_session=True)
        except OSError as exc:
            raise Error("spawn_error", str(exc)) from exc
        self.threads = [threading.Thread(target=self._stdout, daemon=True),
                        threading.Thread(target=self._stderr, daemon=True)]
        try:
            for thread in self.threads:
                thread.start()
        except BaseException:
            self.close()
            raise

    def _stdout(self):
        try:
            while line := self.child.stdout.readline(1024 * 1024 + 1):
                if len(line) > 1024 * 1024 or not line.endswith(b"\n"):
                    raise Error("protocol_error", "Oversized or incomplete JSONL event")
                event = json.loads(line)
                if not isinstance(event, dict) or event.get("version") != 1:
                    raise Error("protocol_error", "Unsupported JSONL protocol version")
                try:
                    self.messages.put_nowait(event)
                except queue.Full:
                    raise Error("buffer_overflow", "Application is not consuming stream events fast enough") from None
        except Exception as exc:
            if not self.closed:
                self.error = exc if isinstance(exc, Error) else Error("protocol_error", str(exc))
                try:
                    self.child.terminate()
                except ProcessLookupError:
                    pass
        finally:
            self.finished.set()

    def _stderr(self):
        while chunk := self.child.stderr.read1(4096):
            self.stderr = (self.stderr + chunk)[-65536:]

    def read(self, timeout):
        if timeout is not None and timeout < 0:
            raise ValueError("timeout must be nonnegative or None")
        deadline = None if timeout is None else time.monotonic() + timeout
        while True:
            if self.error:
                raise self.error
            try:
                event = self.messages.get_nowait()
            except queue.Empty:
                if self.closed:
                    raise Error("closed", "Subscription is closed")
                if self.finished.is_set():
                    self.close()
                    raise Error("process_exit", f"xqry exited ({self.child.returncode}): " +
                                self.stderr.decode("utf-8", errors="replace"))
                if deadline is not None and time.monotonic() >= deadline:
                    raise ReadTimeout()
                self.finished.wait(0.005)
                continue
            if event.get("event") == "error":
                raise Error(event.get("code", "protocol_error"), event.get("message", "xqry error"))
            return event

    def complete(self, timeout):
        try:
            status = self.child.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            raise Error("process_timeout", "xqry did not exit after its final event") from None
        if status:
            raise Error("process_exit", f"xqry exited with status {status}")

    def close(self):
        if self.closed:
            return
        self.closed = True
        if self.child.poll() is None:
            self.child.terminate()
            try:
                self.child.wait(timeout=1)
            except subprocess.TimeoutExpired:
                self.child.kill()
                self.child.wait()
        for thread in self.threads:
            if thread.ident is not None:
                thread.join()
        self.child.stdout.close()
        self.child.stderr.close()
