/**
 * @file display.h
 * @brief Local status display + 5 touch buttons (CONTRACT §13, Sprint S4).
 *
 * Runs in its own low-priority task on Core 1 (DISPLAY_PERIOD_MS), reads only
 * snapshotGet() (and wifiGetIp() for the IP line), sends commands only via
 * cmdQueueSend() with source = LOCAL. It never calls logic or HAL functions.
 */
#pragma once

bool displayStart();   ///< init LGFX, start display task; false if panel init failed
