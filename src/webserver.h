#pragma once
// ============================================================
//  webserver.h — Async HTTP server route setup
// ============================================================

#include "globals.h"

// Registers GET /, GET /status, POST /save route handlers and
// calls server.begin(). The serverStarted flag prevents double-init.
// AP must be up before calling this.
void setupWebServer();
