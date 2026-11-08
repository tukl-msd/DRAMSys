#!/usr/bin/env python3
"""Emit CheckerLPDDR5.cpp and CheckerHBM3_4.cpp in the style of the generated DRAMSys checkers.

Each constraint: (level, from_cmds, to_cmds, expr, cond)
  level: Bank | BankPair | BankGroup | Stack | Rank | OtherStack | OtherRank
  cond:  None | "BL32" | "BL16" (burst length of the "from" command)
"""
import sys
from collections import OrderedDict

LICENSE = """/*
 * Copyright (c) 2026, RPTU Kaiserslautern-Landau
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
 *    Martinius
 */
"""

RD = ["RD", "RDA"]
WR = ["WR", "WRA"]


def block(level, src, dst, expr, cond, std):
    cond_txt = "[]"
    open_if = close_if = ""
    if cond == "BL32":
        cond_txt = "[LastBurstLength(burst_length=32, inversed=False)]"
        open_if, close_if = "            if (burstLength == 32)\n            {\n", "            }\n"
    elif cond == "BL16":
        cond_txt = "[LastBurstLength(burst_length=32, inversed=True)]"
        open_if, close_if = "            if (burstLength != 32)\n            {\n", "            }\n"
    comp = "SameComponent()"
    if level in ("OtherRank",):
        comp = "Different(level=<ComponentLevel.Rank: 3>)"
    if level in ("OtherStack",):
        comp = "Different(level=<ComponentLevel.Stack: 7>)"
    lvl_name = {"Bank": "Bank", "BankPair": "BankPair", "BankGroup": "BankGroup",
                "Stack": "Stack", "Rank": std["rank_name"], "OtherStack": std["rank_name"],
                "OtherRank": "Channel"}[level]
    out = f"        // {lvl_name} ({src},{dst}) {expr} {cond_txt} {comp}\n        {{\n" + open_if
    out += f"            const sc_time constraint = currentTime + {expr};\n"
    if level == "Bank":
        out += f"            sc_time &earliestTimeToStart = nextCommandByBank[Command::{dst}][bank];\n"
        out += "            earliestTimeToStart = std::max(earliestTimeToStart, constraint);\n"
    elif level == "BankPair":
        out += "            for (Bank currentBank : {bank, pairedBank})\n            {\n"
        out += f"                sc_time &earliestTimeToStart = nextCommandByBank[Command::{dst}][currentBank];\n"
        out += "                earliestTimeToStart = std::max(earliestTimeToStart, constraint);\n            }\n"
    elif level == "BankGroup":
        out += f"            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::{dst}][bankGroup];\n"
        out += "            earliestTimeToStart = std::max(earliestTimeToStart, constraint);\n"
    elif level == "Stack":
        out += f"            sc_time &earliestTimeToStart = nextCommandByStack[Command::{dst}][stack];\n"
        out += "            earliestTimeToStart = std::max(earliestTimeToStart, constraint);\n"
    elif level == "Rank":
        out += f"            sc_time &earliestTimeToStart = nextCommandByRank[Command::{dst}][rank];\n"
        out += "            earliestTimeToStart = std::max(earliestTimeToStart, constraint);\n"
    elif level == "OtherStack":
        out += "            for (unsigned int i = static_cast<unsigned>(rank) * memSpec.stacksPerChannel; i < (static_cast<unsigned>(rank) + 1) * memSpec.stacksPerChannel; i++)\n"
        out += "            {\n                Stack currentStack{i};\n                \n                if (currentStack == stack)\n                    continue;\n                \n"
        out += f"                sc_time &earliestTimeToStart = nextCommandByStack[Command::{dst}][currentStack];\n"
        out += "                earliestTimeToStart = std::max(earliestTimeToStart, constraint);\n            }\n"
    elif level == "OtherRank":
        out += "            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)\n"
        out += "            {\n                Rank currentRank{i};\n                \n                if (currentRank == rank)\n                    continue;\n                \n"
        out += f"                sc_time &earliestTimeToStart = nextCommandByRank[Command::{dst}][currentRank];\n"
        out += "                earliestTimeToStart = std::max(earliestTimeToStart, constraint);\n            }\n"
    else:
        raise ValueError(level)
    out += close_if + "        }\n        \n"
    return out


def expand(table):
    """table rows -> OrderedDict from_cmd -> list of (level, dst, expr, cond)"""
    per = OrderedDict()
    for level, srcs, dsts, expr, cond in table:
        for s in srcs:
            for d in dsts:
                per.setdefault(s, []).append((level, d, expr, cond))
    return per


ORDER = {"Bank": 0, "BankPair": 0, "BankGroup": 1, "Stack": 2, "Rank": 3, "OtherStack": 4, "OtherRank": 5}


def emit_switch(table, std, faw_cmds, extra_after):
    per = expand(table)
    out = ""
    for src, rows in per.items():
        out += f"    case Command::{src}:\n    {{\n"
        rows = sorted(rows, key=lambda r: ORDER[r[0]])
        for level, dst, expr, cond in rows:
            out += block(level, src, dst, expr, cond, std)
        if src in faw_cmds:
            out += std["faw_block"] if "faw_block" in std else FAW_BLOCK.format(
                window=std["faw_window"], faw_targets="".join(FAW_TARGET.format(t=t) for t in std["faw_targets"]))
        if src in extra_after:
            out += extra_after[src]
        out += "\n        break;\n    }\n"
    return out


FAW_BLOCK = """        last4ActivatesOnRank[rank].push(currentTime + memSpec.getCommandLength(command));

        if (last4ActivatesOnRank[rank].size() >= 4)
        {{
            sc_time constraint = last4ActivatesOnRank[rank].front() - memSpec.getCommandLength(command) + {window};
{faw_targets}
            last4ActivatesOnRank[rank].pop();
        }}

"""
FAW_TARGET = """            {{
                sc_time &earliestTimeToStart = nextCommandByRank[Command::{t}][rank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }}
"""

# --------------------------------------------------------------------------------------------
# LPDDR5 (JESD209-5 structure; BG, 16B and 8B bank architectures; timings from the memspec)
# Timings that end or start at an ACTIVATE refer to its ACT-2 (LP5_ACT below).
# --------------------------------------------------------------------------------------------
LP5 = []
for c, bl in (("BL16", "16"), ("BL32", "32")):
    LP5 += [
        ("Bank", RD, ["PREPB"], f"tRDPRE{bl}", c),
        ("BankGroup", RD, RD, f"memSpec.tCCD_L{bl}", c),
        ("BankGroup", RD, WR, f"memSpec.tRTW_L{bl}", c),
        ("Rank", RD, RD, f"memSpec.tCCD_S{bl}", c),
        ("Rank", RD, WR, f"memSpec.tRTW_S{bl}", c),
        ("Rank", RD, ["PREAB"], f"tRDPRE{bl}", c),
        ("OtherRank", RD, RD, f"tRDRD_R{bl}", c),
        ("OtherRank", RD, WR, f"tRDWR_R{bl}", c),
        ("Bank", ["RDA"], ["ACT"], f"(tRDPRE{bl} + memSpec.tRPpb)", c),
        ("BankPair", ["RDA"], ["REFP2B"], f"(tRDPRE{bl} + memSpec.tRPpb)", c),
        ("Bank", ["RDA"], ["REFPB"], f"(tRDPRE{bl} + memSpec.tRPpb)", c),
        ("Rank", ["RDA"], ["REFAB"], f"(tRDPRE{bl} + memSpec.tRPpb)", c),
        ("Bank", ["WR"], ["PREPB"], f"tWRPRE{bl}", c),
        ("Bank", ["WR"], ["RDA"], f"std::max(tWRRD_L{bl}, (tWRPRE{bl} - tRDPRE{bl}))", c),
        ("BankGroup", WR, WR, f"memSpec.tCCD_L{bl}", c),
        ("BankGroup", WR, RD, f"tWRRD_L{bl}", c),
        ("Rank", WR, WR, f"memSpec.tCCD_S{bl}", c),
        ("Rank", WR, RD, f"tWRRD_S{bl}", c),
        ("Rank", WR, ["PREAB"], f"tWRPRE{bl}", c),
        ("OtherRank", WR, WR, f"tWRWR_R{bl}", c),
        ("OtherRank", WR, RD, f"tWRRD_R{bl}", c),
        ("Bank", ["WRA"], ["ACT"], f"(tWRPRE{bl} + memSpec.tRPpb)", c),
        ("BankPair", ["WRA"], ["REFP2B"], f"(tWRPRE{bl} + memSpec.tRPpb)", c),
        ("Bank", ["WRA"], ["REFPB"], f"(tWRPRE{bl} + memSpec.tRPpb)", c),
        ("Rank", ["WRA"], ["REFAB"], f"(tWRPRE{bl} + memSpec.tRPpb)", c),
    ]
LP5 += [
    ("Bank", ["ACT"], RD + WR, "memSpec.tRCD", None),
    ("Bank", ["ACT"], ["PREPB"], "memSpec.tRAS", None),
    ("Bank", ["ACT"], ["ACT"], "memSpec.tRCpb", None),
    ("BankPair", ["ACT"], ["REFP2B"], "memSpec.tRCpb", None),
    ("Bank", ["ACT"], ["REFPB"], "memSpec.tRCpb", None),
    ("Rank", ["ACT"], ["ACT"], "memSpec.tRRD", None),
    ("Rank", ["ACT"], ["PREAB"], "memSpec.tRAS", None),
    ("Rank", ["ACT"], ["REFAB"], "memSpec.tRCpb", None),
    ("Rank", ["ACT"], ["REFP2B", "REFPB"], "memSpec.tRRD", None),
    ("Bank", ["PREPB"], ["ACT"], "memSpec.tRPpb", None),
    ("BankPair", ["PREPB"], ["REFP2B"], "memSpec.tRPpb", None),
    ("Bank", ["PREPB"], ["REFPB"], "memSpec.tRPpb", None),
    ("Rank", ["PREPB"], ["PREPB", "PREAB"], "memSpec.tPPD", None),
    ("Rank", ["PREPB"], ["REFAB"], "memSpec.tRPpb", None),
    ("Rank", ["PREAB"], ["ACT"], "memSpec.tRPab", None),
    ("Rank", ["PREAB"], ["PREPB", "PREAB"], "memSpec.tPPD", None),
    ("Rank", ["PREAB"], ["REFAB", "REFP2B", "REFPB"], "memSpec.tRPab", None),
    ("Rank", ["REFAB"], ["ACT", "PREAB", "REFAB", "REFP2B", "REFPB"], "memSpec.tRFCab", None),
    ("BankPair", ["REFP2B"], ["ACT", "REFP2B"], "memSpec.tRFCpb", None),
    ("Rank", ["REFP2B"], ["ACT"], "memSpec.tpbR2act", None),
    ("Rank", ["REFP2B"], ["REFP2B"], "memSpec.tpbR2pbR", None),
    ("Rank", ["REFP2B"], ["REFAB", "PREAB"], "memSpec.tRFCpb", None),
    # 8B mode: per-bank refresh covers one bank (REFPB); BG and 16B modes refresh bank pairs (REFP2B)
    ("Bank", ["REFPB"], ["ACT", "REFPB"], "memSpec.tRFCpb", None),
    ("Rank", ["REFPB"], ["ACT"], "memSpec.tpbR2act", None),
    ("Rank", ["REFPB"], ["REFPB"], "memSpec.tpbR2pbR", None),
    ("Rank", ["REFPB"], ["REFAB", "PREAB"], "memSpec.tRFCpb", None),
]

LP5_STD = {"rank_name": "Rank"}

# Timings that start at an ACTIVATE count from ACT-2 (JESD209-5C as cited by Ramulator2 lpddr5.py: single-bank
# timing "measured from ACT-2", tFAW counts ACT-2 and REFpb). ACT-2 takes a free CA cycle within tAAD after ACT-1,
# so these rows are applied when the ACT-2 is placed (applyActivate) and checked against pending ACT-2s
# (activateDelay). Timings that end at an ACTIVATE bound its ACT-2 as well (Ramulator2: every rule into an
# ACTIVATE targets ACT2): nextCommandBy*[ACT] hold the earliest ACT-2, and ACT-1 may be issued up to tAAD before.
LP5_ACT = [r for r in LP5 if r[1] == ["ACT"]]
LP5 = [r for r in LP5 if r[1] != ["ACT"]]
assert not [r for r in LP5 if "ACT" in r[1]]

LP5_FAW_PUSH = """        // tFAW counts ACT-2 and REFpb (rolling window of four)
        last4ActivatesOnRank[rank].push_back(currentTime);

        if (last4ActivatesOnRank[rank].size() >= 4)
        {
            const sc_time windowEnd = last4ActivatesOnRank[rank].front() + memSpec.tFAW;
            {
                sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
                earliestTimeToStart = std::max(earliestTimeToStart, windowEnd);
            }
            {
                sc_time &earliestTimeToStart = nextCommandByRank[Command::REFP2B][rank];
                earliestTimeToStart = std::max(earliestTimeToStart, windowEnd);
            }
            {
                sc_time &earliestTimeToStart = nextCommandByRank[Command::REFPB][rank];
                earliestTimeToStart = std::max(earliestTimeToStart, windowEnd);
            }

            last4ActivatesOnRank[rank].pop_front();
        }

"""


def emit_activate_delay(rows):
    """C++ body: largest delay from a pending ACT-2 to `command`, depending on how the two commands relate"""
    cond = {"Bank": "sameBank", "BankPair": "samePair", "BankGroup": "sameBankGroup", "Rank": "sameRank"}
    out = ""
    for level, dst, expr, c in rows:
        assert c is None
        out += f"    // {level} (ACT,{dst}) {expr}\n"
        out += f"    if (command == Command::{dst} && {cond[level]})\n"
        out += f"        delay = std::max(delay, {expr});\n"
    return out

LP5_DERIVED = []
for bl in ("16", "32"):
    LP5_DERIVED += [
        (f"tRDPRE{bl}", f"memSpec.tBURST{bl} + memSpec.tRBTP"),
        (f"tWRPRE{bl}", f"memSpec.tWL + memSpec.tBURST{bl} + memSpec.tCK + memSpec.tWR"),
        (f"tWRRD_L{bl}", f"memSpec.tWL + memSpec.tBURSTMAX{bl} + memSpec.tWTR_L"),
        (f"tWRRD_S{bl}", f"memSpec.tWL + memSpec.tBURST{bl} + memSpec.tWTR_S"),
        (f"tRDRD_R{bl}", f"memSpec.tBURST{bl} + memSpec.tRTRS"),
        (f"tWRWR_R{bl}", f"memSpec.tBURST{bl} + memSpec.tRTRS"),
        (f"tRDWR_R{bl}", f"positiveDifference(memSpec.tRL + memSpec.tBURST{bl} + memSpec.tRTRS, memSpec.tWL)"),
        (f"tWRRD_R{bl}", f"positiveDifference(memSpec.tWL + memSpec.tBURST{bl} + memSpec.tRTRS, memSpec.tRL)"),
    ]

# --------------------------------------------------------------------------------------------
# HBM3/HBM4 (JESD238 / JESD270-4A Tables 6, 36, 38, 108). Rank = pseudo channel, Stack = SID.
# ACTIVATE timings refer to the second rising CK edge of the ACTIVATE (JESD270-4A 6.3.2.1).
# --------------------------------------------------------------------------------------------
HB = [
    ("Bank", ["RD"], ["PREPB"], "memSpec.tRTP", None),
    ("BankGroup", RD, RD, "memSpec.tCCDL", None),
    ("Rank", RD, RD, "memSpec.tCCDS", None),
    ("OtherStack", RD, RD, "memSpec.tCCDR", None),
    ("Rank", RD, WR, "memSpec.tRTW", None),
    ("Rank", ["RD"], ["PREAB"], "memSpec.tRTP", None),
    ("Rank", RD, ["PDEA", "PDEP"], "tRDPDE", None),
    ("Bank", ["RDA"], ["ACT"], "((memSpec.tRTP + memSpec.tRP) - memSpec.tCK)", None),
    ("Bank", ["RDA"], ["REFPB"], "(memSpec.tRTP + memSpec.tRP)", None),
    ("Rank", ["RDA"], ["PREAB"], "memSpec.tRTP", None),
    ("Rank", ["RDA"], ["REFAB"], "(memSpec.tRTP + memSpec.tRP)", None),
    ("Rank", ["RDA"], ["SREFEN"], "std::max((memSpec.tRTP + memSpec.tRP), tRDSRE)", None),
    ("Rank", ["RD"], ["SREFEN"], "tRDSRE", None),
    ("Bank", ["WR"], ["PREPB"], "tWRPRE", None),
    ("Bank", ["WR"], ["RDA"], "((memSpec.tWL + tBURST) + std::max((memSpec.tWR - memSpec.tRTP), memSpec.tWTRL))", None),
    ("BankGroup", WR, WR, "memSpec.tCCDL", None),
    ("BankGroup", WR, RD, "tWRRDL", None),
    ("Rank", WR, WR, "memSpec.tCCDS", None),
    ("Rank", WR, RD, "tWRRDS", None),
    ("Rank", ["WR"], ["PREAB"], "tWRPRE", None),
    ("Rank", ["WR"], ["PDEA", "PDEP"], "tWRPDE", None),
    ("Rank", ["WR"], ["SREFEN"], "tWRSRE", None),
    ("Bank", ["WRA"], ["ACT"], "((tWRPRE + memSpec.tRP) - memSpec.tCK)", None),
    ("Bank", ["WRA"], ["REFPB"], "(tWRPRE + memSpec.tRP)", None),
    ("Rank", ["WRA"], ["PREAB"], "tWRPRE", None),
    ("Rank", ["WRA"], ["REFAB"], "(tWRPRE + memSpec.tRP)", None),
    ("Rank", ["WRA"], ["PDEA", "PDEP"], "tWRAPDE", None),
    ("Rank", ["WRA"], ["SREFEN"], "tWRASRE", None),
    ("Bank", ["ACT"], ["RD", "RDA"], "(memSpec.tRCDRD + memSpec.tCK)", None),
    ("Bank", ["ACT"], ["WR", "WRA"], "(memSpec.tRCDWR + memSpec.tCK)", None),
    ("Bank", ["ACT"], ["PREPB"], "(memSpec.tRAS + memSpec.tCK)", None),
    ("Bank", ["ACT"], ["ACT"], "memSpec.tRC", None),
    ("Bank", ["ACT"], ["REFPB"], "(memSpec.tRC + memSpec.tCK)", None),
    ("BankGroup", ["ACT"], ["ACT"], "memSpec.tRRDL", None),
    ("BankGroup", ["ACT"], ["REFPB"], "(memSpec.tRRDL + memSpec.tCK)", None),
    ("Rank", ["ACT"], ["ACT"], "memSpec.tRRDS", None),
    ("Rank", ["ACT"], ["REFPB"], "(memSpec.tRRDS + memSpec.tCK)", None),
    ("Rank", ["ACT"], ["PREAB"], "(memSpec.tRAS + memSpec.tCK)", None),
    ("Rank", ["ACT"], ["REFAB", "SREFEN"], "(memSpec.tRC + memSpec.tCK)", None),
    ("Rank", ["ACT"], ["PDEA"], "tACTPDE", None),
    ("Bank", ["PREPB"], ["ACT"], "(memSpec.tRP - memSpec.tCK)", None),
    ("Bank", ["PREPB"], ["REFPB"], "memSpec.tRP", None),
    ("Rank", ["PREPB"], ["PREPB", "PREAB"], "memSpec.tPPD", None),
    ("Rank", ["PREPB"], ["REFAB", "SREFEN"], "memSpec.tRP", None),
    ("Rank", ["PREPB"], ["PDEA", "PDEP"], "tPRPDE", None),
    ("Rank", ["PREAB"], ["ACT"], "(memSpec.tRP - memSpec.tCK)", None),
    ("Rank", ["PREAB"], ["PREPB", "PREAB"], "memSpec.tPPD", None),
    ("Rank", ["PREAB"], ["REFAB", "REFPB", "SREFEN"], "memSpec.tRP", None),
    ("Rank", ["PREAB"], ["PDEP"], "tPRPDE", None),
    ("Rank", ["REFAB"], ["ACT"], "(memSpec.tRFC - memSpec.tCK)", None),
    ("Rank", ["REFAB"], ["REFAB", "REFPB", "SREFEN"], "memSpec.tRFC", None),
    ("Rank", ["REFAB"], ["PDEP"], "tREFPDE", None),
    ("Bank", ["REFPB"], ["ACT"], "(memSpec.tRFCPB - memSpec.tCK)", None),
    ("Bank", ["REFPB"], ["REFPB"], "memSpec.tRFCPB", None),
    ("Rank", ["REFPB"], ["ACT"], "(memSpec.tRREFD - memSpec.tCK)", None),
    ("Rank", ["REFPB"], ["REFPB"], "memSpec.tRREFD", None),
    ("Rank", ["REFPB"], ["REFAB", "PREAB", "SREFEN"], "memSpec.tRFCPB", None),
    ("Rank", ["REFPB"], ["PDEA", "PDEP"], "tREFPDE", None),
    ("Rank", ["PDEA"], ["PDXA"], "memSpec.tPD", None),
    ("Rank", ["PDEP"], ["PDXP"], "memSpec.tPD", None),
    ("Rank", ["PDXA"], ["PDEA"], "memSpec.tCKE", None),
    ("Rank", ["PDXA"], ["REFPB", "PREPB", "PREAB", "RD", "RDA", "WR", "WRA"], "memSpec.tXP", None),
    ("Rank", ["PDXA"], ["ACT"], "(memSpec.tXP - memSpec.tCK)", None),
    ("Rank", ["PDXP"], ["PDEP"], "memSpec.tCKE", None),
    ("Rank", ["PDXP"], ["REFAB", "REFPB", "SREFEN"], "memSpec.tXP", None),
    ("Rank", ["PDXP"], ["ACT"], "(memSpec.tXP - memSpec.tCK)", None),
    ("Rank", ["SREFEN"], ["SREFEX"], "memSpec.tCKESR", None),
    ("Rank", ["SREFEX"], ["ACT"], "(memSpec.tXS - memSpec.tCK)", None),
    ("Rank", ["SREFEX"], ["REFPB", "REFAB", "PDEP", "SREFEN"], "memSpec.tXS", None),
]
HB_FAW_BLOCK = """        // tFAW counts ACT (from its second rising CK edge) and REFpb (JESD270-4A Table 108 note 10)
        last4ActivatesOnRank[rank].push(command == Command::ACT ? currentTime + memSpec.tCK : currentTime);

        if (last4ActivatesOnRank[rank].size() >= 4)
        {
            const sc_time windowEnd = last4ActivatesOnRank[rank].front() + memSpec.tFAW;
            {
                sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
                earliestTimeToStart = std::max(earliestTimeToStart, windowEnd - memSpec.tCK);
            }
            {
                sc_time &earliestTimeToStart = nextCommandByRank[Command::REFPB][rank];
                earliestTimeToStart = std::max(earliestTimeToStart, windowEnd);
            }

            last4ActivatesOnRank[rank].pop();
        }

"""
# Power-down and self refresh are common to both pseudo channels (JESD270-4A 3.1.2, JESD238 3.1.2);
# DRAMSys' power-down managers act per rank, so these commands are rejected by the checker.
HB_PD = {"PDEA", "PDEP", "SREFEN", "PDXA", "PDXP", "SREFEX"}
HB = [(lvl, src, [d for d in dst if d not in HB_PD], e, c) for lvl, src, dst, e, c in HB
      if not set(src) & HB_PD and [d for d in dst if d not in HB_PD]]
HB_STD = {"rank_name": "Rank", "faw_block": HB_FAW_BLOCK}

HB_REFPB_SET = """        // Stack (REFPB,REFPB) memSpec.tRFCPB [BankwiseRefreshCounter(counter=0, inversed=False)] SameComponent()
        {
            if (bankwiseRefreshCounter[stack] == 0)
            {
            const sc_time constraint = currentTime + memSpec.tRFCPB;
            sc_time &earliestTimeToStart = nextCommandByStack[Command::REFPB][stack];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }

"""


def main(outdir):
    # LPDDR5
    body = emit_switch(LP5, LP5_STD, [], {"REFP2B": LP5_FAW_PUSH, "REFPB": LP5_FAW_PUSH})
    act_rows = sorted(expand(LP5_ACT)["ACT"], key=lambda r: ORDER[r[0]])
    activate = "".join(block(level, "ACT", dst, expr, cond, LP5_STD) for level, dst, expr, cond in act_rows)
    activate += LP5_FAW_PUSH
    activate_delay = emit_activate_delay(act_rows)
    derived = "".join(f"    {n} = {e};\n" for n, e in LP5_DERIVED)
    members = "".join(f"    sc_core::sc_time {n};\n" for n, _ in LP5_DERIVED)
    open(f"{outdir}/CheckerLPDDR5.cpp", "w").write(LP5_CPP.format(
        license=LICENSE, derived=derived, body=body, activate=activate, activate_delay=activate_delay))
    open(f"{outdir}/CheckerLPDDR5.h", "w").write(LP5_H.format(license=LICENSE, members=members))
    # HBM3/4
    body = emit_switch(HB, HB_STD, ["ACT", "REFPB"], {"REFPB": HB_REFPB_SET})
    open(f"{outdir}/CheckerHBM3_4.cpp", "w").write(HB_CPP.format(license=LICENSE, body=body))


LP5_H = """{license}
#ifndef CHECKERLPDDR5_H
#define CHECKERLPDDR5_H

#include "DRAMSys/configuration/memspec/MemSpecLPDDR5.h"
#include "DRAMSys/controller/checker/CheckerIF.h"

#include <deque>
#include <optional>
#include <vector>

namespace DRAMSys
{{

class CheckerLPDDR5 final : public CheckerIF
{{
public:
    explicit CheckerLPDDR5(const MemSpecLPDDR5& memSpec);
    [[nodiscard]] sc_core::sc_time timeToSatisfyConstraints(Command command, const tlm::tlm_generic_payload& payload, int controllerID) const override;
    void insert(Command command, const tlm::tlm_generic_payload& payload, int controllerID) override;

private:
    const MemSpecLPDDR5& memSpec;

{members}
    template<typename T>
    using CommandArray = std::array<T, Command::END_ENUM>;
    template<typename T>
    using BankVector = ControllerVector<Bank, T>;
    template<typename T>
    using BankGroupVector = ControllerVector<BankGroup, T>;
    template<typename T>
    using RankVector = ControllerVector<Rank, T>;

    CommandArray<BankVector<sc_core::sc_time>> nextCommandByBank;
    CommandArray<BankGroupVector<sc_core::sc_time>> nextCommandByBankGroup;
    CommandArray<RankVector<sc_core::sc_time>> nextCommandByRank;

    // ACT-2 and REFpb times of the last (up to three) tFAW events
    RankVector<std::deque<sc_core::sc_time>> last4ActivatesOnRank;

    // ACTIVATE whose ACT-2 still needs a free CA cycle, at the latest at deadline (ACT-1 + tAAD)
    struct PendingActivate
    {{
        sc_core::sc_time deadline;
        Bank bank;
        BankGroup bankGroup;
        Rank rank;
    }};

    // Per controller (channel): CA bus and pending ACT-2s, oldest first
    std::vector<sc_core::sc_time> nextCommandOnBus;
    std::vector<std::deque<PendingActivate>> pendingActivate2;

    [[nodiscard]] Bank getPairedBank(Bank bank) const;
    [[nodiscard]] sc_core::sc_time activate2Bound(const PendingActivate& activate, const std::deque<PendingActivate>& pending,
                                                  const std::vector<sc_core::sc_time>& times) const;
    [[nodiscard]] bool placeActivates(const sc_core::sc_time& slot, const PendingActivate* newActivate, int controllerID,
                                      std::vector<sc_core::sc_time>& times) const;
    [[nodiscard]] bool slotAllowed(Command command, Bank bank, BankGroup bankGroup, Rank rank, const sc_core::sc_time& slot,
                                   int controllerID) const;
    [[nodiscard]] std::optional<sc_core::sc_time> activateDelay(Command command, bool sameBank, bool samePair,
                                                                bool sameBankGroup, bool sameRank) const;
    void applyActivate(const PendingActivate& activate, const sc_core::sc_time& currentTime);
}};

}} // namespace DRAMSys

#endif // CHECKERLPDDR5_H
"""

LP5_CPP = """{license}
#include "CheckerLPDDR5.h"
#include "DRAMSys/common/DebugManager.h"
#include "DRAMSys/common/utils.h"

#include <algorithm>
#include <cmath>

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{{

namespace
{{

sc_time positiveDifference(const sc_time& minuend, const sc_time& subtrahend)
{{
    return minuend > subtrahend ? minuend - subtrahend : SC_ZERO_TIME;
}}

}} // namespace

CheckerLPDDR5::CheckerLPDDR5(const MemSpecLPDDR5& memSpec) : memSpec(memSpec)
{{
    nextCommandByBank.fill({{BankVector<sc_time>(memSpec.numberOfChannels * memSpec.banksPerChannel, SC_ZERO_TIME)}});
    nextCommandByBankGroup.fill({{BankGroupVector<sc_time>(memSpec.numberOfChannels * memSpec.bankGroupsPerChannel, SC_ZERO_TIME)}});
    nextCommandByRank.fill({{RankVector<sc_time>(memSpec.numberOfChannels * memSpec.ranksPerChannel, SC_ZERO_TIME)}});
    last4ActivatesOnRank = RankVector<std::deque<sc_time>>(memSpec.numberOfChannels * memSpec.ranksPerChannel);
    nextCommandOnBus = std::vector<sc_time>(memSpec.numberOfChannels, SC_ZERO_TIME);
    pendingActivate2 = std::vector<std::deque<PendingActivate>>(memSpec.numberOfChannels);

{derived}}}

Bank CheckerLPDDR5::getPairedBank(Bank bank) const
{{
    // REFP2B payloads carry the lower bank of a pair; the partner is per2BankOffset banks above
    const unsigned offset = memSpec.getPer2BankOffset();
    const auto bankInRank = static_cast<unsigned>(static_cast<std::size_t>(bank) % memSpec.banksPerRank);
    if (offset == 0)
        return bank;
    if ((bankInRank / offset) % 2 == 0)
        return Bank(static_cast<std::size_t>(bank) + offset);
    return Bank(static_cast<std::size_t>(bank) - offset);
}}

std::optional<sc_time> CheckerLPDDR5::activateDelay(Command command, bool sameBank, bool samePair,
                                                    bool sameBankGroup, bool sameRank) const
{{
    sc_time delay = SC_ZERO_TIME;
{activate_delay}
    if (delay == SC_ZERO_TIME)
        return std::nullopt;
    return delay;
}}

sc_time CheckerLPDDR5::activate2Bound(const PendingActivate& activate, const std::deque<PendingActivate>& pending,
                                      const std::vector<sc_time>& times) const
{{
    // Earliest ACT-2: constraints of the commands inserted so far and of the ACT-2s placed before it (times)
    sc_time bound = std::max({{nextCommandByBank[Command::ACT][activate.bank],
                              nextCommandByBankGroup[Command::ACT][activate.bankGroup],
                              nextCommandByRank[Command::ACT][activate.rank]}});
    std::vector<sc_time> fawEvents(last4ActivatesOnRank[activate.rank].begin(), last4ActivatesOnRank[activate.rank].end());
    for (std::size_t i = 0; i < times.size(); i++)
    {{
        const PendingActivate& previous = pending[i];
        if (auto delay = activateDelay(Command::ACT, previous.bank == activate.bank, false,
                                       previous.bankGroup == activate.bankGroup, previous.rank == activate.rank))
            bound = std::max(bound, times[i] + *delay);
        if (previous.rank == activate.rank)
            fawEvents.push_back(times[i]);
    }}
    if (fawEvents.size() >= 4)
    {{
        std::sort(fawEvents.begin(), fawEvents.end());
        bound = std::max(bound, fawEvents[fawEvents.size() - 4] + memSpec.tFAW);
    }}
    return bound;
}}

bool CheckerLPDDR5::placeActivates(const sc_time& slot, const PendingActivate* newActivate, int controllerID,
                                   std::vector<sc_time>& times) const
{{
    // ACT-2s take free CA cycles in order, each at its earliest time: idle cycles before slot, then cycles after
    // slot (slot itself is taken by the command). Fails if an ACT-2 would miss its tAAD deadline.
    const std::deque<PendingActivate>& pending = pendingActivate2[controllerID];
    times.clear();
    sc_time cycle = nextCommandOnBus[controllerID];
    auto place = [&](const PendingActivate& activate, const sc_time& notBefore)
    {{
        sc_time time = std::max({{cycle, notBefore, activate2Bound(activate, pending, times)}});
        if (time == slot)
            time += memSpec.tCK;
        if (time > activate.deadline)
            return false;
        times.push_back(time);
        cycle = time + memSpec.tCK;
        return true;
    }};
    for (const PendingActivate& activate : pending)
    {{
        if (!place(activate, SC_ZERO_TIME))
            return false;
    }}
    return newActivate == nullptr || place(*newActivate, slot + memSpec.tCK);
}}

bool CheckerLPDDR5::slotAllowed(Command command, Bank bank, BankGroup bankGroup, Rank rank, const sc_time& slot,
                                int controllerID) const
{{
    std::vector<sc_time> times;
    if (command == Command::ACT)
    {{
        // ACT-1 itself has no timing; its ACT-2 needs a free CA cycle within tAAD that meets all constraints
        const PendingActivate activate{{slot + memSpec.tAAD, bank, bankGroup, rank}};
        return placeActivates(slot, &activate, controllerID, times);
    }}
    if (!placeActivates(slot, nullptr, controllerID, times))
        return false;

    // Timings that start at a pending ACT-2
    const std::deque<PendingActivate>& pending = pendingActivate2[controllerID];
    const bool countsForFaw = command == Command::REFP2B || command == Command::REFPB;
    std::vector<sc_time> fawEvents(last4ActivatesOnRank[rank].begin(), last4ActivatesOnRank[rank].end());
    for (std::size_t i = 0; i < pending.size(); i++)
    {{
        const PendingActivate& activate = pending[i];
        const bool samePair = activate.bank == bank || activate.bank == getPairedBank(bank);
        if (auto delay = activateDelay(command, activate.bank == bank, samePair, activate.bankGroup == bankGroup,
                                       activate.rank == rank))
        {{
            if (slot < times[i] + *delay)
                return false;
        }}
        if (countsForFaw && activate.rank == rank)
            fawEvents.push_back(times[i]);
    }}
    if (countsForFaw && fawEvents.size() >= 4)
    {{
        std::sort(fawEvents.begin(), fawEvents.end());
        if (slot < fawEvents[fawEvents.size() - 4] + memSpec.tFAW)
            return false;
    }}
    return true;
}}

void CheckerLPDDR5::applyActivate(const PendingActivate& activate, const sc_time& currentTime)
{{
    // currentTime is the CA cycle of ACT-2
    const Bank bank = activate.bank;
    const Bank pairedBank = getPairedBank(bank);
    const Rank rank = activate.rank;
    [[maybe_unused]] const BankGroup bankGroup = activate.bankGroup;

{activate}}}

sc_time CheckerLPDDR5::timeToSatisfyConstraints(Command command, const tlm_generic_payload& payload, int controllerID) const
{{
    // State is indexed across all channels: component index = controllerID * componentsPerChannel + index in channel
    const Bank bank = Bank(controllerID * memSpec.banksPerChannel + static_cast<std::size_t>(ControllerExtension::getBank(payload)));
    const BankGroup bankGroup = BankGroup(controllerID * memSpec.bankGroupsPerChannel + static_cast<std::size_t>(ControllerExtension::getBankGroup(payload)));
    const Rank rank = Rank(controllerID * memSpec.ranksPerChannel + static_cast<std::size_t>(ControllerExtension::getRank(payload)));

    sc_time earliestTimeToStart = sc_time_stamp();

    if (command == Command::ACT)
    {{
        // the constraints bound ACT-2, which may follow ACT-1 by up to tAAD
        const sc_time activate2 = std::max({{nextCommandByBank[command][bank], nextCommandByBankGroup[command][bankGroup],
                                            nextCommandByRank[command][rank]}});
        if (activate2 > memSpec.tAAD)
            earliestTimeToStart = std::max(earliestTimeToStart, activate2 - memSpec.tAAD);
    }}
    else
    {{
        earliestTimeToStart = std::max(earliestTimeToStart, nextCommandByBank[command][bank]);
        if (command == Command::REFP2B)
            earliestTimeToStart = std::max(earliestTimeToStart, nextCommandByBank[command][getPairedBank(bank)]);
        earliestTimeToStart = std::max(earliestTimeToStart, nextCommandByBankGroup[command][bankGroup]);
        earliestTimeToStart = std::max(earliestTimeToStart, nextCommandByRank[command][rank]);
    }}
    earliestTimeToStart = std::max(earliestTimeToStart, nextCommandOnBus[controllerID]);
    while (!slotAllowed(command, bank, bankGroup, rank, earliestTimeToStart, controllerID))
        earliestTimeToStart += memSpec.tCK;

    return earliestTimeToStart;
}}

void CheckerLPDDR5::insert(Command command, const tlm_generic_payload& payload, int controllerID)
{{
    const std::size_t bankRelative = static_cast<std::size_t>(ControllerExtension::getBank(payload));
    const Bank bank = Bank(controllerID * memSpec.banksPerChannel + bankRelative);
    const Bank pairedBank = getPairedBank(bank);
    const BankGroup bankGroup = BankGroup(controllerID * memSpec.bankGroupsPerChannel + static_cast<std::size_t>(ControllerExtension::getBankGroup(payload)));
    const std::size_t rankOffset = controllerID * memSpec.ranksPerChannel;
    const Rank rank = Rank(rankOffset + static_cast<std::size_t>(ControllerExtension::getRank(payload)));
    const unsigned burstLength = command.isCasCommand() ? ControllerExtension::getBurstLength(payload) : 0;
    sc_time& nextCommandOnChannelBus = nextCommandOnBus[controllerID];
    std::deque<PendingActivate>& pending = pendingActivate2[controllerID];

    PRINTDEBUGMESSAGE("CheckerLPDDR5", "Changing state on bank " + std::to_string(bankRelative)
                      + " command is " + command.toString());

    const sc_time& currentTime = sc_time_stamp();

    // ACT-2s placed in the idle CA cycles since the previous command (JESD209-5: ACT-2 within tAAD of ACT-1); the
    // timings that start at an ACTIVATE start there
    const PendingActivate activate{{currentTime + memSpec.tAAD, bank, bankGroup, rank}};
    std::vector<sc_time> activate2Times;
    if (!placeActivates(currentTime, command == Command::ACT ? &activate : nullptr, controllerID, activate2Times))
        SC_REPORT_FATAL("CheckerLPDDR5", "ACT-2 missed its tAAD window");
    for (const sc_time& activate2 : activate2Times)
    {{
        if (activate2 > currentTime || pending.empty())
            break;
        applyActivate(pending.front(), activate2);
        pending.pop_front();
    }}

    switch (command)
    {{
    case Command::ACT:
        pending.push_back(activate);
        break;
{body}    default:
        SC_REPORT_FATAL("CheckerLPDDR5", ("Command " + command.toString() + " is not supported for LPDDR5").c_str());
        break;
    }}
    nextCommandOnChannelBus = std::max(nextCommandOnChannelBus, currentTime + memSpec.getCommandLength(command));
}}

}} // namespace DRAMSys
"""

HB_CPP = """{license}
#include "CheckerHBM3_4.h"
#include "DRAMSys/common/DebugManager.h"
#include "DRAMSys/common/utils.h"

#include <algorithm>

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{{

CheckerHBM3_4::CheckerHBM3_4(const MemSpecHBM3_4& memSpec) : memSpec(memSpec)
{{
    nextCommandByBank.fill({{BankVector<sc_time>(memSpec.numberOfChannels * memSpec.banksPerChannel, SC_ZERO_TIME)}});
    nextCommandByBankGroup.fill({{BankGroupVector<sc_time>(memSpec.numberOfChannels * memSpec.bankGroupsPerChannel, SC_ZERO_TIME)}});
    nextCommandByRank.fill({{RankVector<sc_time>(memSpec.numberOfChannels * memSpec.ranksPerChannel, SC_ZERO_TIME)}});
    nextCommandByStack.fill({{StackVector<sc_time>(memSpec.numberOfChannels * memSpec.ranksPerChannel * memSpec.stacksPerChannel, SC_ZERO_TIME)}});
    last4ActivatesOnRank = RankVector<std::queue<sc_time>>(memSpec.numberOfChannels * memSpec.ranksPerChannel);
    bankwiseRefreshCounter = StackVector<unsigned>(memSpec.numberOfChannels * memSpec.ranksPerChannel * memSpec.stacksPerChannel, 0);
    nextCommandOnRasBus = std::vector<sc_time>(memSpec.numberOfChannels, SC_ZERO_TIME);
    nextCommandOnCasBus = std::vector<sc_time>(memSpec.numberOfChannels, SC_ZERO_TIME);

    tBURST = ((memSpec.defaultBurstLength / memSpec.dataRate) * memSpec.tCK);
    tWRPRE = memSpec.tWL + tBURST + memSpec.tWR;
    tWRRDS = memSpec.tWL + tBURST + memSpec.tWTRS;
    tWRRDL = memSpec.tWL + tBURST + memSpec.tWTRL;
}}

Stack CheckerHBM3_4::getStackOfPseudoChannel(const tlm_generic_payload& payload, int controllerID) const
{{
    // SID index unique per pseudo channel across all channels
    const auto rank = controllerID * memSpec.ranksPerChannel + static_cast<unsigned>(ControllerExtension::getRank(payload));
    const auto stack = static_cast<unsigned>(ControllerExtension::getStack(payload));
    return Stack(rank * memSpec.stacksPerChannel + stack);
}}

sc_time CheckerHBM3_4::timeToSatisfyConstraints(Command command, const tlm_generic_payload& payload, int controllerID) const
{{
    // State is indexed across all channels: component index = controllerID * componentsPerChannel + index in channel
    const Bank bank = Bank(controllerID * memSpec.banksPerChannel + static_cast<std::size_t>(ControllerExtension::getBank(payload)));
    const BankGroup bankGroup = BankGroup(controllerID * memSpec.bankGroupsPerChannel + static_cast<std::size_t>(ControllerExtension::getBankGroup(payload)));
    const Rank rank = Rank(controllerID * memSpec.ranksPerChannel + static_cast<std::size_t>(ControllerExtension::getRank(payload)));
    const Stack stack = getStackOfPseudoChannel(payload, controllerID);

    sc_time earliestTimeToStart = sc_time_stamp();

    earliestTimeToStart = std::max(earliestTimeToStart, nextCommandByBank[command][bank]);
    earliestTimeToStart = std::max(earliestTimeToStart, nextCommandByBankGroup[command][bankGroup]);
    earliestTimeToStart = std::max(earliestTimeToStart, nextCommandByRank[command][rank]);
    earliestTimeToStart = std::max(earliestTimeToStart, nextCommandByStack[command][stack]);
    if (command.isRasCommand())
    {{
        earliestTimeToStart = std::max(earliestTimeToStart, nextCommandOnRasBus[controllerID]);
    }}
    if (command.isCasCommand())
    {{
        earliestTimeToStart = std::max(earliestTimeToStart, nextCommandOnCasBus[controllerID]);
    }}
    // Only PREpb and PREab may use a falling CK edge; every other command starts on a rising edge
    // (JESD270-4A / JESD238 row and column command truth tables)
    if (command != Command::PREPB && command != Command::PREAB)
        earliestTimeToStart = alignAtNext(earliestTimeToStart, memSpec.tCK);

    return earliestTimeToStart;
}}

void CheckerHBM3_4::insert(Command command, const tlm_generic_payload& payload, int controllerID)
{{
    const std::size_t bankRelative = static_cast<std::size_t>(ControllerExtension::getBank(payload));
    const Bank bank = Bank(controllerID * memSpec.banksPerChannel + bankRelative);
    const BankGroup bankGroup = BankGroup(controllerID * memSpec.bankGroupsPerChannel + static_cast<std::size_t>(ControllerExtension::getBankGroup(payload)));
    const Rank rank = Rank(controllerID * memSpec.ranksPerChannel + static_cast<std::size_t>(ControllerExtension::getRank(payload)));
    const Stack stack = getStackOfPseudoChannel(payload, controllerID);

    PRINTDEBUGMESSAGE("CheckerHBM3_4", "Changing state on bank " + std::to_string(bankRelative)
                      + " command is " + command.toString());

    const sc_time& currentTime = sc_time_stamp();
    if (command == Command::REFPB)
    {{
        // Per-bank refresh sets are counted per SID (JESD270-4A Table 38, note 3)
        bankwiseRefreshCounter[stack] = (bankwiseRefreshCounter[stack] + 1) % memSpec.banksPerStack;
    }}

    switch (command)
    {{
{body}    default:
        SC_REPORT_FATAL("CheckerHBM3_4", ("Command " + command.toString() + " is not supported for HBM3/HBM4: "
                                          "power-down and self refresh are common to both pseudo channels "
                                          "(use PowerDownPolicy NoPowerDown), refresh management is not modeled").c_str());
        break;
    }}
    if (command.isRasCommand())
    {{
        nextCommandOnRasBus[controllerID] = std::max(nextCommandOnRasBus[controllerID], currentTime + memSpec.getCommandLength(command));
    }}
    if (command.isCasCommand())
    {{
        nextCommandOnCasBus[controllerID] = std::max(nextCommandOnCasBus[controllerID], currentTime + memSpec.getCommandLength(command));
    }}
}}

}} // namespace DRAMSys
"""

if __name__ == "__main__":
    main(sys.argv[1])
