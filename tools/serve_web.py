"""Serves the browser build (build/web/site) to phones on this network: scripts/serve-web.ps1 runs it.

    python -I -B tools/serve_web.py [--port 8091] [--root build/web/site]

Every answer says no-cache, so a phone that kept an older index.js never runs it against a newer index.wasm.
"""
import argparse
import functools
import http.server
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


class Handler(http.server.SimpleHTTPRequestHandler):
    extensions_map = {
        **http.server.SimpleHTTPRequestHandler.extensions_map,
        ".wasm": "application/wasm",
        ".js": "text/javascript",
        ".webmanifest": "application/manifest+json",
    }

    def end_headers(self):
        self.send_header("Cache-Control", "no-cache")
        super().end_headers()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", type=int, default=8091)
    ap.add_argument("--root", default=os.path.join(ROOT, "build", "web", "site"))
    a = ap.parse_args()
    if not os.path.isfile(os.path.join(a.root, "index.html")):
        raise SystemExit(f"no index.html in {a.root}: run scripts\\build-web.ps1 first")
    server = http.server.ThreadingHTTPServer(("0.0.0.0", a.port), functools.partial(Handler, directory=a.root))
    print(f"[serve] {a.root} on port {a.port}; Ctrl+C stops it", flush=True)
    server.serve_forever()


if __name__ == "__main__":
    main()
