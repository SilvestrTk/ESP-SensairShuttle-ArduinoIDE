/*
 * AppConfig.h — all user and compile configuration for the Alarm Device
 * in one place. Edit here, not in the modules.
 */
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* ==========================================================================
 * USER CONFIGURATION (defaults — most can be changed on-device and are
 * then stored in NVM, which takes precedence over these values)
 * ========================================================================*/

/* Identity + remote server (IFTTT webhook by default) */
#define APP_DEVICE_ID_DEFAULT  "Sensor 001"
#define APP_SERVER_URL_DEFAULT \
  "https://maker.ifttt.com/trigger/alarm_triggered/with/key/XXXX"

/* Default alarm thresholds (alarms themselves default to OFF) */
#define APP_DEF_TEMP_THRESH    35.0f    /* deg C, upper                     */
#define APP_DEF_HUM_THRESH     70.0f    /* % RH, upper                      */
#define APP_DEF_AIR_THRESH     150.0f   /* IAQ index 0..500, higher = worse */
#define APP_DEF_VIB_THRESH     100.0f   /* mg (std-dev of acceleration)     */
#define APP_DEF_SOUND_THRESH   60.0f    /* mV RMS from the microphone       */
#define APP_DEF_RESET_MIN      5        /* remote alarm re-arm period [min] */

/* Measurement scheduling */
#define APP_ENV_PERIOD_MS      3000     /* BME690 forced-mode read interval */
#define APP_SOUND_PERIOD_MS    400      /* microphone level read interval   */
#define APP_SOUND_WINDOW_MS    20       /* microphone sampling window       */
#define APP_VIB_SAMPLE_MS      20       /* accelerometer sampling tick      */
#define APP_VIB_WINDOW_MS      2000     /* vibration std-dev window         */
#define APP_IAQ_WARMUP_MS      60000UL  /* gas sensor burn-in before IAQ    */

/* Network behaviour */
#define APP_RETRY_MINUTES      3        /* unsent-alarm retry interval      */
#define APP_RECONNECT_S        30       /* WiFi reconnect attempt interval  */
#define APP_WIFI_TIMEOUT_MS    15000
#define APP_HTTP_TIMEOUT_MS    8000
#define APP_PENDING_MAX        8        /* unsent alarms kept in NVM        */

/* Local alarm (siren) sound — two alternating tones */
#define APP_SIREN_FREQ_A       1000
#define APP_SIREN_FREQ_B       1500
#define APP_SIREN_BEEP_MS      150
#define APP_SIREN_GAP_MS       450
#define APP_SIREN_VOLUME       90       /* 0..100 */

/* ==========================================================================
 * COMPILE CONFIGURATION
 * ========================================================================*/

#define APP_DEBUG_LEVEL        3        /* 0 off, 1 error, 2 warn, 3 info, 4 verbose */
#define APP_WATCHDOG_S         30       /* task watchdog timeout, 0 = disabled       */
#define APP_UI_REFRESH_MS      1000     /* main screen value refresh                 */

/*
 * Build the self-test suite instead of the application:
 * set to 1, upload, open Serial Monitor for PASS/FAIL report.
 */
#define APP_RUN_TESTS          0

#endif /* APP_CONFIG_H */
