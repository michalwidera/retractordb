from fractions import Fraction
import re

from .models import Error, ReadTimeout, Record, Schema, Stream
from .subscription import Process


class Subscription:
    def __init__(self, owner, stream, limit, idle_timeout, capacity):
        self._owner = owner
        self._process = Process(owner._args("--select", stream, "--elimitqry", str(limit),
                                           "--idle-timeout", str(round(idle_timeout * 1000))), capacity)
        self.end_reason = None
        self.closed = False
        try:
            event = self._process.read(owner.timeout)
            if event.get("event") != "schema":
                raise ValueError("Expected schema event")
            self.schema = Schema.from_event(event)
            if self.schema.stream != stream:
                raise ValueError("Unexpected schema stream")
        except BaseException as exc:
            self.close()
            if isinstance(exc, Exception) and not isinstance(exc, Error):
                raise Error("protocol_error", str(exc)) from exc
            raise

    @property
    def pid(self):
        return self._process.child.pid

    @property
    def returncode(self):
        return self._process.child.poll()

    def next(self, timeout=None):
        if self.closed:
            return None
        try:
            event = self._process.read(timeout)
            if event.get("event") == "end":
                self.end_reason = event["reason"]
                self._process.complete(self._owner.timeout)
                self.close()
                return None
            if event.get("event") != "record":
                raise ValueError("Expected record or end event")
            return Record.from_event(event, self.schema)
        except ReadTimeout:
            raise
        except Exception as exc:
            self.close()
            if isinstance(exc, Error):
                raise
            raise Error("protocol_error", str(exc)) from exc

    def close(self):
        if not self.closed:
            self.closed = True
            self._process.close()
            self._owner._subscriptions.discard(self)

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()

    def __iter__(self):
        return self

    def __next__(self):
        result = self.next()
        if result is None:
            raise StopIteration
        return result


class Client:
    def __init__(self, server, *, xqry="xqry", timeout=5.0):
        if not re.fullmatch(r"[a-z][a-z0-9_-]{0,31}", server):
            raise ValueError("server must be an explicit xretractor instance name")
        if timeout <= 0:
            raise ValueError("timeout must be positive")
        self.server, self.xqry, self.timeout = server, str(xqry), timeout
        self._subscriptions = set()
        self.closed = False

    def _args(self, *args):
        if self.closed:
            raise Error("closed", "Client is closed")
        return [self.xqry, "--server", self.server, "--jsonl", *args]

    def _command(self, expected, *args):
        process = Process(self._args(*args), 16)
        try:
            result = process.read(self.timeout)
            if result.get("event") != expected:
                raise Error("protocol_error", "Unexpected command response")
            process.complete(self.timeout)
            return result
        finally:
            process.close()

    def ping(self):
        self._command("pong", "--hello")
        return True

    def streams(self):
        result = self._command("streams", "--dir")
        try:
            return [Stream(s["name"], Fraction(s["delta"])) for s in result["streams"]]
        except (KeyError, TypeError, ValueError) as exc:
            raise Error("protocol_error", str(exc)) from exc

    def describe(self, stream):
        try:
            return Schema.from_event(self._command("schema", "--detail", stream))
        except (KeyError, TypeError, ValueError) as exc:
            raise Error("protocol_error", str(exc)) from exc

    def subscribe(self, stream, *, limit=0, idle_timeout=0.0, capacity=1024):
        if not isinstance(limit, int) or limit < 0 or idle_timeout < 0:
            raise ValueError("limit and idle_timeout must be nonnegative")
        result = Subscription(self, stream, limit, idle_timeout, capacity)
        self._subscriptions.add(result)
        return result

    def close(self):
        self.closed = True
        for subscription in tuple(self._subscriptions):
            subscription.close()

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()
