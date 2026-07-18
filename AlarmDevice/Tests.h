/*
 * Tests.h — self-test suite of the Alarm Device.
 *
 * Enable with APP_RUN_TESTS 1 in AppConfig.h, upload, open the Serial
 * Monitor: every test prints PASS/FAIL and a summary follows. Runs on
 * the device (uses real NVS in a scratch namespace) but needs no
 * sensors, display or network.
 */
#ifndef TESTS_H
#define TESTS_H

namespace Tests {
void run();
}

#endif /* TESTS_H */
