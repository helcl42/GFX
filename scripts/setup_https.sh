#!/bin/bash
# Setup HTTPS server for WebGPU testing on local network

# Get the local IP address
LOCAL_IP=$(hostname -I | awk '{print $1}')

echo "Local IP detected: $LOCAL_IP"
echo "Generating SSL certificate in scripts/ directory for $LOCAL_IP..."

# Generate self-signed certificate in the scripts directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"
openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem \
    -days 365 -nodes -subj "/CN=$LOCAL_IP" 2>/dev/null

echo "Certificate generated!"
echo ""
echo "To start the HTTPS server, run from project root:"
echo "  python3 scripts/https_server.py build_web/web"
echo ""
echo "Or from any directory:"
echo "  python3 $SCRIPT_DIR/https_server.py /path/to/serve"
echo ""
echo "Then access from your phone:"
echo "  https://$LOCAL_IP:8443/cube_example_cpp_web.html"
echo ""
echo "Note: You'll need to accept the certificate warning on your phone."
