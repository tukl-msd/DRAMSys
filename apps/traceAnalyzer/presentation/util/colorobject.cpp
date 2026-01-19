/*
 * Copyright (c) 2015, RPTU Kaiserslautern-Landau
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors:
 *    Janik Schlemminger
 *    Robert Gernhardt
 *    Matthias Jung
 *    Iron Prando da Silva
 */

#include "colorobject.h"

QColor ColorObject::getColor(unsigned int i)
{
    i = i % numberOfColors;
    QColor result(r[i], g[i], b[i]);
    result.setAlpha(130);
    return result;
}

QColor ColorObject::getRainbowColored(unsigned int i)
{
    const int minAlpha = 50;
    const int alphaLevels = 20 - 255 / minAlpha;
    const int alphaStep = (255. - minAlpha) / alphaLevels;

    int alpha = minAlpha + (int)(alphaStep * (i % alphaLevels));
    i = (i / alphaLevels) % numberOfColors;
    QColor result(r[i], g[i], b[i]);
    result.setAlpha(alpha);

    return result;
}

ColorDefault::ColorDefault()
{
    numberOfColors = 16;

    r.resize(numberOfColors);
    g.resize(numberOfColors);
    b.resize(numberOfColors);

    r[0] = 0xFF;
    r[1] = 0x00;
    r[2] = 0x00;
    r[3] = 0xFF;
    g[0] = 0x00;
    g[1] = 0xFF;
    g[2] = 0x00;
    g[3] = 0xFF;
    b[0] = 0x00;
    b[1] = 0x00;
    b[2] = 0xFF;
    b[3] = 0x00;

    r[4] = 0xFF;
    r[5] = 0x00;
    r[6] = 0xFF;
    r[7] = 0x6B;
    g[4] = 0x00;
    g[5] = 0xFF;
    g[6] = 0xA5;
    g[7] = 0x8E;
    b[4] = 0xFF;
    b[5] = 0xFF;
    b[6] = 0x00;
    b[7] = 0x23;

    r[8] = 0x8A;
    r[9] = 0xFF;
    r[10] = 0x7C;
    r[11] = 0x00;
    g[8] = 0x2B;
    g[9] = 0xD7;
    g[10] = 0xFC;
    g[11] = 0x00;
    b[8] = 0xE2;
    b[9] = 0x00;
    b[10] = 0x00;
    b[11] = 0x80;

    r[12] = 0x80;
    r[13] = 0x00;
    r[14] = 0xEE;
    r[15] = 0xFF;
    g[12] = 0x00;
    g[13] = 0x80;
    g[14] = 0x82;
    g[15] = 0x45;
    b[12] = 0x00;
    b[13] = 0x00;
    b[14] = 0xEE;
    b[15] = 0x00;
}

ColorHSV15::ColorHSV15()
{
    numberOfColors = 15;

    r.resize(numberOfColors);
    g.resize(numberOfColors);
    b.resize(numberOfColors);

    r[0] = 0XFF;
    r[1] = 0XFF;
    r[2] = 0XFF;
    r[3] = 0XD1;
    r[4] = 0X6C;
    r[5] = 0X08;
    r[6] = 0X00;
    r[7] = 0X00;
    r[8] = 0X00;
    r[9] = 0X00;
    r[10] = 0X00;
    r[11] = 0X54;
    r[12] = 0XB9;
    r[13] = 0XFF;
    r[14] = 0XFF;

    g[0] = 0X00;
    g[1] = 0X64;
    g[2] = 0XC9;
    g[3] = 0XFF;
    g[4] = 0XFF;
    g[5] = 0XFF;
    g[6] = 0XFF;
    g[7] = 0XFF;
    g[8] = 0XD9;
    g[9] = 0X74;
    g[10] = 0X10;
    g[11] = 0X00;
    g[12] = 0X00;
    g[13] = 0X00;
    g[14] = 0X00;

    b[0] = 0X00;
    b[1] = 0X00;
    b[2] = 0X00;
    b[3] = 0X00;
    b[4] = 0X00;
    b[5] = 0X00;
    b[6] = 0X5C;
    b[7] = 0XC1;
    b[8] = 0XFF;
    b[9] = 0XFF;
    b[10] = 0XFF;
    b[11] = 0XFF;
    b[12] = 0XFF;
    b[13] = 0XE1;
    b[14] = 0X7C;
}
