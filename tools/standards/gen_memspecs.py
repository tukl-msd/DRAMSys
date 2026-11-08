#!/usr/bin/env python3
"""Write the LPDDR5 and HBM4 memspec JSONs for DRAMSys. Every number carries its source tag.

LPDDR5 sources (all public):
  [CB]  coreboot util/spd_tools/src/spd_gen/lp5.go (61dc9a49): JESD209-5B Table 220 MaxCASLatency (CKR 4:1,
        RL Set 0), Table 372 tRCD/tRPab/tRPpb, Tables 235/236 tRFCab/tRFCpb, Tables 6-11 row address bits
  [R5C] CMU-SAFARI/ramulator2 main (7bead6c) python/ramulator/dram/lpddr5.py, LPDDR5_6400 preset and secondary
        timings with their JESD209-5C citations
  [G5]  gem5 src/python/gem5/components/memory/dram_interfaces/lpddr5.py (LPDDR5_6400_1x16_BG_BL32)
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


def lpddr5_6400():
    tck = 1.25                              # ns; CK = 6400 MT/s / 8 (WCK:CK 4:1, DDR on WCK)
    rl, wl = 17, 9                          # [CB] MaxCASLatency(6400), [R5C] nCL 17 / nCWL 9
    t = {
        "tCK": tck * 1e-9,
        "RCD_S": ru(18, tck), "RCD_L": ru(18, tck),
        "RPpb": ru(18, tck), "RPab": ru(21, tck), "RAS": ru(42, tck),
        "WR": ru(34, tck),
        "RBTP": ru(7.5 - 2 * tck, tck),     # [G5] tRTP = 7.5 ns - 2 tCK; [R5C] nRBTP 4
        "RL": rl, "WL": wl, "PPD": 2,
        "FAW": ru(20, tck), "RRD": max(2, ru(5, tck)),
        "WTR_L": ru(12, tck), "WTR_S": ru(6.25, tck),
        "BL_n_min_16": 2, "BL_n_max_16": 4, "BL_n_L_16": 4, "BL_n_S_16": 2,
        "BL_n_min_32": 0, "BL_n_max_32": 0, "BL_n_L_32": 0, "BL_n_S_32": 0,
        "RTW_L_32": 0, "RTW_S_32": 0,
        "RTRS": 0, "WCK2CK": 4, "WCK2DQO": 2,
        "REFI": int(3906 / tck), "REFIpb": int(488 / tck),
        "RFCab": ru(280, tck), "RFCpb": ru(140, tck),
        "pbR2pbR": ru(90, tck), "pbR2act": ru(7.5, tck),
    }
    t["RCpb"] = t["RAS"] + t["RPpb"]
    t["RCab"] = t["RAS"] + t["RPab"]
    t["RTW_L_16"] = rl + t["BL_n_max_16"] + 2 - wl
    t["RTW_S_16"] = rl + t["BL_n_min_16"] + 2 - wl
    # the same values as Ramulator2's LPDDR5_6400 preset (nCK)
    ram = dict(nCL=17, nRCD=15, nRP=15, nRPab=17, nRAS=34, nRC=49, nWR=28, nRTP=6, nCWL=9, nWTRS=5, nWTRL=10)
    ours = dict(nCL=t["RL"], nRCD=t["RCD_S"], nRP=t["RPpb"], nRPab=t["RPab"], nRAS=t["RAS"], nRC=t["RCpb"],
                nWR=t["WR"], nRTP=t["BL_n_min_16"] + t["RBTP"], nCWL=t["WL"], nWTRS=t["WTR_S"], nWTRL=t["WTR_L"])
    assert ours == ram, (ours, ram)
    order = ["tCK", "REFI", "REFIpb", "RFCab", "RFCpb", "RAS", "RPab", "RPpb", "RCpb", "RCab", "PPD",
             "RCD_L", "RCD_S", "FAW", "RRD", "RL", "RBTP", "WL", "WR", "RTRS",
             "BL_n_min_16", "BL_n_max_16", "BL_n_L_16", "BL_n_S_16",
             "BL_n_min_32", "BL_n_max_32", "BL_n_L_32", "BL_n_S_32",
             "WTR_L", "WTR_S", "RTW_L_32", "RTW_L_16", "RTW_S_32", "RTW_S_16",
             "WCK2DQO", "WCK2CK", "pbR2act", "pbR2pbR"]
    # No public LPDDR5 currents, supply voltages or I/O impedances: zero placeholders, so DRAMPower reports 0 J
    # for this memspec until device values are filled in.
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
            "memoryId": "JEDEC_16Gb_LPDDR5-6400_16bit_BG",
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
            "sources": LPDDR5_SOURCES,
        }
    }
    return spec, t


LPDDR5_SOURCES = {
    "architecture": "[R5C] LPDDR5_16Gb_x16 (JESD209-5C Table 6): 16Gb x16 BG mode, 4 BG x 4 banks, 65,536 rows, 1024 columns (64 x BL16); [CB] 16 row address bits for 16Gb x16; per2BankOffset 8: REFpb refreshes banks n and n+8 [R5C] (JESD209-5C 7.5.1, Table 235)",
    "tCK": "data rate / 8 (WCK:CK 4:1, DDR WCK): 1.25 ns [R5C][G5]",
    "RL/WL": "RL 17 (Set 0): [CB] MaxCASLatency(6400), [R5C] nCL; WL 9 (Set A): [R5C] nCWL",
    "RCD/RPpb/RPab/RAS": "18/18/21/42 ns [CB][R5C][G5]; RCD_L = RCD_S = tRCD (single tRCD)",
    "RCpb/RCab": "RAS + RPpb / RAS + RPab ([R5C] nRC = nRAS + nRP)",
    "RBTP": "RU((7.5 ns - 2 tCK) / tCK) = 4 [G5] tRTP; [R5C] nRTP = BL/n_min (2) + nRBTP (4)",
    "WR": "34 ns [R5C][G5]",
    "WTR_L/WTR_S": "12 / 6.25 ns [R5C][G5]",
    "RRD": "5 ns [R5C] (BG mode tRRD_S = tRRD_L), [G5] (single tRRD in the DRAMUtils LPDDR5 schema)",
    "FAW": "20 ns [R5C] (JESD209-5C Tables 381-383), [G5]",
    "PPD": "2 nCK [R5C], [G5] tPPD = 2 tCK",
    "BL_n_16": "BL16 at 4:1, WCK > 1600 MHz: BL/n_min 2, BL/n_max 4; tCCD_S = 2, tCCD_L = 4 [R5C] (JESD209-5C Table 339)",
    "BL_n_32/RTW_32": "0 = unused: BL32 in BG mode is rejected by MemSpecLPDDR5",
    "RTW_16": "same BG: RL + BL/n_max + 2 - WL, different BG: RL + BL/n_min + 2 - WL; 2 = RU(tWCK2DQO_HF(max)/tCK) [R5C] (JESD209-5C Tables 347/348, 350/351)",
    "RTRS": "0 = unused, single rank",
    "WCK2DQO": "2 nCK, the RU(tWCK2DQO_HF(max)/tCK) term of [R5C]; read data starts at RL + WCK2DQO",
    "ACT-1/ACT-2": "ACT-1 takes one CA cycle, ACT-2 any free CA cycle within tAAD = 8 nCK ([R5C] nAAD, JESD209-5C 7.3.1; [G5] tAAD); timings that start at an ACTIVATE run from ACT-2 [R5C]; tAAD is fixed in MemSpecLPDDR5 (not in the DRAMUtils schema)",
    "REFI/REFIpb": "3906 / 488 ns [R5C] (JESD209-5C Table 240), rounded down",
    "RFCab/RFCpb": "280 / 140 ns for 16Gb [CB] (JESD209-5B Tables 235/236), [R5C] (JESD209-5C Table 240)",
    "pbR2pbR/pbR2act": "90 / 7.5 ns [R5C] (JESD209-5C Tables 240/241)",
    "mempowerspec/memimpedancespec": "no public source for LPDDR5 currents, supply voltages or I/O impedances: all zero, so DRAMPower reports 0 J; fill in device values for energy results",
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
    spec, t = lpddr5_6400()
    json.dump(spec, open(f"{out}/JEDEC_16Gb_LPDDR5-6400_16bit_BG.json", "w"), indent=4)
    print("LPDDR5-6400", {k: v for k, v in t.items() if k != "tCK"})
    spec, t = hbm4()
    json.dump(spec, open(f"{out}/JEDEC_HBM4_32Gb_8H_8000.json", "w"), indent=4)
    print("HBM4", t)
