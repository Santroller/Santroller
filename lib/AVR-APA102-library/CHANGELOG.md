# CHANGELOG

## master

* Add an optional `CURRENT_PER_LED` macro. Defining this will scale colors used
  with the `simple_effects` & `patterns` modules if they surpass the current
  limit, assuming linear current draw at a maximum of 60mA per LED.  The
  example's Makefile shows how to implement a current limit for the entire
  strip.
* Add a `series_data` field to the `SeriesArgs` struct, allowing the user to
  pass custom data to the step count & get pattern functions.
* Add a `delay` field to the `SolidArgs` struct. This allows users to pause on
  a `SOLID` pattern when it's nested in a `SERIES` pattern.
* Add the `CUSTOM` pattern type. This allows defining patterns that use your
  own `_step_count` & `_set_sequence` functions as well as arbitrary data.
* Add the `rgbcolor_equal` function to the `apa102.h` module for determining
  whether the RGB values for two colors are equal.
* Add the `SERIES` pattern type. This allows creating patterns that are
  sequences of other patterns. E.g., multiple flash patterns with decreasing
  blank delays, or a scroll up followed by a scroll down.
* Replace the extend/retract `_current_sequence` functions with
  `extend_pattern` & `retract_pattern` functions. These functions play the
  patterns while extending & retracting, instead of displaying a single
  sequence from a pattern.
* Add the `run_step` function to the `apa102_patterns` module. This combines
  the act of updating & outputting a pattern's sequence, delaying for the
  appropriate amount of time, & incrementing the step count.
* Add an `apa102_patterns` module. Instead of directly outputting sequences
  like the `apa102_simple_effects` module, the patterns module calculates the
  number of steps in a pattern, tracks the current step in the pattern, and
  writes the sequence to an `RGBColor_t` array. The array can then be modified
  and output to the DotStar strip. This allows manipulations like
  extension/retraction while playing a pattern, which the
  `apa102_simple_effects` module does not support.
* Add an optional `STARTING_LED` macro to shift the controlled LEDs down the
  strip by prepending sequences with blank LED frames.
* Move `apa102_effects.h` functions to `apa102_simple_effects.h`.


## v1.0.0

The simplest thing this library could be. The main `apa102.h` file has the
hardware SPI initialization & frame transmission functions along with the
`RGBColor_t` type.

The `apa102_effects.h` file contains some functions that output different
patterns directly to the hardware SPI, like flashing between a sequence of
colors, extending/retracting solid colors, or scrolling a sequence of colors
up/down the saber.
