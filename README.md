# Real Estate Management System — DSA Project

A complete college mini-project using **C + C++ + HTML + CSS + JavaScript**. The C reporting utility reads the backend's property database and prints portfolio totals.

## DSA mapping
- **Linked List:** property records
- **Queue:** customer requests (FIFO)
- **Stack:** booking history (LIFO)
- **Hash Table:** fast Property ID search (average O(1))

## Project structure
```text
real_estate_management_system/
├── backend/main.cpp
├── backend/report.c
├── frontend/index.html
├── frontend/style.css
├── frontend/app.js
└── README.md
```

## Run backend

### Windows / MinGW
Open a terminal in `backend`:
```bash
g++ -std=c++17 -O2 main.cpp -o real_estate_server.exe -lws2_32
real_estate_server.exe
```

### Linux / macOS
```bash
g++ -std=c++17 -O2 main.cpp -o real_estate_server
./real_estate_server
```

Backend: `http://localhost:8080`

## Run frontend
Use VS Code Live Server on `frontend/index.html`, or any static HTTP server. Keep the C++ backend running.

## Demo flow
1. Open Dashboard.
2. Go to Properties and search `RE101` to demonstrate the hash table.
3. Add a property to demonstrate linked-list insertion.
4. Add customer requests to demonstrate queue/FIFO.
5. Book a property to demonstrate stack push.
6. Cancel the latest booking to demonstrate stack pop.

The backend creates sample property data on first run and stores property data in `properties.db`.

## Run the C report

Open a second terminal in `backend` after the backend has created `properties.db`:
```bash
gcc -std=c11 -Wall -Wextra -pedantic report.c -o property_report.exe
property_report.exe
```

The report accepts an optional database path, for example `property_report.exe properties.db`.

## API
```text
GET    /api/health
GET    /api/properties
POST   /api/properties
GET    /api/properties/:id
PUT    /api/properties/:id
DELETE /api/properties/:id
GET    /api/search?id=RE101
GET    /api/requests
POST   /api/requests
POST   /api/requests/process
GET    /api/bookings
POST   /api/bookings
POST   /api/bookings/cancel-latest
```

This is intentionally a simple academic HTTP server so the DSA code stays visible. For production software, use a mature HTTP framework, database, authentication, validation, and HTTPS.
