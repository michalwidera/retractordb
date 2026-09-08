import argparse

from retractordb import Client, Error

parser = argparse.ArgumentParser()
parser.add_argument("server")
parser.add_argument("stream")
parser.add_argument("--xqry", default="xqry")
parser.add_argument("--limit", type=int, default=0)
args = parser.parse_args()

try:
    with Client(args.server, xqry=args.xqry) as db:
        with db.subscribe(args.stream, limit=args.limit) as samples:
            for sample in samples:
                print(sample.values, flush=True)
except KeyboardInterrupt:
    pass
except Error as exc:
    parser.exit(1, f"{exc.code}: {exc}\n")
