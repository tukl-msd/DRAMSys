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

#ifndef QUERYTEXTS_H
#define QUERYTEXTS_H
#include <QString>

struct TransactionQueryTexts
{
    QString queryHead;
    QString selectTransactionsByTimespan, selectTransactionById;
    QString checkDependenciesExist, selectDependenciesByTimespan;
    QString selectDependencyTypePercentages, selectTimeDependencyPercentages,
        selectDelayedPhasePercentages, selectDependencyPhasePercentages;

    TransactionQueryTexts()
    {
        queryHead = "SELECT Transactions.ID AS TransactionID, Ranges.begin, Ranges.end, Address, "
                    "DataLength, Thread, Channel, Command, Phases.ID AS PhaseID, PhaseName, "
                    "PhaseBegin, PhaseEnd, DataStrobeBegin, DataStrobeEnd, Rank, BankGroup, Bank, "
                    "Row, Column, BurstLength "
                    " FROM Transactions INNER JOIN Phases ON Phases.Transact = Transactions.ID "
                    "INNER JOIN Ranges ON Transactions.Range = Ranges.ID ";

        selectTransactionsByTimespan =
            queryHead + " WHERE Ranges.end >= :begin AND Ranges.begin <= :end";
        selectTransactionById = queryHead + " WHERE Transactions.ID = :id";

        checkDependenciesExist =
            "SELECT CASE WHEN 0 < (SELECT count(*) FROM sqlite_master WHERE type = 'table' AND "
            "name = 'DirectDependencies') THEN 1 ELSE 0 END AS result";
        selectDependenciesByTimespan =
            "WITH timespanTransactions AS (" + selectTransactionsByTimespan +
            ") SELECT * from DirectDependencies WHERE DelayedPhaseID IN ("
            " SELECT DirectDependencies.DelayedPhaseID FROM DirectDependencies JOIN "
            "timespanTransactions "
            " ON DirectDependencies.DelayedPhaseID = timespanTransactions.PhaseID ) ";

        // For some reason I could not use a parameter for these below
        selectDependencyTypePercentages =
            "WITH TotalDeps (total) AS ( "
            "SELECT COUNT(*) FROM DirectDependencies "
            "), "
            "DependencyTypeDeps (param, ndeps) AS ( "
            "SELECT "
            "DependencyType, "
            "COUNT(*) "
            "FROM DirectDependencies "
            "GROUP BY \"DependencyType\" "
            ") "
            "SELECT param, ROUND(ndeps*100.0 / (SELECT total FROM TotalDeps), 3) as percentage "
            "FROM DependencyTypeDeps "
            "ORDER BY percentage DESC ;";

        selectTimeDependencyPercentages =
            "WITH TotalDeps (total) AS ( "
            "SELECT COUNT(*) FROM DirectDependencies "
            "), "
            "DependencyTypeDeps (param, ndeps) AS ( "
            "SELECT "
            "TimeDependency, "
            "COUNT(*) "
            "FROM DirectDependencies "
            "GROUP BY \"TimeDependency\" "
            ") "
            "SELECT param, ROUND(ndeps*100.0 / (SELECT total FROM TotalDeps), 3) as percentage "
            "FROM DependencyTypeDeps "
            "ORDER BY percentage DESC ;";

        selectDelayedPhasePercentages =
            "WITH TotalDeps (total) AS ( "
            "SELECT COUNT(*) FROM DirectDependencies "
            "), "
            "DependencyTypeDeps (param, ndeps) AS ( "
            "SELECT "
            "DelayedPhaseName, "
            "COUNT(*) "
            "FROM DirectDependencies "
            "GROUP BY \"DelayedPhaseName\" "
            ") "
            "SELECT param, ROUND(ndeps*100.0 / (SELECT total FROM TotalDeps), 3) as percentage "
            "FROM DependencyTypeDeps "
            "ORDER BY percentage DESC ;";

        selectDependencyPhasePercentages =
            "WITH TotalDeps (total) AS ( "
            "SELECT COUNT(*) FROM DirectDependencies "
            "), "
            "DependencyTypeDeps (param, ndeps) AS ( "
            "SELECT "
            "DependencyPhaseName, "
            "COUNT(*) "
            "FROM DirectDependencies "
            "GROUP BY \"DependencyPhaseName\" "
            ") "
            "SELECT param, ROUND(ndeps*100.0 / (SELECT total FROM TotalDeps), 3) as percentage "
            "FROM DependencyTypeDeps "
            "ORDER BY percentage DESC ;";
    }
};

#endif // QUERYTEXTS_H
