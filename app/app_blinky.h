#ifndef APP_BLINKY_H
#define APP_BLINKY_H

/**
 * @file app_blinky.h
 * @brief Blinky application — top-level application layer.
 *
 * Depends only on services/, never on platform/ or board/ directly.
 * Dependency rule: app → services → drivers → platform → board → vendor
 */

/**
 * @brief Initialise the blinky application.
 *        Must be called once after board_init().
 */
void app_blinky_init(void);

/**
 * @brief Run one iteration of the blinky loop.
 *        Call repeatedly from main().
 */
void app_blinky_run(void);

#endif /* APP_BLINKY_H */
