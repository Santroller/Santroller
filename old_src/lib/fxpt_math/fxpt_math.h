/*
 * fxpt_atan2.c
 *
 * Copyright (C) 2012, Xo Wang
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <math.h>
#include <stdint.h>


/**
 * 16-bit fixed point four-quadrant arctangent. Given some Cartesian vector
 * (x, y), find the angle subtended by the vector and the positive x-axis.
 *
 * The value returned is in units of 1/65536ths of one turn. This allows the use
 * of the full 16-bit unsigned range to represent a turn. e.g. 0x0000 is 0
 * radians, 0x8000 is pi radians, and 0xFFFF is (65535 / 32768) * pi radians.
 *
 * Because the magnitude of the input vector does not change the angle it
 * represents, the inputs can be in any signed 16-bit fixed-point format.
 *
 * @param y y-coordinate in signed 16-bit
 * @param x x-coordinate in signed 16-bit
 * @return angle in (val / 32768) * pi radian increments from 0x0000 to 0xFFFF
 */
uint16_t fxpt_atan2(const int16_t y, const int16_t x);
uint16_t fxpt_asin(int16_t x);