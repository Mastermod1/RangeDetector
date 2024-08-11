#!/usr/bin/bash

# Stop live-server
screen -S live-server -X quit

# Stop tailwindcss
screen -S tailwind -X quit
