#!/usr/bin/bash

# Start live-server in a new screen session named "live-server"
screen -dmS live-server bash -c 'npx live-server --no-css-inject'

# Start tailwindcss in a new screen session named "tailwind"
screen -dmS tailwind bash -c 'npx tailwindcss -i ./input.css -o ./output.css --watch'
