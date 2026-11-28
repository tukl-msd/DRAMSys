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
 */

#include "phasefactory.h"
#include "businessObjects/timespan.h"
#include "businessObjects/transaction.h"
#include "data/tracedb.h"
#include "phase.h"

std::shared_ptr<Phase> PhaseFactory::createPhase(const QString& dbPhaseName,
                                                 Timespan span,
                                                 Timespan spanOnDataStrobe,
                                                 unsigned int rank,
                                                 unsigned int bankGroup,
                                                 unsigned int bank,
                                                 unsigned int row,
                                                 unsigned int column,
                                                 unsigned int burstLength,
                                                 unsigned int dualBank,
                                                 const std::shared_ptr<Transaction>& trans,
                                                 TraceDB& database)
{
    auto clk = static_cast<traceTime>(database.getGeneralInfo().clkPeriod);
    unsigned int groupsPerRank = database.getGeneralInfo().groupsPerRank;
    unsigned int banksPerGroup = database.getGeneralInfo().banksPerGroup;
    auto const& cl = database.getCommandLengths();

    if (dbPhaseName == "REQ")
        return std::shared_ptr<Phase>(new REQ(span,
                                              Timespan{},
                                              rank,
                                              bankGroup,
                                              bank,
                                              row,
                                              column,
                                              burstLength,
                                              dualBank,
                                              clk,
                                              trans,
                                              {},
                                              groupsPerRank,
                                              banksPerGroup));

    if (dbPhaseName == "RESP")
        return std::shared_ptr<Phase>(new RESP(span,
                                               Timespan{},
                                               rank,
                                               bankGroup,
                                               bank,
                                               row,
                                               column,
                                               burstLength,
                                               dualBank,
                                               clk,
                                               trans,
                                               {},
                                               groupsPerRank,
                                               banksPerGroup));

    if (dbPhaseName == "PREPB")
        return std::shared_ptr<Phase>(
            new PREPB(span,
                      Timespan{},
                      rank,
                      bankGroup,
                      bank,
                      row,
                      column,
                      burstLength,
                      dualBank,
                      clk,
                      trans,
                      {Timespan(span.Begin(), span.Begin() + cl.at("PREPB"))},
                      groupsPerRank,
                      banksPerGroup));

    if (dbPhaseName == "ACT")
        return std::shared_ptr<Phase>(new ACT(span,
                                              Timespan{},
                                              rank,
                                              bankGroup,
                                              bank,
                                              row,
                                              column,
                                              burstLength,
                                              dualBank,
                                              clk,
                                              trans,
                                              {Timespan(span.Begin(), span.Begin() + cl.at("ACT"))},
                                              groupsPerRank,
                                              banksPerGroup));

    if (dbPhaseName == "PREAB")
        return std::shared_ptr<Phase>(
            new PREAB(span,
                      Timespan{},
                      rank,
                      bankGroup,
                      bank,
                      row,
                      column,
                      burstLength,
                      dualBank,
                      clk,
                      trans,
                      {Timespan(span.Begin(), span.Begin() + cl.at("PREAB"))},
                      groupsPerRank,
                      banksPerGroup));

    if (dbPhaseName == "REFAB")
        return std::shared_ptr<Phase>(
            new REFAB(span,
                      Timespan{},
                      rank,
                      bankGroup,
                      bank,
                      row,
                      column,
                      burstLength,
                      dualBank,
                      clk,
                      trans,
                      {Timespan(span.Begin(), span.Begin() + cl.at("REFAB"))},
                      groupsPerRank,
                      banksPerGroup));

    if (dbPhaseName == "RFMAB")
        return std::shared_ptr<Phase>(
            new RFMAB(span,
                      Timespan{},
                      rank,
                      bankGroup,
                      bank,
                      row,
                      column,
                      burstLength,
                      dualBank,
                      clk,
                      trans,
                      {Timespan(span.Begin(), span.Begin() + cl.at("RFMAB"))},
                      groupsPerRank,
                      banksPerGroup));

    if (dbPhaseName == "REFPB")
        return std::shared_ptr<Phase>(
            new REFPB(span,
                      Timespan{},
                      rank,
                      bankGroup,
                      bank,
                      row,
                      column,
                      burstLength,
                      dualBank,
                      clk,
                      trans,
                      {Timespan(span.Begin(), span.Begin() + cl.at("REFPB"))},
                      groupsPerRank,
                      banksPerGroup));

    if (dbPhaseName == "RFMPB")
        return std::shared_ptr<Phase>(
            new RFMPB(span,
                      Timespan{},
                      rank,
                      bankGroup,
                      bank,
                      row,
                      column,
                      burstLength,
                      dualBank,
                      clk,
                      trans,
                      {Timespan(span.Begin(), span.Begin() + cl.at("RFMPB"))},
                      groupsPerRank,
                      banksPerGroup));

    if (dbPhaseName == "REFP2B")
        return std::shared_ptr<Phase>(
            new REFP2B(span,
                       Timespan{},
                       rank,
                       bankGroup,
                       bank,
                       row,
                       column,
                       burstLength,
                       dualBank,
                       clk,
                       trans,
                       {Timespan(span.Begin(), span.Begin() + cl.at("REFP2B"))},
                       groupsPerRank,
                       banksPerGroup));

    if (dbPhaseName == "RFMP2B")
        return std::shared_ptr<Phase>(
            new RFMP2B(span,
                       Timespan{},
                       rank,
                       bankGroup,
                       bank,
                       row,
                       column,
                       burstLength,
                       dualBank,
                       clk,
                       trans,
                       {Timespan(span.Begin(), span.Begin() + cl.at("RFMP2B"))},
                       groupsPerRank,
                       banksPerGroup));

    if (dbPhaseName == "REFDB")
        return std::shared_ptr<Phase>(
            new REFDB(span,
                      Timespan{},
                      rank,
                      bankGroup,
                      bank,
                      row,
                      column,
                      burstLength,
                      dualBank,
                      clk,
                      trans,
                      {Timespan(span.Begin(), span.Begin() + cl.at("REFDB"))},
                      groupsPerRank,
                      banksPerGroup));

    if (dbPhaseName == "RFMDB")
        return std::shared_ptr<Phase>(
            new RFMDB(span,
                      Timespan{},
                      rank,
                      bankGroup,
                      bank,
                      row,
                      column,
                      burstLength,
                      dualBank,
                      clk,
                      trans,
                      {Timespan(span.Begin(), span.Begin() + cl.at("RFMDB"))},
                      groupsPerRank,
                      banksPerGroup));

    if (dbPhaseName == "PRESB")
        return std::shared_ptr<Phase>(
            new PRESB(span,
                      Timespan{},
                      rank,
                      bankGroup,
                      bank,
                      row,
                      column,
                      burstLength,
                      dualBank,
                      clk,
                      trans,
                      {Timespan(span.Begin(), span.Begin() + cl.at("PRESB"))},
                      groupsPerRank,
                      banksPerGroup));

    if (dbPhaseName == "REFSB")
        return std::shared_ptr<Phase>(
            new REFSB(span,
                      Timespan{},
                      rank,
                      bankGroup,
                      bank,
                      row,
                      column,
                      burstLength,
                      dualBank,
                      clk,
                      trans,
                      {Timespan(span.Begin(), span.Begin() + cl.at("REFSB"))},
                      groupsPerRank,
                      banksPerGroup));

    if (dbPhaseName == "RFMSB")
        return std::shared_ptr<Phase>(
            new RFMSB(span,
                      Timespan{},
                      rank,
                      bankGroup,
                      bank,
                      row,
                      column,
                      burstLength,
                      dualBank,
                      clk,
                      trans,
                      {Timespan(span.Begin(), span.Begin() + cl.at("RFMSB"))},
                      groupsPerRank,
                      banksPerGroup));

    if (dbPhaseName == "RD")
        return std::shared_ptr<Phase>(new RD(span,
                                             spanOnDataStrobe,
                                             rank,
                                             bankGroup,
                                             bank,
                                             row,
                                             column,
                                             burstLength,
                                             dualBank,
                                             clk,
                                             trans,
                                             {Timespan(span.Begin(), span.Begin() + cl.at("RD"))},
                                             groupsPerRank,
                                             banksPerGroup));

    if (dbPhaseName == "RDA")
        return std::shared_ptr<Phase>(new RDA(span,
                                              spanOnDataStrobe,
                                              rank,
                                              bankGroup,
                                              bank,
                                              row,
                                              column,
                                              burstLength,
                                              dualBank,
                                              clk,
                                              trans,
                                              {Timespan(span.Begin(), span.Begin() + cl.at("RDA"))},
                                              groupsPerRank,
                                              banksPerGroup));

    if (dbPhaseName == "WR")
        return std::shared_ptr<Phase>(new WR(span,
                                             spanOnDataStrobe,
                                             rank,
                                             bankGroup,
                                             bank,
                                             row,
                                             column,
                                             burstLength,
                                             dualBank,
                                             clk,
                                             trans,
                                             {Timespan(span.Begin(), span.Begin() + cl.at("WR"))},
                                             groupsPerRank,
                                             banksPerGroup));

    if (dbPhaseName == "MWR")
        return std::shared_ptr<Phase>(new MWR(span,
                                              spanOnDataStrobe,
                                              rank,
                                              bankGroup,
                                              bank,
                                              row,
                                              column,
                                              burstLength,
                                              dualBank,
                                              clk,
                                              trans,
                                              {Timespan(span.Begin(), span.Begin() + cl.at("WR"))},
                                              groupsPerRank,
                                              banksPerGroup));

    if (dbPhaseName == "WRA")
        return std::shared_ptr<Phase>(new WRA(span,
                                              spanOnDataStrobe,
                                              rank,
                                              bankGroup,
                                              bank,
                                              row,
                                              column,
                                              burstLength,
                                              dualBank,
                                              clk,
                                              trans,
                                              {Timespan(span.Begin(), span.Begin() + cl.at("WRA"))},
                                              groupsPerRank,
                                              banksPerGroup));

    if (dbPhaseName == "MWRA")
        return std::shared_ptr<Phase>(new MWRA(span,
                                               spanOnDataStrobe,
                                               rank,
                                               bankGroup,
                                               bank,
                                               row,
                                               column,
                                               burstLength,
                                               dualBank,
                                               clk,
                                               trans,
                                               {Timespan(span.Begin(), span.Begin() + cl.at("WR"))},
                                               groupsPerRank,
                                               banksPerGroup));

    if (dbPhaseName == "PDNA")
        return std::shared_ptr<Phase>(
            new PDNA(span,
                     Timespan{},
                     rank,
                     bankGroup,
                     bank,
                     row,
                     column,
                     burstLength,
                     dualBank,
                     clk,
                     trans,
                     {Timespan(span.Begin(), span.Begin() + cl.at("PDEA")),
                      Timespan(span.End() - cl.at("PDXA"), span.End())},
                     groupsPerRank,
                     banksPerGroup));

    if (dbPhaseName == "PDNP")
        return std::shared_ptr<Phase>(
            new PDNP(span,
                     Timespan{},
                     rank,
                     bankGroup,
                     bank,
                     row,
                     column,
                     burstLength,
                     dualBank,
                     clk,
                     trans,
                     {Timespan(span.Begin(), span.Begin() + cl.at("PDEP")),
                      Timespan(span.End() - cl.at("PDXP"), span.End())},
                     groupsPerRank,
                     banksPerGroup));

    if (dbPhaseName == "SREF")
        return std::shared_ptr<Phase>(
            new SREF(span,
                     Timespan{},
                     rank,
                     bankGroup,
                     bank,
                     row,
                     column,
                     burstLength,
                     dualBank,
                     clk,
                     trans,
                     {Timespan(span.Begin(), span.Begin() + cl.at("SREFEN")),
                      Timespan(span.End() - cl.at("SREFEX"), span.End())},
                     groupsPerRank,
                     banksPerGroup));

    throw std::runtime_error("DB phasename " + dbPhaseName.toStdString() +
                             " unknown to phasefactory");
}
