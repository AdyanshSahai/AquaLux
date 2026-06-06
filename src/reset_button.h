#pragma once
// ============================================================
//  reset_button.h — Physical factory-reset button handler
// ============================================================

#include "globals.h"

// Non-blocking hold-detection for RESET_PIN.
// Hold >= RESET_HOLD_MS → wipe NVS → confirmation beep → reboot.
// Must be called every loop iteration (highest priority check).
void handleResetButton();
