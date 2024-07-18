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
 *    Derek Christ
 */

#include "traceanalyzer.h"

#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QSet>
#include <QString>
#include <filesystem>
#include <iostream>
#include <pybind11/embed.h>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    QIcon icon(QStringLiteral(":/icon"));
    QApplication::setWindowIcon(icon);
    QApplication::setApplicationName(QStringLiteral("TraceAnalyzer"));
    QApplication::setApplicationDisplayName(QStringLiteral("Trace Analyzer"));

    std::filesystem::path extensionDir = DRAMSYS_TRACE_ANALYZER_EXTENSION_DIR;
    std::filesystem::path modulesDir = extensionDir / "scripts";

    pybind11::scoped_interpreter guard;

    // Add scripts directory to local module search path
    pybind11::module_ sys = pybind11::module_::import("sys");
    pybind11::list path = sys.attr("path");
    path.append(modulesDir.c_str());

    if (argc > 1)
    {
        QSet<QString> arguments;
        for (int i = 1; i < argc; ++i)
            arguments.insert(QString(argv[i]));

        QString openFolderFlag("-f");
        if (arguments.contains(openFolderFlag))
        {
            arguments.remove(openFolderFlag);
            QStringList nameFilter("*.tdb");
            QSet<QString> paths = arguments;
            arguments.clear();
            for (QString path : paths)
            {
                QDir directory(path);
                QStringList files = directory.entryList(nameFilter);
                for (QString& file : files)
                {
                    arguments.insert(path.append("/") + file);
                }
            }
        }

        TraceAnalyzer analyzer(arguments);
        analyzer.show();
        return QApplication::exec();
    }

    TraceAnalyzer analyzer;
    analyzer.show();
    return QApplication::exec();
}
