import struct
import sys
from pathlib import Path


# Niezalezny od silnika odczyt formatu: 8 B naglowka, flaga, dwa size_t, bitset.
def null_masks(path):
    data = path.read_bytes()
    assert data[:8] == bytes(8), path
    entry = struct.Struct('=B' + ('Q' if struct.calcsize('P') == 8 else 'I') * 2 + 'B')
    assert (len(data) - 8) % entry.size == 0, path
    masks = []
    for gap, count, width, mask in entry.iter_unpack(data[8:]):
        assert gap == 0 and width == 2, (path, gap, width)
        masks.extend([mask] * count)
    return masks


root = Path(sys.argv[1])
storage = sys.argv[2]
expected = [[(None, 11), (22, None), (33, 44)], [(55, 66), (None, 77), (88, None)]]
for number, rows in enumerate(expected):
    suffix = f'.old{number}'
    data_path = root / ('result' + suffix)
    meta_path = root / ('result.meta' + suffix)
    values = list(struct.iter_unpack('=ii', data_path.read_bytes()))
    masks = null_masks(meta_path)
    assert len(values) == len(masks) == len(rows), (storage, number, values, masks)
    actual = [tuple(None if mask & (1 << field) else value for field, value in enumerate(record))
              for record, mask in zip(values, masks)]
    assert actual == rows, (storage, number, actual, rows)
    if storage in ('DEFAULT', 'POSIXSHD'):
        assert (root / ('result.shadow' + suffix)).is_file(), (storage, number)
    print(storage, number, 'values and NULL bits OK')

assert (root / 'counter').read_text().strip() == '2', storage
for name in ('result', 'result.meta', 'result.shadow', 'result.meta.shadow'):
    assert not (root / name).exists(), (storage, name, 'active file survived shutdown')
