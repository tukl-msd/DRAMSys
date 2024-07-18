/*
 * Copyright (c) 2019, RPTU Kaiserslautern-Landau
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
 *    Lukas Steiner
 */

#ifndef COMMANDLENGTHS_H
#define COMMANDLENGTHS_H

struct CommandLengths
{
    double NOP = 1;
    double RD = 1;
    double WR = 1;
    double MWR = 1;
    double RDA = 1;
    double WRA = 1;
    double MWRA = 1;
    double ACT = 1;
    double PREPB = 1;
    double REFPB = 1;
    double RFMPB = 1;
    double REFP2B = 1;
    double RFMP2B = 1;
    double PRESB = 1;
    double REFSB = 1;
    double RFMSB = 1;
    double PREAB = 1;
    double REFAB = 1;
    double RFMAB = 1;
    double PDEA = 1;
    double PDXA = 1;
    double PDEP = 1;
    double PDXP = 1;
    double SREFEN = 1;
    double SREFEX = 1;

    CommandLengths(double NOP,
                   double RD,
                   double WR,
                   double MWR,
                   double RDA,
                   double WRA,
                   double MWRA,
                   double ACT,
                   double PREPB,
                   double REFPB,
                   double RFMPB,
                   double REFP2B,
                   double RFMP2B,
                   double PRESB,
                   double REFSB,
                   double RFMSB,
                   double PREAB,
                   double REFAB,
                   double RFMAB,
                   double PDEA,
                   double PDXA,
                   double PDEP,
                   double PDXP,
                   double SREFEN,
                   double SREFEX) :
        NOP(NOP),
        RD(RD),
        WR(WR),
        MWR(MWR),
        RDA(RDA),
        WRA(WRA),
        MWRA(MWRA),
        ACT(ACT),
        PREPB(PREPB),
        REFPB(REFPB),
        RFMPB(RFMPB),
        REFP2B(REFP2B),
        RFMP2B(RFMP2B),
        PRESB(PRESB),
        REFSB(REFSB),
        RFMSB(RFMSB),
        PREAB(PREAB),
        REFAB(REFAB),
        RFMAB(RFMAB),
        PDEA(PDEA),
        PDXA(PDXA),
        PDEP(PDEP),
        PDXP(PDXP),
        SREFEN(SREFEN),
        SREFEX(SREFEX)
    {
    }

    CommandLengths() = default;
};

#endif // COMMANDLENGTHS_H
