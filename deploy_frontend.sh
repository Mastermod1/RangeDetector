#!/usr/bin/bash

cd frontend

npx tailwindcss -o output.css --minify

cp main.html ../data
cp jquery-3.7.1.min.js ../data
cp output.css ../data

cd ..

#pio run --target upload
