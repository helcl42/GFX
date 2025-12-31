#!/bin/bash

# Simple HTTP server script for testing the web example

PORT=8000

if [ -d "web_build" ]; then
    echo "Starting HTTP server on port $PORT..."
    echo "Open your browser to: http://localhost:$PORT/"
    echo ""
    echo "Make sure your browser supports WebGPU!"
    echo "Check: https://webgpureport.org/"
    echo ""
    echo "Press Ctrl+C to stop the server."
    echo ""
    
    # Try python3 first, then python
    if command -v python3 &> /dev/null; then
        python3 -m http.server $PORT -d web_build
    elif command -v python &> /dev/null; then
        python -m http.server $PORT -d web_build
    else
        echo "Error: Python not found. Please install Python to run the HTTP server."
        echo "Alternatively, use any other HTTP server like:"
        echo "  npx http-server web_build -p $PORT"
        exit 1
    fi
else
    echo "Error: web_build directory not found!"
    echo "Please run ./build_web.sh first to build the web example."
    exit 1
fi
