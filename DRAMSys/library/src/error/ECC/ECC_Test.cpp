/*
 * Copyright (c) 2017, Technische Universität Kaiserslautern
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
 *    Johannes Feldmann
 *    Eder F. Zulian
 */

// ECC_Test.cpp : Entry point for the console application.
//

#include "stdafx.h"
#include <time.h>
#include "ECC.h"

int main()
{
    // Random number init
    srand(time(NULL));

    // Erstellen
    unsigned size = 4;
    CWord p(ECC::GetNumParityBits(size)), v(size);

    // Daten eingeben
    for (unsigned a = 0; a < 16; a++) {
        v = a;
        v.Rotate();

        ECC::ExtendWord(v);
        printf("%d:\t", a);

        p = 0;
        ECC::CalculateCheckbits(v, p);
        ECC::InsertCheckbits(v, p);
        ECC::CalculateParityBit(v, p[3]);
        ECC::InsertParityBit(v, p[3]);

        v.Print();

        v.Resize(size);
    }

    printf("\r\n");

    for (unsigned x = 0; x < 100; x++) {
        //Get random number
        unsigned a = rand() % 16;

        v.Resize(size);
        v = a;
        v.Rotate();

        ECC::ExtendWord(v);

        p = 0;
        ECC::CalculateCheckbits(v, p);
        ECC::InsertCheckbits(v, p);
        ECC::CalculateParityBit(v, p[3]);
        ECC::InsertParityBit(v, p[3]);
        v.Print();

        // Insert error
        unsigned pos = rand() % 8;
        v[pos] ^= CBit(CBit::ONE);

        printf("Data: %d, Error at pos %d: ", a, pos + 1);
        v[pos].Print();
        printf("\r\n");
        v.Print();

        p = 0;
        ECC::CalculateCheckbits(v, p);
        ECC::CalculateParityBit(v, p[3]);

        printf("%d:\t", a);

        p.Print();

        // Interpreting Data

        unsigned syndrome = 0;
        for (unsigned i = 0; i < p.GetLength() - 1; i++) {
            if (p[i] == CBit::ONE)
                syndrome += (1 << i);
        }

        if (p[3] == CBit::ZERO) {
            // Parity even

            if (syndrome) {
                // Double error
                printf("Double error detected.\r\n");
                break;
            } else {
                // No Error
                printf("No error detected.\r\n");
                break;
            }
        } else {
            // Parity odd

            if (syndrome) {
                // Bit error
                printf("Error detected in Bit %d.\r\n", syndrome);
                if (syndrome == pos + 1)
                    continue;
                else
                    break;
            } else {
                // Overall parity Error
                printf("Overall parity error detected.\r\n");
                if (pos == 7 || pos == 3 || pos == 1 || pos == 0)
                    continue;
                else
                    break;
            }
        }
    }
    system("pause");

    return 0;
}

