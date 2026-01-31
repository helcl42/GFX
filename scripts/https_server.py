#!/usr/bin/env python3
import http.server
import ssl
import sys
import os

# Get directory to serve from command line argument, default to current directory
serve_dir = sys.argv[1] if len(sys.argv) > 1 else '.'
serve_dir = os.path.abspath(serve_dir)

# Change to the directory to serve
os.chdir(serve_dir)

server_address = ('0.0.0.0', 8443)
httpd = http.server.HTTPServer(server_address, http.server.SimpleHTTPRequestHandler)

# Load SSL certificate (always from the script's directory)
script_dir = os.path.dirname(os.path.abspath(__file__))
cert_file = os.path.join(script_dir, 'cert.pem')
key_file = os.path.join(script_dir, 'key.pem')

context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
context.load_cert_chain(cert_file, key_file)
httpd.socket = context.wrap_socket(httpd.socket, server_side=True)

print(f"Serving HTTPS from: {serve_dir}")
print(f"Server running at https://{server_address[0]}:{server_address[1]}/")
httpd.serve_forever()
