#!/usr/bin/env python3
"""Write the LPDDR5 and HBM4 memspec JSONs for DRAMSys. Every number carries its source tag.

LPDDR5/LPDDR5X sources: JESD209-5C, tables cited per value in each memspec's "sources" block and in
lpddr5() below. The LPDDR5-6400 values equal Ramulator2's LPDDR5_6400 preset (CMU-SAFARI/ramulator2 7bead6c,
checked below).
HBM4 sources:
  [J]   JESD270-4A Table 107 (8.0 Gbps bin, p.192), Table 108 (p.194-198), Table 4 (p.6), Table 5/6 (p.7),
        Tables 36/38 (p.52/60), MR1-MR3 (p.21-22)
  [RAM] CMU-SAFARI/ramulator2 main (7bead6c) python/ramulator/dram/hbm4.py, HBM4_8000Mbps preset. JESD270-4A
        Table 108 lists these timings as vendor-specific ('-'); Ramulator2 marks its values as estimates.

usage: gen_memspecs.py <configs/memspec directory>
"""
import json
import math
import sys


def ru(t_ns, tck_ns):
    """round up with a 1e-9 relative guard against float noise"""
    return int(math.ceil(t_ns / tck_ns - 1e-9))


# JESD209-5C (LPDDR5/5X), CKR 4:1, x16, BG mode, DVFSC / Enhanced DVFSC / link ECC / DBI disabled:
#   tCK, RL Set 0 and nRBTP: Table 225 (7.4.8.1); WL Set A: Table 229 (7.4.8.2); nWR (x16, no DVFSC, no write
#   link ECC): Table 230 (7.4.8.3); core timing: Table 381 (9.2.1, LPDDR5) and Table 396 (9.3.1, LPDDR5X);
#   tRBTP: Table 221; BL/n: Table 339; tRTW: Tables 347/348 (8.2.2.1); tWCK2DQO_HF(max) 1600 ps: Table 462
#   (15.3); refresh: Table 240 (16Gb); tAAD: 7.3.1; organisation: Table 6
LP5_BINS = {
    # rate: (device, tCK ps = RU(1 / upper CK limit of Table 225), RL, WL, nRBTP, nWR)
    6400: ("LPDDR5", 1250, 17, 9, 4, 28),
    7500: ("LPDDR5X", 1067, 20, 11, 6, 32),
    8533: ("LPDDR5X", 938, 23, 12, 6, 37),
}
CORE = {
    # ns: tRCD (ACT-2 to READ), tRCD (ACT-2 to WRITE), tRRD, tFAW; the rest is equal in Tables 381 and 396
    "LPDDR5": dict(RCD_RD=18, RCD_WR=18, RRD=5, FAW=20, table="Table 381"),
    "LPDDR5X": dict(RCD_RD=18, RCD_WR=8, RRD=3.75, FAW=15, table="Table 396"),
}


def lpddr5(rate):
    device, tck_ps, rl, wl, nrbtp, nwr = LP5_BINS[rate]
    core = CORE[device]
    tck = tck_ps / 1000.0
    assert nrbtp == ru(max(7.5, 2 * tck) - 2 * tck, tck)        # Table 221 / Table 225 note 2
    assert nwr == max(3, ru(34, tck))                             # Table 230 = RU(tWR / tCK), tWR = max(34 ns, 3nCK)
    t = {
        "tCK": tck_ps * 1e-12,
        "RCD_L": max(2, ru(core["RCD_RD"], tck)), "RCD_S": max(2, ru(core["RCD_WR"], tck)),
        "RPpb": max(2, ru(18, tck)), "RPab": max(2, ru(21, tck)), "RAS": max(3, ru(42, tck)),
        "WR": nwr, "RBTP": nrbtp, "RL": rl, "WL": wl, "PPD": 2,
        "FAW": ru(core["FAW"], tck), "RRD": max(2, ru(core["RRD"], tck)),
        "WTR_L": max(4, ru(12, tck)), "WTR_S": max(4, ru(6.25, tck)),
        "BL_n_min_16": 2, "BL_n_max_16": 4, "BL_n_L_16": 4, "BL_n_S_16": 2,
        "BL_n_min_32": 0, "BL_n_max_32": 0, "BL_n_L_32": 0, "BL_n_S_32": 0,
        "RTW_L_32": 0, "RTW_S_32": 0,
        "RTRS": 0, "WCK2CK": 4, "WCK2DQO": ru(1.6, tck),
        "REFI": int(3906 / tck), "REFIpb": int(488 / tck),
        "RFCab": ru(280, tck), "RFCpb": ru(140, tck),
        "pbR2pbR": ru(90, tck), "pbR2act": ru(7.5, tck),
    }
    t["RCpb"] = t["RAS"] + t["RPpb"]
    t["RCab"] = t["RAS"] + t["RPab"]
    t["RTW_L_16"] = rl + t["BL_n_max_16"] + t["WCK2DQO"] - wl
    t["RTW_S_16"] = rl + t["BL_n_min_16"] + t["WCK2DQO"] - wl
    if rate == 6400:
        # the same values as Ramulator2's LPDDR5_6400 preset (nCK)
        ram = dict(nCL=17, nRCD=15, nRP=15, nRPab=17, nRAS=34, nRC=49, nWR=28, nRTP=6, nCWL=9, nWTRS=5, nWTRL=10)
        ours = dict(nCL=t["RL"], nRCD=t["RCD_L"], nRP=t["RPpb"], nRPab=t["RPab"], nRAS=t["RAS"], nRC=t["RCpb"],
                    nWR=t["WR"], nRTP=t["BL_n_min_16"] + t["RBTP"], nCWL=t["WL"], nWTRS=t["WTR_S"],
                    nWTRL=t["WTR_L"])
        assert ours == ram, (ours, ram)
    order = ["tCK", "REFI", "REFIpb", "RFCab", "RFCpb", "RAS", "RPab", "RPpb", "RCpb", "RCab", "PPD",
             "RCD_L", "RCD_S", "FAW", "RRD", "RL", "RBTP", "WL", "WR", "RTRS",
             "BL_n_min_16", "BL_n_max_16", "BL_n_L_16", "BL_n_S_16",
             "BL_n_min_32", "BL_n_max_32", "BL_n_L_32", "BL_n_S_32",
             "WTR_L", "WTR_S", "RTW_L_32", "RTW_L_16", "RTW_S_32", "RTW_S_16",
             "WCK2DQO", "WCK2CK", "pbR2act", "pbR2pbR"]
    # No public currents, supply voltages or I/O impedances: zero placeholders, so DRAMPower reports 0 J
    # for these memspecs until device values are filled in.
    power = {k: 0.0 for k in (
        "vdd1", "idd01", "idd2n1", "idd3n1", "idd4r1", "idd4w1", "idd51", "idd5pb1", "idd61", "idd6ds1", "idd2p1",
        "idd3p1", "vdd2h", "idd02h", "idd2n2h", "idd3n2h", "idd4r2h", "idd4w2h", "idd52h", "idd5pb2h", "idd62h",
        "idd6ds2h", "idd2p2h", "idd3p2h", "vdd2l", "idd02l", "idd2n2l", "idd3n2l", "idd4r2l", "idd4w2l", "idd52l",
        "idd5pb2l", "idd62l", "idd6ds2l", "idd2p2l", "idd3p2l", "vddq")}
    imp = {}
    for line in ("ck", "ca", "rdq", "wdq", "rdqs", "wck", "rdbi", "wdbi"):
        imp.update({f"{line}_termination": False, f"{line}_R_eq": 0.0, f"{line}_dyn_E": 0.0})
    spec = {
        "memspec": {
            "memoryId": memspec_name(rate),
            "memoryType": "LPDDR5",
            "memarchitecturespec": {
                "nbrOfDevices": 1, "nbrOfChannels": 1, "nbrOfRanks": 1,
                "nbrOfBanks": 16, "nbrOfBankGroups": 4,
                "nbrOfRows": 65536, "nbrOfColumns": 1024,
                "burstLength": 16, "maxBurstLength": 16, "dataRate": 2, "width": 16,
                "per2BankOffset": 8, "WCKalwaysOn": True,
            },
            "mempowerspec": power,
            "memtimingspec": {k: t[k] for k in order},
            "memimpedancespec": imp,
            "sources": lpddr5_sources(rate),
        }
    }
    return spec, t


def memspec_name(rate):
    return f"JEDEC_16Gb_{LP5_BINS[rate][0]}-{rate}_16bit_BG"


def lpddr5_sources(rate):
    device, tck_ps, rl, wl, nrbtp, nwr = LP5_BINS[rate]
    core = CORE[device]
    tab = core["table"]
    mr = {6400: "1011B", 7500: "1100B", 8533: "1101B"}[rate]
    return {
        "standard": f"JESD209-5C, {device} SDRAM, x16, BG mode, WCK:CK 4:1, DVFSC/Enhanced DVFSC, link ECC and DBI disabled",
        "architecture": "Table 6 (16Gb x16 BG mode): 4 BG x 4 banks, 65,536 rows, 1024 columns (64 x BL16); per2BankOffset 8: REFpb refreshes banks n and n+8 (7.5.1, Table 235)",
        "tCK": f"{tck_ps} ps = 1 / upper CK limit of the {rate} row of Table 225 (MR2 OP[3:0] = {mr}), rounded up to 1 ps",
        "RL/RBTP": f"Table 225 (7.4.8.1), MR2 OP[3:0] = {mr}: RL Set 0 = {rl}, nRBTP = {nrbtp} = RU(tRBTP/tCK), tRBTP = max(7.5 ns, 2nCK) - 2nCK (Table 221)",
        "WL": f"Table 229 (7.4.8.2), MR1 OP[7:4] = {mr}: WL Set A = {wl}",
        "WR": f"Table 230 (7.4.8.3), x16 without DVFSC and write link ECC: nWR = {nwr} = RU(tWR/tCK), tWR = max(34 ns, 3nCK) ({tab})",
        "RCD_L/RCD_S": f"{tab}: tRCD ACT-2 to READ max({core['RCD_RD']} ns, 2nCK) (RCD_L), ACT-2 to WRITE max({core['RCD_WR']} ns, 2nCK) (RCD_S)",
        "RPpb/RPab/RAS": f"{tab}: max(18 ns, 2nCK) / max(21 ns, 2nCK) / max(42 ns, 3nCK)",
        "RCpb/RCab": f"{tab}: tRAS + tRPpb / tRAS + tRPab",
        "RRD/FAW": f"{tab}: tRRD max({core['RRD']} ns, 2nCK), tFAW {core['FAW']} ns",
        "WTR_L/WTR_S": f"{tab}: max(12 ns, 4nCK) / max(6.25 ns, 4nCK)",
        "PPD": f"{tab}: 2 nCK",
        "BL_n_16": "Table 339, BG mode, 4:1, WCK > 1600 MHz, BL16: BL/n_min 2, BL/n_max 4; same BG 4, different BG 2",
        "BL_n_32/RTW_32": "0 = unused: BL32 in BG mode is rejected by MemSpecLPDDR5",
        "RTW_16": "Tables 347/348 (8.2.2.1, RDQS on, NT-ODT and DQ ODT off): RL + BL/n_max (same BG) or BL/n_min (different BG) + RU(tWCK2DQO(max)/tCK) - WL",
        "WCK2DQO": "RU(tWCK2DQO_HF(max) / tCK), tWCK2DQO_HF(max) = 1600 ps (Table 462, 15.3); read data starts at RL + WCK2DQO",
        "ACT-1/ACT-2": "ACT-2 within tAAD = 8 nCK of ACT-1 (7.3.1); timings that end or start at an ACTIVATE refer to ACT-2; tAAD is fixed in MemSpecLPDDR5 (not in the DRAMUtils schema)",
        "RTRS": "0 = unused, single rank",
        "REFI/REFIpb": "Table 240: 3.906 us / 488 ns, rounded down",
        "RFCab/RFCpb/pbR2pbR/pbR2act": "Table 240, 16Gb: 280 / 140 / 90 / 7.5 ns",
        "mempowerspec/memimpedancespec": "no public source for currents, supply voltages or I/O impedances: all zero, so DRAMPower reports 0 J; fill in device values for energy results",
    }


def hbm4():
    tck = 0.5  # [J] Table 107, 8.0 Gbps/pin bin, fCK 2000 MHz
    rl, wl = 20, 10  # [RAM] (MR2 RL 17-90, MR1 WL 4-19 allowed [J])
    t = {
        "tCK": tck * 1e-9,
        "RC": 90, "RAS": 57, "RCDRD": 39, "RCDWR": 19, "RRDL": 7, "RRDS": 5, "FAW": 30,
        "RTP": 12, "RP": 33, "WR": 42, "WTRL": 13, "WTRS": 9, "RL": rl, "WL": wl,
        "PL": 0,
        "DQSCK": ru(0.1 + 2.5, tck),                     # RU((tDQSS(max) + tWDQS2DQ_O(max)) / tCK)
        "CCDL": max(4, ru(2.5, tck)), "CCDS": 2, "CCDR": 3, "PPD": 2,
        "RTW": ru((rl + 8 / 4 - wl + 0.5) * tck + 2.5 - 0.1, tck),
        "XP": max(10, ru(7.5, tck)),
        "CKE": max(10, ru(7.5, tck)) + 6,               # tPD = tCKSR = tCPDED + 6 tCK
        "XS": max(10, ru(450 + 10, tck)),
        "RFC": ru(450, tck), "RFCPB": ru(280, tck), "RREFD": max(3, ru(8, tck)),
        "REFI": int(3900 / tck), "REFIPB": int(3900 / 32 / tck),
    }
    order = ["tCK", "DQSCK", "RC", "RAS", "RCDRD", "RCDWR", "RRDL", "RRDS", "FAW", "RTP", "RP", "RL",
             "WL", "PL", "WR", "CCDL", "CCDS", "CCDR", "WTRL", "WTRS", "RTW", "XP", "CKE", "XS",
             "RFC", "RFCPB", "RREFD", "REFI", "REFIPB", "PPD"]
    spec = {
        "memspec": {
            "memoryId": "JEDEC HBM4 8.0 Gbps, 32Gb/die 8-High, one channel (2 pseudo channels)",
            "memoryType": "HBM4",
            "memarchitecturespec": {
                "nbrOfRows": 16384, "nbrOfColumns": 32, "burstLength": 8, "maxBurstLength": 8,
                "dataRate": 4, "width": 32, "nbrOfChannels": 1, "nbrOfPseudoChannels": 2,
                "nbrOfDevices": 1, "nbrOfBanks": 32, "nbrOfBankGroups": 4, "nbrOfStacks": 2,
                "RAAIMT": 0, "RAAMMT": 0, "RAADEC": 0,
            },
            "memtimingspec": {k: t[k] for k in order},
            "sources": HBM4_SOURCES,
        }
    }
    return spec, t


HBM4_SOURCES = {
    "architecture": "[J] Table 4: 32Gb 8H = 8 Gb/channel, 2 PC x 32 DQ, 256-bit prefetch (BL8), RA[13:0], CA[4:0] (32 bursts, 1 KB page), SID[0] + BA[3:0] = 32 banks/PC; Table 5: 8 banks per bank group = 4 BG/PC; dataRate 4 beats/CK (WDQS = 2x CK, DDR)",
    "tCK": "[J] Table 107: 0.500 ns at 8.0 Gbps/pin",
    "CCDL": "[J] Table 108: max(4, 2.5 ns/tCK) = 5",
    "CCDS/PPD": "[J] Table 108: 2 nCK",
    "CCDR": "[J] Table 108 note 17: vendor value in tCCDS+1..tCCDS+2; minimum 3 used",
    "RTW": "[J] Table 108 note 18: RU(((RL + BL/4 - WL + 0.5) tCK + tWDQS2DQ_O(max) 2.5 ns - tWDQS2DQ_I(min) 0.1 ns) / tCK)",
    "DQSCK": "RU((tDQSS(max) 0.1 ns + tWDQS2DQ_O(max) 2.5 ns) / tCK), [J] Tables 107/108; used for tRDPDE and read data timing",
    "RFC/RFCPB": "[J] Table 108: tRFCab 450 ns (32Gb/die 8H), tRFCpb 280 ns (32Gb/die)",
    "RREFD": "[J] Table 108: max(3 tCK, 8 ns)",
    "REFI/REFIPB": "[J] Table 108: 3.9 us; 8H tREFI/32; rounded down",
    "XP": "[J] Table 108: max(10 tCK, 7.5 ns)",
    "CKE": "[J] Table 108: tPD = tCKSR = tCPDED + 6 tCK, tCPDED = max(10 tCK, 7.5 ns)",
    "XS": "[J] Table 108: max(10 tCK, tRFC + 10 ns)",
    "PL": "parity off (MR1 PL 0)",
    "RL/WL/RC/RAS/RCDRD/RCDWR/RRDL/RRDS/FAW/RTP/RP/WR/WTRL/WTRS": "vendor-specific in [J] Table 108 ('-'); values from [RAM] (estimates there): RL 20, WL 10, RC 45 ns, RAS 28.5 ns, RCDRD 19.5 ns, RCDWR 9.5 ns, RRDL 3.5 ns, RRDS 2.5 ns, FAW 15 ns, RTP 6 ns, RP 16.5 ns, WR 21 ns, WTRL 6.5 ns, WTRS 4.5 ns",
    "RAAIMT/RAAMMT/RAADEC": "0 = refresh management not modeled (vendor DEVICE_ID values)",
}

if __name__ == "__main__":
    out = sys.argv[1]
    for rate in LP5_BINS:
        spec, t = lpddr5(rate)
        json.dump(spec, open(f"{out}/{memspec_name(rate)}.json", "w"), indent=4)
        print(memspec_name(rate), {k: v for k, v in t.items() if k != "tCK"})
    spec, t = hbm4()
    json.dump(spec, open(f"{out}/JEDEC_HBM4_32Gb_8H_8000.json", "w"), indent=4)
    print("HBM4", t)
