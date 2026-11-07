#!/usr/bin/env python3
"""embed_app.py – copies data/www/app.js into data/www/index.html and index.html to web-ui/.

data/www/index.html carries a byte-identical copy of data/www/app.js between the lines
"/* ===== ADDAPP: copy of data/www/app.js ... ===== */" and "/* ===== end of ADDAPP ===== */"
(checked by test/js/test_pages.js). After every change of app.js run:

    python3 scripts/embed_app.py

It replaces only the text between the two marker lines ("</script>" written as "<\\/script>") and
then copies index.html to web-ui/ims-demo-ui.html. Prints "embedded <n> bytes" or an error.
"""
import os, sys

REPO = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
APP = os.path.join(REPO, 'data', 'www', 'app.js')
INDEX = os.path.join(REPO, 'data', 'www', 'index.html')
COPY = os.path.join(REPO, 'web-ui', 'ims-demo-ui.html')
START = '/* ===== ADDAPP: copy of data/www/app.js (byte for byte, checked by test/js/test_pages.js) ===== */\n'
END = '/* ===== end of ADDAPP ===== */\n'


def main():
    with open(APP, encoding='utf-8', newline='') as f:
        app = f.read().replace('\r\n', '\n').replace('</script>', '<\\/script>')
    with open(INDEX, encoding='utf-8', newline='') as f:
        html = f.read().replace('\r\n', '\n')
    if html.count(START) != 1 or html.count(END) != 1 or html.index(START) > html.index(END):
        sys.exit('error: marker lines not found exactly once in data/www/index.html')
    a = html.index(START) + len(START)
    b = html.index(END)
    if not app.endswith('\n'):
        app += '\n'
    html = html[:a] + app + html[b:]
    for path in (INDEX, COPY):
        with open(path, 'w', encoding='utf-8', newline='') as f:
            f.write(html)
    print('embedded %d bytes' % len(app))


if __name__ == '__main__':
    main()
