/*
 * Copyright (c) 2024, RPTU Kaiserslautern-Landau
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
 *    Derek Christ
 */

#ifndef EXTENSION_H
#define EXTENSION_H

#include <QLabel>
#include <QMessageBox>

inline constexpr std::string_view EXTENSION_DISCLAIMER =
    "This feature is part of the full version of Trace Analyzer. For more information, please "
    "contact <a href=\"mailto:DRAMSys@iese.fraunhofer.de\">DRAMSys@iese.fraunhofer.de</a>.";

inline void showExtensionDisclaimerMessageBox()
{
    QMessageBox box;
    box.setText("Feature unavailable");
    box.setInformativeText(EXTENSION_DISCLAIMER.data());
    box.setIcon(QMessageBox::Information);
    box.exec();
}

inline QLabel* disclaimerLabel()
{
    auto* label = new QLabel;
    label->setText(EXTENSION_DISCLAIMER.data());
    label->setAlignment(Qt::AlignHCenter);
    label->setWordWrap(true);
    return label;
}

#endif // EXTENSION_H
