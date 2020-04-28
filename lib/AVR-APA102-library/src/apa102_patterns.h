#ifndef APA102_EFFECTS_H
#define APA102_EFFECTS_H

#include <util/delay.h>
#include <stdbool.h>

#include "apa102.h"

// The total number of steps the current pattern has.
extern uint8_t total_pattern_steps;
// The current step of the pattern the player is on.
extern uint8_t current_pattern_step;
// Are we playing a series of patterns?
extern bool playing_pattern_series;
// The total number of patterns the series has.
extern uint8_t total_series_steps;
// The current step of the pattern series the player is on.
extern uint8_t current_series_step;

// Delay for a variable amount of milliseconds
static inline void variable_delay(uint16_t ms_delay) {
    while (ms_delay) {
        _delay_ms(1);
        ms_delay--;
    }
}


/* Pattern Playback */

/* The different pattern types available. */
typedef enum PatternType {
    SOLID = 0,
    FLASH = 1,
    RIBBON = 2,
    SCROLL = 3,
    WIDE_SCROLL = 4,
    SERIES = 5,
    CUSTOM = 6,
} PatternType_t;

/* A Generic Pattern representing the union of all available PatternTypes &
 * their data structs.
 */
typedef struct GenericPattern {
    PatternType_t pattern_type;
    const void *pattern_type_args;
} GenericPattern_t;

/* Defines to simplify creation of GenericPattern structs */
#define PATTERN_ARGS(pattern) .pattern_type_args = &pattern
#define SOLID_PATTERN(pattern) { .pattern_type = SOLID, PATTERN_ARGS(pattern) }
#define FLASH_PATTERN(pattern) { .pattern_type = FLASH, PATTERN_ARGS(pattern) }
#define RIBBON_PATTERN(pattern) { .pattern_type = RIBBON, PATTERN_ARGS(pattern) }
#define SCROLL_PATTERN(pattern) { .pattern_type = SCROLL, PATTERN_ARGS(pattern) }
#define WIDE_SCROLL_PATTERN(pattern) { .pattern_type = WIDE_SCROLL, PATTERN_ARGS(pattern) }
#define SERIES_PATTERN(pattern) { .pattern_type = SERIES, PATTERN_ARGS(pattern) }
#define CUSTOM_PATTERN(pattern) { .pattern_type = CUSTOM, PATTERN_ARGS(pattern) }



/* Initialize the total_pattern_steps for a pattern & reset the
 * current_pattern_step to 0.
 */
void initialize_pattern(const GenericPattern_t *pattern_data);

/* Update the current_sequence array with the color data for the current step
 * of the given pattern. Returns the delay time for the current step.
 */
uint16_t update_sequence(const GenericPattern_t *pattern_data);


/* Output the `current_sequence` to the LEDs via SPI. */
void output_current_sequence(void);

/* Increment the `current_pattern_step` modulo the `total_pattern_steps`. When
 * playing a series of patterns, completing a pattern will increment the
 * current pattern in the series.
 */
inline void increment_current_step(void) {
    current_pattern_step = (current_pattern_step + 1) % total_pattern_steps;
    if (current_pattern_step == 0 && playing_pattern_series) {
        current_series_step = (current_series_step + 1) % total_series_steps;
    }
}

/* Update the Sequence, Output it, Delay, & Increment the step count */
static inline void run_step(const GenericPattern_t *pattern_data) {
    uint16_t delay = update_sequence(pattern_data);
    output_current_sequence();
    variable_delay(delay);
    increment_current_step();
}


/* Pattern Steppers
 *
 * These functions describe the available patterns/effects for the LEDs.
 *
 * The `_step_count` functions determine the number of steps a pattern has
 * before it repeats.
 *
 * The `_set_sequence` functions set the value of the `current_sequence` array
 * to the colors for the pattern's `current_pattern_step`.
 *
 * Each `PatternArgs_t` type is a struct containing the data used by the
 * `_step_count` and `_set_sequence` functions.
 *
 */

// Solid Colors
typedef struct SolidArgs{
    RGBColor_t color;
    uint16_t delay;
} SolidArgs_t;
uint8_t solid_step_count(void);
uint16_t solid_set_sequence(const SolidArgs_t *args);

// Flash - solid colors interspersed with blank gaps
typedef struct FlashArgs {
    const RGBColor_t *sequence;
    uint8_t length;
    uint16_t color_delay;
    uint16_t blank_delay;
} FlashArgs_t;
uint8_t flash_step_count(const uint8_t sequence_len);
uint16_t flash_set_sequence(const FlashArgs_t *args);

// Ribbon - solid colors with no blank gaps
typedef struct RibbonArgs {
    const RGBColor_t *sequence;
    uint8_t length;
    uint16_t delay;
} RibbonArgs_t;
uint8_t ribbon_step_count(const uint8_t sequence_len);
uint16_t ribbon_set_sequence(const RibbonArgs_t *args);

// Scroll - move sequence up/down strip - repeating if shorter than LED count
// TODO: Change `reverse` to ScrollDirection enum - ScrollDown/ScrollUp
typedef struct ScrollArgs {
    const RGBColor_t *sequence;
    uint8_t length;
    bool reverse;
    uint16_t delay;
} ScrollArgs_t;
uint8_t scroll_step_count(const uint8_t sequence_len);
uint16_t scroll_set_sequence(const ScrollArgs_t *args);

// Wide Scroll - extend color bands instead of repeating
// TODO: Add ScrollDirection
typedef struct WideScrollArgs {
    const RGBColor_t *sequence;
    uint8_t length;
    uint16_t delay;
} WideScrollArgs_t;
uint8_t wide_scroll_step_count(void);
uint16_t wide_scroll_set_sequence(const WideScrollArgs_t *args);

// Series - play multiple patterns one after another
// The player code assumes that `get_pattern_for_step` will never return a
// `SERIES` pattern. Bad things may happen if you do this!
// TODO: Could reduce computations by storing generated GenericPatterns while
// playing them.
typedef struct SeriesArgs {
    // Return the total number of patterns in the series.
    uint8_t (*total_series_steps_function)(void *);
    // Generate the current pattern using the `current_series_step` global.
    GenericPattern_t (*get_pattern_for_step)(void *);
    // Additional pattern-specific data passed to the above functions.
    void *series_data;
} SeriesArgs_t;

// Custom User Patterns - set each step's color sequence yourself
typedef struct CustomPatternArgs {
    // Return the total number of steps in the pattern.
    uint8_t (*step_count_function)(void *);
    // Set the color sequence array using the `current_pattern_step` global.
    uint16_t (*set_sequence_function)(RGBColor_t *, void *);
    // Additional pattern-specific data passed to the above functions.
    void *custom_data;
} CustomPatternArgs_t;


// Extend/Retract
/* These effects assume the desired pattern_data is already initialized. They
 * will increase/reduce the amount of LEDs shown while playing the given
 * pattern. The pattern's delay is ignored and the delay argument is used
 * instead - this allows faster extension/retraction with slow patterns.
 */
void extend_pattern(const GenericPattern_t *pattern_data, const uint16_t delay);
void retract_pattern(const GenericPattern_t *pattern_data, const uint16_t delay);


#endif
