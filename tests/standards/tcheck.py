#!/usr/bin/env python3
"""Independent JEDEC timing checker for DRAMSys command traces (.tdb) of LPDDR5(X) and HBM4.

Written from the timing tables, not from the DRAMSys checker code:
  LPDDR5:  JESD209-5B/5C relations as cited by Ramulator2 lpddr5.py and coreboot lp5.go (see sources in the
           memspec); timings that start at an ACTIVATE run from ACT-2, tFAW counts ACT-2 and REFpb
  HBM4:    JESD270-4A Table 3/6 (scopes), Table 36 (precharge/auto-precharge), Table 38 (refresh),
           Table 108 (core timings), 6.3.2.1 (ACT timings referenced to its second rising CK edge).
Checks: bank state machine (ACT to idle bank, RD/WR to the open row, PRE closes, REF needs idle banks),
every pairwise min-delay rule, rolling four-activate window, shared command bus(es), data bus overlap.

usage: tcheck.py <memspec.json> <trace.tdb> [--max-report N]
exit 0 = no violations, 1 = violations found
"""
import argparse
import json
import sqlite3
import sys
from collections import defaultdict, deque

COL = {"RD", "RDA", "WR", "WRA"}


class Checker:
    def __init__(self, spec, tck_ps):
        self.s = spec
        self.tck = tck_ps
        self.viol = []
        self.count = defaultdict(int)

    def bad(self, rule, t, msg):
        self.viol.append((t, rule, msg))
        self.count[rule] += 1

    def need(self, rule, t_now, t_prev, nck, msg=""):
        """t_now - t_prev >= nck cycles"""
        if t_prev is None:
            return
        if t_now - t_prev < nck - 1e-6:
            self.bad(rule, t_now, f"{msg} gap {t_now - t_prev:g} < {nck:g} nCK")


def load(tdb):
    db = sqlite3.connect(tdb)
    clk, = db.execute("select clk from GeneralInfo").fetchone()
    rows = db.execute(
        "select PhaseName, PhaseBegin, Rank, BankGroup, Bank, Row, BurstLength from Phases "
        "where PhaseName not in ('REQ','RESP') order by PhaseBegin, ID").fetchall()
    return clk, rows


# ------------------------------------------------------------------------------------------ LPDDR5
def check_lpddr5(ms, clk, rows):
    a, t = ms["memarchitecturespec"], ms["memtimingspec"]
    c = Checker(ms, clk)
    nb, ngrp = a["nbrOfBanks"], max(1, a["nbrOfBankGroups"])
    bpg = nb // ngrp
    off = a["per2BankOffset"]
    RL, WL = t["RL"], t["WL"]

    def bln(bl, kind):  # kind min/max/L/S, in nCK
        return t[f"BL_n_{kind}_{bl}"]

    open_row = {}
    last = defaultdict(lambda: None)    # (cmd, scope...) -> time (nCK)
    last_bl = {}
    bus_free = 0.0
    data_busy = []      # (start, end, kind)
    pend_pre = {}       # bank -> (earliest internal precharge time) for RDA/WRA
    faw_events = defaultdict(list)      # rank -> ACT-2 and REFpb times
    last_act2 = -1
    AAD, DQO = 8, t["WCK2DQO"]

    # ACT-2: each ACTIVATE needs a CA cycle within (ACT-1, ACT-1 + tAAD] that no command or earlier ACT-2 uses and
    # that meets every timing ending at the ACTIVATE; the earliest such cycle is taken (an earlier ACT-2 only
    # relaxes the timings that start at it). Without one, the first free cycle is taken and the rules are reported.
    times = []
    for name, tb, *_ in rows:
        tn = tb / clk
        if abs(tn - round(tn)) > 1e-6:
            c.bad("cycle-alignment", tn, f"{name} not on a CK edge")
        times.append(round(tn))
    ca_used = set(times)

    def after(t_prev, nck):
        return -1 if t_prev is None else t_prev + nck

    def place_act2(ta, bank, rank):
        ev = sorted(faw_events[rank])
        bound = max(after(pend_pre.get(bank), t["RPpb"]), after(last[("ACT", bank)], t["RCpb"]),
                    after(last[("PREPB", bank)], t["RPpb"]), after(last[("PREAB", rank)], t["RPab"]),
                    after(last[("ACT", "rank", rank)], t["RRD"]), after(last[("REFAB", rank)], t["RFCab"]),
                    after(last[("REFpb", bank)], t["RFCpb"]), after(last[("REFpb", "rank", rank)], t["pbR2act"]),
                    ev[-4] + t["FAW"] if len(ev) >= 4 else -1)
        free = [s for s in range(ta + 1, ta + AAD + 1) if s not in ca_used]
        if not free:
            c.bad("ACT-2 CA slot within tAAD", ta, "no free CA cycle")
            return ta + AAD
        return next((s for s in free if s >= bound), free[0])

    for i, (name, tb, rank, bg, bank, row, bl) in enumerate(rows):
        tn = times[i]
        # command bus: every command (ACT-1 for ACTIVATE) takes one CA cycle; ACT-2 was placed above
        if tn < bus_free - 1e-9:
            c.bad("cmd-bus", tn, f"{name} overlaps previous command")
        bus_free = tn + 1
        # bank state
        if name == "ACT":
            # timings that end or start at an ACTIVATE refer to its ACT-2; no ACT-1 between another
            # ACTIVATE's ACT-1 and ACT-2 (JESD209-5C 7.3.1)
            if last_act2 > tn:
                c.bad("ACT-1 before previous ACT-2", tn, f"previous ACT-2 at {last_act2}")
            a2 = place_act2(tn, bank, rank)
            last_act2 = a2
            ca_used.add(a2)
            if bank in open_row:
                c.bad("state", tn, f"ACT to open bank {bank}")
            if bank in pend_pre:
                c.need("RDA/WRA->ACT", a2, pend_pre[bank], t["RPpb"], f"bank {bank}")
                del pend_pre[bank]
            c.need("ACT->ACT same bank tRC", a2, last[("ACT", bank)], t["RCpb"])
            c.need("PRE->ACT tRPpb", a2, last[("PREPB", bank)], t["RPpb"])
            c.need("PREA->ACT tRPab", a2, last[("PREAB", rank)], t["RPab"])
            c.need("ACT->ACT tRRD", a2, last[("ACT", "rank", rank)], t["RRD"])
            c.need("REFab->ACT tRFCab", a2, last[("REFAB", rank)], t["RFCab"])
            c.need("REFpb->ACT same bank tRFCpb", a2, last[("REFpb", bank)], t["RFCpb"])
            c.need("REFpb->ACT tpbR2act", a2, last[("REFpb", "rank", rank)], t["pbR2act"])
            faw_events[rank].append(a2)
            open_row[bank] = row
            last[("ACT", bank)] = a2
            last[("ACT", "rank", rank)] = a2
        elif name in COL:
            if open_row.get(bank) != row:
                c.bad("state", tn, f"{name} bank {bank} row {row} not open (open={open_row.get(bank)})")
            rd = name.startswith("RD")
            # tRCD: ACT-2 to READ RCD_L, to WRITE RCD_S (equal on LPDDR5, JESD209-5C Table 396 on LPDDR5X)
            c.need("ACT->RD tRCD" if rd else "ACT->WR tRCD", tn, last[("ACT", bank)], t["RCD_L" if rd else "RCD_S"])
            for prev in ("RD", "WR"):
                for pbl in (16, 32):
                    pt_bg = last[(prev, "bg", bank // bpg, pbl)]
                    pt_rk = last[(prev, "rank", rank, pbl)]
                    if prev == "RD" and rd:
                        c.need("RD->RD same BG tCCD_L", tn, pt_bg, bln(pbl, "L"))
                        c.need("RD->RD tCCD_S", tn, pt_rk, bln(pbl, "S"))
                    elif prev == "WR" and not rd:
                        c.need("WR->WR same BG tCCD_L", tn, pt_bg, bln(pbl, "L"))
                        c.need("WR->WR tCCD_S", tn, pt_rk, bln(pbl, "S"))
                    elif prev == "RD" and not rd:
                        # same BG uses BL/n_max (JESD209-5C Tables 347/350 as cited by Ramulator2)
                        c.need("RD->WR same BG (RL+BL/n_max+2-WL)", tn, pt_bg, RL + bln(pbl, "max") + 2 - WL)
                        c.need("RD->WR diff BG (RL+BL/n_min+2-WL)", tn, pt_rk, RL + bln(pbl, "min") + 2 - WL)
                    else:
                        c.need("WR->RD same BG (WL+BL/n_max+tWTR_L)", tn, pt_bg,
                               WL + bln(pbl, "max") + t["WTR_L"])
                        c.need("WR->RD diff BG (WL+BL/n_min+tWTR_S)", tn, pt_rk,
                               WL + bln(pbl, "min") + t["WTR_S"])
            if name == "RDA":
                for pbl in (16, 32):
                    wr = last[("WR", "bank", bank, pbl)]
                    # internal precharge of RDA must not precede write recovery of an earlier WR
                    if wr is not None:
                        c.need("WR->RDA write recovery", tn + bln(bl, "min") + t["RBTP"], wr,
                               WL + bln(pbl, "min") + 1 + t["WR"])
            ds = tn + (RL + DQO if rd else WL)
            de = ds + bln(bl, "min")
            data_busy.append((ds, de, name))
            key = "RD" if rd else "WR"
            last[(key, "bg", bank // bpg, bl)] = tn
            last[(key, "rank", rank, bl)] = tn
            last[(key, "bank", bank, bl)] = tn
            last_bl[(key, bank)] = bl
            if name in ("RDA", "WRA"):
                ip = tn + (bln(bl, "min") + t["RBTP"] if rd else WL + bln(bl, "min") + 1 + t["WR"])
                ip = max(ip, last[("ACT", bank)] + t["RAS"])
                pend_pre[bank] = ip
                open_row.pop(bank, None)
        elif name in ("PREPB", "PREAB"):
            targets = [bank] if name == "PREPB" else [b for b in list(open_row) if b // nb == rank]
            for b in targets:
                c.need("ACT->PRE tRAS", tn, last[("ACT", b)], t["RAS"], f"bank {b}")
                for pbl in (16, 32):
                    c.need("RD->PRE (BL/n+nRBTP)", tn, last[("RD", "bank", b, pbl)], bln(pbl, "min") + t["RBTP"])
                    c.need("WR->PRE (WL+BL/n+1+nWR)", tn, last[("WR", "bank", b, pbl)],
                           WL + bln(pbl, "min") + 1 + t["WR"])
                open_row.pop(b, None)
            c.need("PRE->PRE tPPD", tn, last[("PRE", rank)], t["PPD"])
            last[("PRE", rank)] = tn
            if name == "PREPB":
                last[("PREPB", bank)] = tn
            else:
                last[("PREAB", rank)] = tn
        elif name == "REFAB":
            if any(b // nb == rank for b in open_row):
                c.bad("state", tn, "REFab with open banks")
            for b, ip in list(pend_pre.items()):
                c.need("RDA/WRA->REFab", tn, ip, t["RPpb"], f"bank {b}")
                del pend_pre[b]
            c.need("REFab->REFab tRFCab", tn, last[("REFAB", rank)], t["RFCab"])
            c.need("PREA->REFab tRPab", tn, last[("PREAB", rank)], t["RPab"])
            for b in range(rank * nb, (rank + 1) * nb):
                c.need("PRE->REFab tRPpb", tn, last[("PREPB", b)], t["RPpb"])
                c.need("ACT->REFab tRC", tn, last[("ACT", b)], t["RCpb"])
            c.need("REFpb->REFab tRFCpb", tn, last[("REFpb", "rank", rank)], t["RFCpb"])
            last[("REFAB", rank)] = tn
        elif name in ("REFP2B", "REFPB"):
            # BG and 16B modes refresh a bank pair per REFpb, 8B mode a single bank
            if (name == "REFP2B") != (nb == 16):
                c.bad("refresh mode", tn, f"{name} with {nb} banks")
            pair = (bank, bank + off) if name == "REFP2B" else (bank,)
            for b in pair:
                if b in open_row:
                    c.bad("state", tn, f"REFpb to open bank {b}")
                if b in pend_pre:
                    c.need("RDA/WRA->REFpb", tn, pend_pre.pop(b), t["RPpb"])
                c.need("PRE->REFpb tRPpb", tn, last[("PREPB", b)], t["RPpb"])
                c.need("ACT->REFpb same bank tRC", tn, last[("ACT", b)], t["RCpb"])
                c.need("REFpb same bank tRFCpb", tn, last[("REFpb", b)], t["RFCpb"])
            c.need("ACT->REFpb tRRD", tn, last[("ACT", "rank", rank)], t["RRD"])
            c.need("REFpb->REFpb tpbR2pbR", tn, last[("REFpb", "rank", rank)], t["pbR2pbR"])
            c.need("REFab->REFpb tRFCab", tn, last[("REFAB", rank)], t["RFCab"])
            c.need("PREA->REFpb tRPab", tn, last[("PREAB", rank)], t["RPab"])
            for b in pair:
                last[("REFpb", b)] = tn
            last[("REFpb", "rank", rank)] = tn
            faw_events[rank].append(tn)
        else:
            c.bad("unknown-command", tn, name)
    # rolling four-activate window over ACT-2 and REFpb (JESD209-5C Figures 97-98 as cited by Ramulator2)
    for rank, ev in faw_events.items():
        ev.sort()
        for k in range(4, len(ev)):
            if ev[k] - ev[k - 4] < t["FAW"]:
                c.bad("tFAW", ev[k], f"5th ACT-2/REFpb within {ev[k] - ev[k - 4]} < {t['FAW']}")
    check_data_bus(c, data_busy)
    return c


# ------------------------------------------------------------------------------------------ HBM4
def check_hbm(ms, clk, rows):
    a, t = ms["memarchitecturespec"], ms["memtimingspec"]
    c = Checker(ms, clk)
    nb = a["nbrOfBanks"]                 # per pseudo channel, includes SIDs
    nsid = a["nbrOfStacks"]
    bpsid = nb // nsid
    ngrp = a["nbrOfBankGroups"]
    bpg = nb // ngrp
    RL, WL, BLC = t["RL"], t["WL"], a["burstLength"] // a["dataRate"]

    def sid(bank):
        return (bank % nb) // bpsid

    open_row = {}
    last = defaultdict(lambda: None)
    win = defaultdict(lambda: deque(maxlen=4))
    row_bus_free = col_bus_free = 0.0
    data_busy = defaultdict(list)
    pend_pre = {}
    refpb_count = defaultdict(int)
    last_row = None          # (name, pc, bank, start) of the latest row command

    for name, tb, pc, bg, bank, row, bl in rows:
        tn = tb / clk
        # PREpb/PREab may use a rising or falling CK edge, every other command a rising edge (Table 33/34)
        step = 0.5 if name in ("PREPB", "PREAB") else 1.0
        if abs(tn / step - round(tn / step)) > 1e-6:
            c.bad("clock-edge", tn, f"{name} not on an allowed CK edge")
        tn = round(tn / step) * step
        if name in COL:
            if tn < col_bus_free:
                c.bad("column-bus", tn, f"{name} overlaps")
            col_bus_free = tn + 1
        else:
            # row bus slots: ACT = R, F, R; PREpb, PREab, REFpb, REFab = one edge
            if tn < row_bus_free:
                c.bad("row-bus", tn, f"{name} overlaps")
            row_bus_free = tn + (1.5 if name == "ACT" else 0.5)
            if tn != int(tn):
                c.count["(info) PRE on falling edge"] += 1
                # Table 35: PREab/PREpb on the falling edge after the rising-edge row command of the same cycle
                if last_row and last_row[3] + (1.5 if last_row[0] == "ACT" else 0.5) == tn:
                    an, apc, abank, _ = last_row
                    if an in ("PDEA", "PDEP", "SREFEN", "PDXA", "PDXP", "SREFEX"):
                        ok = False
                    elif apc != pc:
                        ok = True
                    else:
                        ok = name == "PREPB" and an in ("ACT", "PREPB", "REFPB") and abank != bank
                    if not ok:
                        c.bad("Table 35 falling-edge PRE", tn, f"{name} after {an} (same PC={apc == pc})")
            last_row = (name, pc, bank, tn)
        if name == "ACT":
            ta = tn + 1                      # second rising edge
            if bank in open_row:
                c.bad("state", tn, f"ACT to open bank {bank}")
            if bank in pend_pre:
                c.need("RDA/WRA->ACT", ta, pend_pre.pop(bank), t["RP"])
            c.need("ACT->ACT same bank tRC", ta, last[("ACTe", bank)], t["RC"])
            c.need("ACT->ACT same BG tRRDL", ta, last[("ACTe", "bg", bank // bpg)], t["RRDL"])
            c.need("ACT->ACT tRRDS", ta, last[("ACTe", "pc", pc)], t["RRDS"])
            c.need("PRE->ACT tRP", ta, last[("PREPB", bank)], t["RP"])
            c.need("PREab->ACT tRP", ta, last[("PREAB", pc)], t["RP"])
            c.need("REFab->ACT tRFCab", ta, last[("REFAB", pc)], t["RFC"])
            c.need("REFpb->ACT same bank tRFCpb", ta, last[("REFPB", bank)], t["RFCPB"])
            c.need("REFpb->ACT diff bank tRREFD", ta, last[("REFPB", "pc", pc)], t["RREFD"])
            w = win[pc]
            if len(w) == 4 and ta - w[0] < t["FAW"]:
                c.bad("tFAW", tn, f"5th ACT/REFpb within {ta - w[0]} < {t['FAW']}")
            w.append(ta)
            open_row[bank] = row
            last[("ACTe", bank)] = ta
            last[("ACTe", "bg", bank // bpg)] = ta
            last[("ACTe", "pc", pc)] = ta
        elif name in COL:
            rd = name.startswith("RD")
            if open_row.get(bank) != row:
                c.bad("state", tn, f"{name} bank {bank} row {row} not open")
            c.need("ACT->RD tRCDRD" if rd else "ACT->WR tRCDWR", tn, last[("ACTe", bank)],
                   t["RCDRD"] if rd else t["RCDWR"])
            g, s = bank // bpg, (pc, sid(bank))
            if rd:
                c.need("RD->RD same BG tCCDL", tn, last[("RD", "bg", g)], t["CCDL"])
                c.need("RD->RD diff BG tCCDS", tn, last[("RD", "sid", s)], t["CCDS"])
                for other in range(nsid):
                    if other != s[1]:
                        c.need("RD->RD diff SID tCCDR", tn, last[("RD", "sid", (pc, other))], t["CCDR"])
                c.need("WR->RD same BG (WL+BL/4+tWTRL)", tn, last[("WR", "bg", g)], WL + BLC + t["WTRL"])
                c.need("WR->RD diff BG (WL+BL/4+tWTRS)", tn, last[("WR", "pc", pc)], WL + BLC + t["WTRS"])
                if name == "RDA":
                    c.need("WR->RDA same bank (WL+2+max(WR-RTP,tWTRL))", tn, last[("WR", "bank", bank)],
                           WL + BLC + max(t["WR"] - t["RTP"], t["WTRL"]))
            else:
                c.need("WR->WR same BG tCCDL", tn, last[("WR", "bg", g)], t["CCDL"])
                c.need("WR->WR tCCDS", tn, last[("WR", "pc", pc)], t["CCDS"])
                c.need("RD->WR tRTW", tn, last[("RD", "pc", pc)], t["RTW"])
            key = "RD" if rd else "WR"
            last[(key, "bg", g)] = tn
            last[(key, "sid", s)] = tn
            last[(key, "pc", pc)] = tn
            last[(key, "bank", bank)] = tn
            ds = tn + (RL if rd else WL)
            data_busy[pc].append((ds, ds + BLC, name))
            if name in ("RDA", "WRA"):
                ip = tn + (t["RTP"] if rd else WL + BLC + t["WR"])
                ip = max(ip, last[("ACTe", bank)] + t["RAS"])
                pend_pre[bank] = ip
                open_row.pop(bank, None)
        elif name in ("PREPB", "PREAB"):
            targets = [bank] if name == "PREPB" else [b for b in list(open_row) if b // nb == pc]
            for b in targets:
                c.need("ACT->PRE tRAS", tn, last[("ACTe", b)], t["RAS"])
                c.need("RD->PRE tRTP", tn, last[("RD", "bank", b)], t["RTP"])
                c.need("WR->PRE (WL+2+tWR)", tn, last[("WR", "bank", b)], WL + BLC + t["WR"])
                open_row.pop(b, None)
            c.need("PRE->PRE tPPD", tn, last[("PRE", pc)], t["PPD"])
            last[("PRE", pc)] = tn
            if name == "PREPB":
                last[("PREPB", bank)] = tn
            else:
                last[("PREAB", pc)] = tn
        elif name == "REFAB":
            if any(b // nb == pc for b in open_row):
                c.bad("state", tn, "REFab with open banks")
            for b in list(pend_pre):
                if b // nb == pc:
                    c.need("RDA/WRA->REFab", tn, pend_pre.pop(b), t["RP"])
            c.need("REFab->REFab tRFCab", tn, last[("REFAB", pc)], t["RFC"])
            c.need("PREab->REFab tRP", tn, last[("PREAB", pc)], t["RP"])
            for b in range(pc * nb, (pc + 1) * nb):
                c.need("PRE->REFab tRP", tn, last[("PREPB", b)], t["RP"])
                c.need("ACT->REFab tRC", tn, last[("ACTe", b)], t["RC"])
            c.need("REFpb->REFab tRFCpb", tn, last[("REFPB", "pc", pc)], t["RFCPB"])
            last[("REFAB", pc)] = tn
        elif name == "REFPB":
            if bank in open_row:
                c.bad("state", tn, f"REFpb to open bank {bank}")
            if bank in pend_pre:
                c.need("RDA/WRA->REFpb", tn, pend_pre.pop(bank), t["RP"])
            c.need("PRE->REFpb tRP", tn, last[("PREPB", bank)], t["RP"])
            c.need("ACT->REFpb same bank tRC", tn, last[("ACTe", bank)], t["RC"])
            c.need("ACT->REFpb diff bank tRRDS", tn, last[("ACTe", "pc", pc)], t["RRDS"])
            c.need("REFpb->REFpb same bank tRFCpb", tn, last[("REFPB", bank)], t["RFCPB"])
            sk = (pc, sid(bank))
            new_set = refpb_count[sk] % bpsid == 0 and refpb_count[sk] > 0
            c.need("REFpb->REFpb new SID set tRFCpb" if new_set else "REFpb->REFpb tRREFD", tn,
                   last[("REFPB", "sid", sk)] if new_set else last[("REFPB", "pc", pc)],
                   t["RFCPB"] if new_set else t["RREFD"])
            refpb_count[sk] += 1
            c.need("REFab->REFpb tRFCab", tn, last[("REFAB", pc)], t["RFC"])
            w = win[pc]
            if len(w) == 4 and tn - w[0] < t["FAW"]:
                c.bad("tFAW", tn, "5th ACT/REFpb in tFAW")
            w.append(tn)
            last[("REFPB", bank)] = tn
            last[("REFPB", "pc", pc)] = tn
            last[("REFPB", "sid", sk)] = tn
        else:
            c.bad("unknown-command", tn, name)
    for pc, busy in data_busy.items():
        check_data_bus(c, busy, f"pc{pc} ")
    return c


def check_data_bus(c, busy, prefix=""):
    busy.sort()
    for (s0, e0, n0), (s1, e1, n1) in zip(busy, busy[1:]):
        if s1 < e0:
            c.bad("data-bus", s1, f"{prefix}{n1} data [{s1},{e1}) overlaps {n0} [{s0},{e0})")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("memspec")
    ap.add_argument("tdb")
    ap.add_argument("--max-report", type=int, default=5)
    args = ap.parse_args()
    ms = json.load(open(args.memspec))["memspec"]
    clk, rows = load(args.tdb)
    kind = ms["memoryType"]
    if kind == "LPDDR5":
        c = check_lpddr5(ms, clk, rows)
    elif kind in ("HBM3", "HBM4"):
        c = check_hbm(ms, clk, rows)
    else:
        sys.exit(f"unsupported {kind}")
    cmds = defaultdict(int)
    for r in rows:
        cmds[r[0]] += 1
    print(f"{kind} {args.tdb.split('/')[-1]}: tCK={clk} ps, commands={len(rows)} "
          + " ".join(f"{k}={v}" for k, v in sorted(cmds.items())))
    print(f"violations={len(c.viol)}")
    for rule, n in sorted(c.count.items(), key=lambda x: -x[1]):
        print(f"  {n:7d}  {rule}")
    for tv, rule, msg in sorted(c.viol)[: args.max_report]:
        print(f"  first: t={tv} nCK  {rule}: {msg}")
    sys.exit(1 if c.viol else 0)


if __name__ == "__main__":
    main()
