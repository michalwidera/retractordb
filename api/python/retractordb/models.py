from dataclasses import dataclass
from fractions import Fraction


class Error(RuntimeError):
    def __init__(self, code, message):
        self.code = code
        super().__init__(message)


class ReadTimeout(Error, TimeoutError):
    def __init__(self):
        super().__init__("read_timeout", "No event within the read timeout")


@dataclass(frozen=True)
class Stream:
    name: str
    delta: Fraction


@dataclass(frozen=True)
class Field:
    name: str
    type: str
    count: int


@dataclass(frozen=True)
class Schema:
    stream: str
    delta: Fraction
    query: str
    fields: tuple[Field, ...]

    @classmethod
    def from_event(cls, event):
        fields = tuple(Field(**f) for f in event["fields"])
        if any(f.count < 1 for f in fields) or len({f.name for f in fields}) != len(fields):
            raise ValueError("Invalid schema fields")
        return cls(event["stream"], Fraction(event["delta"]), event["query"], fields)


def _value(text, kind):
    if text is None:
        return None
    if not isinstance(text, str):
        raise ValueError("Expected a text value or null")
    if kind in ("BYTE", "INTEGER", "UINT"):
        return int(text)
    if kind in ("FLOAT", "DOUBLE"):
        return float(text)
    if kind == "RATIONAL":
        return Fraction(text)
    if kind == "INTPAIR":
        a, b = text.split(",")
        return int(a), int(b)
    if kind == "IDXPAIR":
        a, b = text.rsplit("[", 1)
        if not b.endswith("]"):
            raise ValueError("Invalid index pair")
        return a, int(b[:-1])
    if kind == "STRING":
        return text
    raise ValueError("Unsupported field type: " + kind)


@dataclass(frozen=True)
class Record:
    stream: str
    values: dict

    def __getitem__(self, name):
        return self.values[name]

    @classmethod
    def from_event(cls, event, schema):
        raw = event["values"]
        if event["stream"] != schema.stream or len(raw) != sum(f.count for f in schema.fields):
            raise ValueError("Record does not match schema")
        values = {}
        offset = 0
        for field in schema.fields:
            items = [_value(v, field.type) for v in raw[offset:offset + field.count]]
            values[field.name] = items[0] if field.count == 1 else items
            offset += field.count
        return cls(schema.stream, values)
