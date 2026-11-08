#!/usr/bin/env python3
"""build_data.py – generated files in data/ (Contract V1.6.5, Sprint D1.2).

1. data/add.html from data/add.json (ADD spec §5.3): the same JSON twice – in
   <script type="application/ld+json"> with every "</" written as "<\\/", and readable in <pre> with
   & < > escaped. Until V1.6.4 the firmware built this page at runtime in small chunks (about 24 s for
   the 34 kB ADD document); now it is a file and is streamed in one piece.
2. a gzip copy <file>.gz of every larger text file the device serves (GZ below). The firmware sends
   the copy with Content-Encoding: gzip when the client accepts gzip, else the plain file.

Run after every change of add.json, openapi.json, doc.html, app.js or index.html (embed_app.py
calls it), then "pio run -e firmware -t uploadfs". test/js/test_add.js fails if a generated file
is out of date.

    python3 scripts/build_data.py
"""
import gzip, os

REPO = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
DATA = os.path.join(REPO, 'data')
GZ = ['www/index.html', 'www/app.js', 'doc.html', 'add.json', 'add.html', 'openapi.json']


def add_html(js):
    pre = js.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')
    return ('<!DOCTYPE html><html><head><meta charset="utf-8"><title>ADD</title>\n'
            '<script type="application/ld+json">\n' + js.replace('</', '<\\/') + '\n</script></head><body><pre>\n'
            + pre + '\n</pre></body></html>\n')


def write_if_changed(path, data):
    try:
        with open(path, 'rb') as f:
            if f.read() == data:
                return False
    except OSError:
        pass
    with open(path, 'wb') as f:
        f.write(data)
    return True


def main():
    with open(os.path.join(DATA, 'add.json'), 'rb') as f:
        js = f.read().decode('utf-8')
    changed = write_if_changed(os.path.join(DATA, 'add.html'), add_html(js).encode('utf-8'))
    print('add.html %s' % ('written' if changed else 'unchanged'))
    for name in GZ:
        src = os.path.join(DATA, name)
        with open(src, 'rb') as f:
            raw = f.read()
        old = None
        try:
            with open(src + '.gz', 'rb') as f:
                old = gzip.decompress(f.read())
        except OSError:
            pass
        if old == raw:                     # content unchanged: keep the file (no git noise)
            state = 'unchanged'
        else:
            with open(src + '.gz', 'wb') as f:
                f.write(gzip.compress(raw, compresslevel=9, mtime=0))
            state = 'written'
        print('%-16s %7d -> %6d bytes  %s' % (name, len(raw), os.path.getsize(src + '.gz'), state))


if __name__ == '__main__':
    main()
