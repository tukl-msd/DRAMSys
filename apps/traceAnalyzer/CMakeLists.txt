# Copyright (c) 2020, RPTU Kaiserslautern-Landau
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
# OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors:
#    Matthias Jung
#    Lukas Steiner
#    Derek Christ
#    Iron Prando da Silva

########################################
###           TraceAnalyzer          ###
########################################

if (APPLE)
    set(Qt5_DIR "/opt/homebrew/opt/qt@5/lib/cmake/Qt5")
endif(APPLE)

find_package(Qt5 COMPONENTS Core Gui Widgets Sql REQUIRED)
find_package(Qwt REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(TraceAnalyzer
    businessObjects/commentmodel.cpp
    businessObjects/configmodels.cpp
    businessObjects/phases/phase.cpp
    businessObjects/phases/phasefactory.cpp
    businessObjects/pythoncaller.cpp
    businessObjects/timespan.cpp
    businessObjects/traceplotlinemodel.cpp
    businessObjects/transaction.cpp
    data/tracedb.cpp
    evaluationtool.cpp
    gototimedialog.cpp
    main.cpp
    presentation/debugmessagetreewidget.cpp
    presentation/selectedtransactiontreewidget.cpp
    presentation/tracePlotMouseLabel.cpp
    presentation/tracedrawing.cpp
    presentation/tracedrawingproperties.cpp
    presentation/tracemetrictreewidget.cpp
    presentation/tracenavigator.cpp
    presentation/traceplot.cpp
    presentation/traceplotitem.cpp
    presentation/tracescroller.cpp
    presentation/transactiontreewidget.cpp
    presentation/util/clkgrid.cpp
    presentation/util/colorgenerator.cpp
    presentation/util/colorobject.cpp
    presentation/util/customlabelscaledraw.cpp
    presentation/util/testlight.cpp
    presentation/util/togglecollapsedaction.cpp
    queryeditor.cpp
    selectmetrics.cpp
    traceanalyzer.cpp
    tracefiletab.cpp
)

# DRAMSys module interface that modules can link against
add_library(traceanalyzer_modules INTERFACE)

target_link_libraries(TraceAnalyzer
    PUBLIC
        traceanalyzer_modules
        Qt5::Widgets
        Qt5::Sql
        Qwt::Qwt
        nlohmann_json::nlohmann_json
)

install(TARGETS TraceAnalyzer)
