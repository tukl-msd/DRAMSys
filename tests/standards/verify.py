#!/usr/bin/env python3
"""DRAMSys LPDDR5 / HBM4 verification runs.

Identical DRAM-coordinate access sequences go to DRAMSys (StlPlayer, own address mapping) and, if
RAMULATOR_PYTHON is set, to Ramulator2 (LoadStoreTrace + RoBaRaCoCh, addresses encoded for that mapper;
the mapping is checked against Ramulator's own command trace). Reports whole-run and steady-state
bandwidth (middle 80 % of column commands, same method for both simulators), read latency, a DRAMSys
latency breakdown, the independent timing check (tcheck.py) and planted-violation runs.

usage: verify.py <outdir> [job-name-regex]   (writes <outdir>/results.json and prints a table)
environment: DRAMSYS_BIN       DRAMSys executable (default: build/bin/DRAMSys in this repository)
             RAMULATOR_PYTHON  Python interpreter with the ramulator package (Ramulator runs are skipped if unset)
"""
import csv
import json
import os
import random
import re
import sqlite3
import statistics
import subprocess
import sys
from collections import Counter
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.dirname(os.path.dirname(HERE))
DRAMSYS = os.environ.get("DRAMSYS_BIN", f"{SRC}/build/bin/DRAMSys")
RAMPY = os.environ.get("RAMULATOR_PYTHON")
CFG = f"{SRC}/configs"
BYTES = 32                      # one BL16 x16 (LPDDR5) or BL8 x32 (HBM4 pseudo channel) burst

STD = {
    "lp6400": dict(memspec=f"{CFG}/memspec/JEDEC_16Gb_LPDDR5-6400_16bit_BG.json",
                   am=f"{CFG}/addressmapping/am_lpddr5_16Gbx16_bg_robacobgc.json",
                   ram=("LPDDR5", "LPDDR5_16Gb_x16", "LPDDR5_6400", "LPDDR5"), ram_tick_ns=1.25,
                   n=32768, clk=800),
    "lp7500": dict(memspec=f"{CFG}/memspec/JEDEC_16Gb_LPDDR5X-7500_16bit_BG.json",
                   am=f"{CFG}/addressmapping/am_lpddr5_16Gbx16_bg_robacobgc.json",
                   ram=("LPDDR5", "LPDDR5_16Gb_x16", "LPDDR5X_7500", "LPDDR5"), ram_tick_ns=1.067,
                   n=32768, clk=937),
    "lp8533": dict(memspec=f"{CFG}/memspec/JEDEC_16Gb_LPDDR5X-8533_16bit_BG.json",
                   am=f"{CFG}/addressmapping/am_lpddr5_16Gbx16_bg_robacobgc.json",
                   ram=("LPDDR5", "LPDDR5_16Gb_x16", "LPDDR5X_8533", "LPDDR5"), ram_tick_ns=0.938,
                   n=32768, clk=1066),
    "hbm4": dict(memspec=f"{CFG}/memspec/JEDEC_HBM4_32Gb_8H_8000.json",
                 am=f"{CFG}/addressmapping/am_hbm4_32Gb-8H_pc_robacosibgpc.json",
                 ram=("HBM4", "HBM4_32Gb_8Hi", "HBM4_8000Mbps", "HBM34"), ram_tick_ns=0.25,
                 n=65536, clk=2000),
}
# Ramulator tick = preset tCK / tick_multiplier (HBM4: 500 ps / 2)


# ------------------------------------------------------------------ DRAM coordinates and encodings
def lp_coords(pattern, n, rng):
    """(bg, ba, row, col), col = BL16 burst index 0..63"""
    if pattern in ("sr", "sw"):
        return [(i & 3, (i >> 8) & 3, i >> 10, (i >> 2) & 63) for i in range(n)]
    return [(rng.randrange(4), rng.randrange(4), rng.randrange(65536), rng.randrange(64)) for _ in range(n)]


def hb_coords(pattern, n, rng):
    """(pc, sid, bg, ba, row, col), col = BL8 burst index 0..31"""
    if pattern in ("sr", "sw"):
        return [(i & 1, (i >> 2) & 1, (i >> 1) & 1, (i >> 3) & 7, i >> 11, (i >> 6) & 31) for i in range(n)]
    return [(rng.randrange(2), rng.randrange(2), rng.randrange(2), rng.randrange(8), rng.randrange(16384),
             rng.randrange(32)) for _ in range(n)]


def lp_dramsys_addr(c):
    bg, ba, row, col = c
    return (col << 7) | (bg << 5) | (ba << 13) | (row << 15)


def hb_dramsys_addr(c):
    pc, sid, bg, ba, row, col = c
    return (pc << 5) | (bg << 6) | (sid << 7) | (ba << 8) | (col << 11) | (row << 16)


# Ramulator RoBaRaCoCh: addr >> log2(tx bytes); column bits first, then the levels below the channel
# in hierarchy order up to Row (LPDDR5: Rank, BankGroup, Bank, Row; HBM4: PseudoChannel, Sid,
# BankGroup, Bank, Row).
def lp_ram_addr(c):
    bg, ba, row, col = c
    return ((((row << 2 | ba) << 2 | bg) << 6) | col) << 5


def hb_ram_addr(c):
    pc, sid, bg, ba, row, col = c
    return ((((((row << 3 | ba) << 1 | bg) << 1 | sid) << 1 | pc) << 5) | col) << 5


def write_traces(std, pattern, d, gap=0, n=None, split_pc=False):
    rng = random.Random(20261107)
    lp = std.startswith("lp")
    n = n or STD[std]["n"]
    coords = (lp_coords if lp else hb_coords)(pattern, n, rng)
    ds_addr = lp_dramsys_addr if lp else hb_dramsys_addr
    ram_addr = lp_ram_addr if lp else hb_ram_addr
    ops = []
    for _ in coords:
        if pattern in ("sr", "rr", "ul"):
            ops.append("R")
        elif pattern == "sw":
            ops.append("W")
        else:
            ops.append("R" if rng.random() < 2 / 3 else "W")
    streams = {"trace.stl": list(zip(ops, coords))}
    if split_pc:
        streams = {f"trace_pc{p}.stl": [(o, c) for o, c in zip(ops, coords) if c[0] == p] for p in (0, 1)}
    for name, seq in streams.items():
        with open(f"{d}/{name}", "w") as f:
            for k, (o, c) in enumerate(seq):
                f.write(f"{k * gap}:\t{'read' if o == 'R' else 'write'}\t0x{ds_addr(c):x}\n")
    with open(f"{d}/trace.ls", "w") as f:
        for o, c in zip(ops, coords):
            f.write(f"{'LD' if o == 'R' else 'ST'} 0x{ram_addr(c):x}\n")
    json.dump([list(c) for c in coords], open(f"{d}/coords.json", "w"))
    return list(streams)


def std_is_lp(d):
    return json.load(open(f"{d}/memspec.json"))["memspec"]["memoryType"] == "LPDDR5"


def steady_bw(times_ns, frac=0.1):
    """GB/s over the middle (1-2*frac) of the column commands, each moving BYTES"""
    t = sorted(times_ns)
    if len(t) < 100:
        return None
    a, b = int(frac * len(t)), int((1 - frac) * len(t)) - 1
    return (b - a) * BYTES / (t[b] - t[a])


# ------------------------------------------------------------------ DRAMSys
def dramsys_cfg(std, d, memspec_obj, refresh, power, resp, streams, mc, arbiter):
    s = STD[std]
    sim = {
        "simulation": {
            "addressmapping": s["am"],
            "mcconfig": {"PagePolicy": "Open", "Scheduler": "FrFcfs", "SchedulerBuffer": "Shared",
                         "RequestBufferSize": 32, "CmdMux": "Oldest", "RespQueue": resp,
                         "RefreshPolicy": refresh, "RefreshMaxPostponed": 0, "RefreshMaxPulledin": 0,
                         "PowerDownPolicy": "NoPowerDown", "Arbiter": arbiter,
                         "MaxActiveTransactions": 128, "RefreshManagement": False, **(mc or {})},
            "memspec": memspec_obj["memspec"],
            "simconfig": {"AddressOffset": 0, "DatabaseRecording": True, "Debug": False,
                          "EnableWindowing": False, "PowerAnalysis": power,
                          "SimulationName": "run", "SimulationProgressBar": False,
                          "StoreMode": "NoStorage", "WindowSize": 1000,
                          "TogglingRate": {"togglingRateRead": 0.5, "togglingRateWrite": 0.5,
                                           "dutyCycleRead": 0.5, "dutyCycleWrite": 0.5,
                                           "idlePatternRead": "L", "idlePatternWrite": "L"}},
            "simulationid": "x",
            "tracesetup": [{"type": "player", "clkMhz": s["clk"], "dataLength": BYTES, "name": f"{d}/{st}"}
                           for st in streams],
        }
    }
    json.dump(sim, open(f"{d}/sim.json", "w"), indent=1)
    json.dump(memspec_obj, open(f"{d}/memspec.json", "w"), indent=1)


def run_dramsys(d):
    p = subprocess.run([DRAMSYS, f"{d}/sim.json"], cwd=d, capture_output=True, text=True, timeout=3600)
    open(f"{d}/dramsys.out", "w").write(p.stdout + p.stderr)
    out = p.stdout
    r = {"rc": p.returncode}
    for key, pat in (("bw", r"controller0::AverageBandwidth:\s+([\d.]+)"),
                     ("bw_max", r"controller0::MaximumTheoreticalBandwidth:\s+([\d.]+)"),
                     ("util", r"controller0::AverageUtilization:\s+([\d.]+)"),
                     ("sim_time_s", r"Simulator::SimulationTime:\s+([\d.e+-]+)"),
                     ("energy_J", r"drampoweradapter0\s+Total Energy:\s+([\d.e+-]+)"),
                     ("core_J", r"drampoweradapter0\s+Core Energy:\s+([\d.e+-]+)"),
                     ("iface_J", r"drampoweradapter0\s+Interface Energy:\s+([\d.e+-]+)"),
                     ("power_W", r"drampoweradapter0\s+Average Power:\s+([\d.e+-]+)")):
        m = re.search(pat, out)
        if m:
            r[key] = float(m.group(1))
    tdb = [f for f in os.listdir(d) if f.endswith(".tdb")]
    if not tdb:
        return r
    db = sqlite3.connect(f"{d}/{tdb[0]}")
    rows = db.execute(
        "select q.PhaseBegin, q.PhaseEnd, c.PhaseBegin, c.DataStrobeEnd, s.PhaseBegin "
        "from Phases q join Phases c on c.Transact = q.Transact join Phases s on s.Transact = q.Transact "
        "join Transactions t on t.ID = q.Transact where q.PhaseName = 'REQ' and s.PhaseName = 'RESP' "
        "and c.PhaseName in ('RD', 'RDA') and t.Command = 'R'").fetchall()
    if rows:
        r["rd_lat_ns"] = statistics.mean(s - qe for _, qe, _, _, s in rows) / 1000
        r["rd_lat_parts_ns"] = {
            "queue_to_cmd": statistics.mean(c - qe for _, qe, c, _, _ in rows) / 1000,
            "cmd_to_data_end": statistics.mean(de - c for _, _, c, de, _ in rows) / 1000,
            "data_end_to_resp": statistics.mean(s - de for _, _, _, de, s in rows) / 1000,
        }
    col = db.execute("select PhaseBegin, Rank, BurstLength from Phases "
                     "where PhaseName in ('RD','RDA','WR','WRA')").fetchall()
    bl0 = min(bl for _, _, bl in col) if col else 0
    r["steady_bw"] = steady_bw([t / 1000 for t, _, _ in col]) if col and all(bl == bl0 for _, _, bl in col) else None
    if r["steady_bw"] and bl0 == 32 and std_is_lp(d):
        r["steady_bw"] *= 2        # BL32 on x16 moves 64 B per column command
    # refresh windows: bandwidth of the other pseudo channel while one pseudo channel is in REFab
    ref = db.execute("select PhaseBegin, PhaseEnd, Rank from Phases where PhaseName = 'REFAB'").fetchall()
    col = [(t, rk) for t, rk, _ in col]
    ranks = sorted({rk for _, rk in col})
    if ref and len(ranks) > 1:
        busy = sum(e - b for b, e, _ in ref) / 1000
        other = sum(1 for t, rk in col for b, e, rr in ref if rr != rk and b <= t < e)
        r["other_pc_bw_during_refab"] = other * BYTES / busy
        r["refab_windows_ns"] = busy
    cnt = dict(db.execute("select PhaseName, count(*) from Phases group by PhaseName").fetchall())
    r["cmds"] = {k: v for k, v in cnt.items() if k not in ("REQ", "RESP")}
    r["tdb"] = f"{d}/{tdb[0]}"
    return r


def run_tcheck(memspec_true, tdb):
    p = subprocess.run([sys.executable, "-I", f"{HERE}/tcheck.py", memspec_true, tdb],
                       capture_output=True, text=True, timeout=3600)
    m = re.search(r"violations=(\d+)", p.stdout)
    return {"rc": p.returncode, "violations": int(m.group(1)) if m else None, "out": p.stdout.strip()}


# ------------------------------------------------------------------ Ramulator
RAM_SCRIPT = r'''
import json, sys, ramulator
std, org, timing, ctrl_name, trace, refresh, cmdpath = sys.argv[1:8]
dram = getattr(ramulator.dram, std)(org_preset=org, timing_preset=timing)
if refresh == "none":
    ref = ramulator.refresh_manager.NoRefresh()
elif refresh == "allbank":
    ref = ramulator.refresh_manager.AllBank()
else:
    ref = ramulator.refresh_manager.AllBank(scatter_interval=int(refresh.split(":")[1]))
ctrl = getattr(ramulator.controller, ctrl_name)(
    dram=dram, scheduler=ramulator.scheduler.FRFCFS(), refresh_manager=ref,
    row_policy=ramulator.row_policy.Open(), addr_mapper=ramulator.addr_mapper.RoBaRaCoCh(),
    controller_plugins=[ramulator.controller_plugin.CmdTraceRecorder(path=cmdpath)])
mem = ramulator.memory_system.GenericDRAM(clock_ratio=1, controllers=[ctrl],
                                          channel_mapper=ramulator.channel_mapper.CacheLineInterleave())
sim = ramulator.Simulation(ramulator.frontend.LoadStoreTrace(clock_ratio=1, path=trace), mem)
sim.run()
c = sim.stats["memory_system"]["controller"]
print("RAMJSON" + json.dumps({k: c[k] for k in c if isinstance(c[k], (int, float))}))
'''


def run_ramulator(std, d, refresh):
    s = STD[std]
    open(f"{d}/ram_run.py", "w").write(RAM_SCRIPT)
    p = subprocess.run([RAMPY, f"{d}/ram_run.py", *s["ram"], f"{d}/trace.ls", refresh, f"{d}/ramcmd"],
                       cwd=d, capture_output=True, text=True, timeout=3600)
    open(f"{d}/ramulator.out", "w").write(p.stdout + p.stderr)
    m = re.search(r"RAMJSON(.*)", p.stdout)
    if not m:
        return {"rc": p.returncode, "error": (p.stdout + p.stderr)[-800:]}
    c = json.loads(m.group(1))
    tick = s["ram_tick_ns"]
    served = c.get("num_read_reqs_served", 0) + c.get("num_write_reqs_served", 0)
    r = {"rc": p.returncode, "bw": served * BYTES / (c["cycles"] * tick),
         "rd_lat_ns": c.get("avg_read_latency", 0) * tick if c.get("num_read_reqs_served") else None,
         "served": served, "coalesced": c.get("num_write_reqs_coalesced", 0),
         "forwarded": c.get("num_read_reqs_forwarded", 0), "raw": c}
    # command trace: steady-state bandwidth + address-mapping check
    coords = {tuple(x) for x in json.load(open(f"{d}/coords.json"))}
    lp = std.startswith("lp")
    names = ("BankGroup", "Bank", "Row", "Column") if lp else \
        ("PseudoChannel", "Sid", "BankGroup", "Bank", "Row", "Column")
    times, bad, cmds = [], 0, Counter()
    with open(f"{d}/ramcmd.ch0") as f:
        for row in csv.DictReader(f):
            cmds[row["command"]] += 1
            if row["command"] in ("RD", "WR", "RDA", "WRA"):
                times.append(int(row["clock"]) * tick)
                if tuple(int(row[k]) for k in names) not in coords:
                    bad += 1
    r["steady_bw"] = steady_bw(times)
    r["map_mismatch"] = bad
    r["cmds"] = dict(cmds)
    os.remove(f"{d}/ramcmd.ch0")
    return r


# ------------------------------------------------------------------ runs
def job(out, name, std, pattern, refresh="NoRefresh", ram=None, mutate=None, gap=0, n=None, power=None,
        resp="Fifo", split_pc=False, mc=None, arbiter="Simple"):
    d = f"{out}/{name}"
    os.makedirs(d, exist_ok=True)
    for f in os.listdir(d):
        if f.endswith(".tdb"):
            os.remove(f"{d}/{f}")
    streams = write_traces(std, pattern, d, gap=gap, n=n, split_pc=split_pc)
    true_ms = STD[std]["memspec"]
    ms = json.load(open(true_ms))
    for k, v in (mutate or {}).items():
        ms["memspec"]["memtimingspec"][k] = v
    if power is None:
        power = False       # the memspecs carry no current or I/O data
    dramsys_cfg(std, d, ms, refresh, power, resp, streams, mc, arbiter)
    r = {"name": name, "std": std, "pattern": pattern, "refresh": refresh, "mutate": mutate, "resp": resp,
         "split_pc": split_pc, "mc": mc, "arbiter": arbiter}
    r["dramsys"] = run_dramsys(d)
    if "tdb" in r["dramsys"]:
        r["tcheck"] = run_tcheck(true_ms, r["dramsys"]["tdb"])
        os.remove(r["dramsys"].pop("tdb"))      # tdb files are large; keep results only
    if ram and RAMPY:
        r["ramulator"] = run_ramulator(std, d, ram)
    return r


# LPDDR5 16B and 8B bank modes and BL32: code-path runs only. The BG-mode 6400 memspec is reused with
# BL/n values derived from the burst length
# (BL32 at WCK:CK 4:1 = 4 nCK on the bus); DRAMSys and tcheck read the same test memspec.
CODEPATH = {
    "16B": dict(banks=16, groups=1, offset=8,
                am={"BYTE_BIT": [0], "COLUMN_BIT": list(range(1, 11)), "BANK_BIT": list(range(11, 15)),
                    "ROW_BIT": list(range(15, 31))}),
    "8B": dict(banks=8, groups=1, offset=4,
               am={"BYTE_BIT": [0], "COLUMN_BIT": list(range(1, 11)), "BANK_BIT": list(range(11, 14)),
                   "ROW_BIT": list(range(14, 30))}),
}


def codepath_job(out, name, mode, bl, refresh, n=16384):
    d = f"{out}/{name}"
    os.makedirs(d, exist_ok=True)
    for f in os.listdir(d):
        if f.endswith(".tdb"):
            os.remove(f"{d}/{f}")
    cp = CODEPATH[mode]
    ms = json.load(open(STD["lp6400"]["memspec"]))
    a, t = ms["memspec"]["memarchitecturespec"], ms["memspec"]["memtimingspec"]
    a.update(nbrOfBanks=cp["banks"], nbrOfBankGroups=cp["groups"], per2BankOffset=cp["offset"],
             burstLength=16 if mode == "16B" else 32, maxBurstLength=32)
    t.update(BL_n_min_16=2, BL_n_max_16=2, BL_n_L_16=2, BL_n_S_16=2, BL_n_min_32=4, BL_n_max_32=4,
             BL_n_L_32=4, BL_n_S_32=4)
    t["RTW_L_16"] = t["RTW_S_16"] = t["RL"] + 2 + 2 - t["WL"]
    t["RTW_L_32"] = t["RTW_S_32"] = t["RL"] + 4 + 2 - t["WL"]
    ms["memspec"]["memoryId"] = f"code-path test: LPDDR5 {mode} mode, BL{bl} (not a sourced device)"
    ms["memspec"].pop("sources", None)
    json.dump(ms, open(f"{d}/memspec.json", "w"), indent=1)
    size = bl * 2                                    # x16: 2 bytes per beat
    span = cp["banks"] * 65536 * 1024 * 2
    rng = random.Random(20261108)
    with open(f"{d}/trace.stl", "w") as f:
        for _ in range(n):
            op = "read" if rng.random() < 2 / 3 else "write"
            f.write(f"0:\t{op}\t0x{rng.randrange(span // size) * size:x}\n")
    sim = {"simulation": {
        "addressmapping": cp["am"],
        "mcconfig": {"PagePolicy": "Open", "Scheduler": "FrFcfs", "SchedulerBuffer": "Shared",
                     "RequestBufferSize": 32, "CmdMux": "Oldest", "RespQueue": "Fifo", "RefreshPolicy": refresh,
                     "RefreshMaxPostponed": 0, "RefreshMaxPulledin": 0, "PowerDownPolicy": "NoPowerDown",
                     "Arbiter": "Simple", "MaxActiveTransactions": 128, "RefreshManagement": False},
        "memspec": ms["memspec"],
        "simconfig": {"AddressOffset": 0, "DatabaseRecording": True, "Debug": False, "EnableWindowing": False,
                      "PowerAnalysis": False, "SimulationName": "run", "SimulationProgressBar": False,
                      "StoreMode": "NoStorage", "WindowSize": 1000,
                      "TogglingRate": {"togglingRateRead": 0.5, "togglingRateWrite": 0.5, "dutyCycleRead": 0.5,
                                       "dutyCycleWrite": 0.5, "idlePatternRead": "L", "idlePatternWrite": "L"}},
        "simulationid": "x",
        "tracesetup": [{"type": "player", "clkMhz": 800, "dataLength": size, "name": f"{d}/trace.stl"}]}}
    json.dump(sim, open(f"{d}/sim.json", "w"), indent=1)
    r = {"name": name, "std": f"lp-{mode}", "pattern": f"rm-bl{bl}", "refresh": refresh}
    r["dramsys"] = run_dramsys(d)
    if "tdb" in r["dramsys"]:
        r["tcheck"] = run_tcheck(f"{d}/memspec.json", r["dramsys"]["tdb"])
        os.remove(r["dramsys"].pop("tdb"))
    return r


def jobs():
    js = []
    for std in ("lp6400", "lp7500", "lp8533", "hbm4"):
        for pat in ("sr", "sw", "rr", "rm"):
            js.append(dict(name=f"{std}_{pat}_noref", std=std, pattern=pat, ram="none"))
            js.append(dict(name=f"{std}_{pat}_ref", std=std, pattern=pat, refresh="AllBank", ram="allbank"))
    # (b) latency: DRAMSys response queue in request order (Reorder) vs completion order (Fifo)
    js.append(dict(name="lp6400_rr_noref_reorder", std="lp6400", pattern="rr", resp="Reorder"))
    js.append(dict(name="hbm4_rr_noref_reorder", std="hbm4", pattern="rr", resp="Reorder"))
    # unloaded latency: one random read every 1000 initiator clocks
    for std in ("lp6400", "lp8533", "hbm4"):
        js.append(dict(name=f"{std}_ul", std=std, pattern="ul", gap=1000, n=2000))
    # HBM4 refresh: DRAMSys staggers REFab of the two pseudo channels by tREFI/2, Ramulator refreshes both at once
    js.append(dict(name="hbm4_sr_ref_ramscatter", std="hbm4", pattern="sr", refresh="AllBank",
                   ram="scatter:7800"))
    # one in-order stream: per-bank request buffers let the other pseudo channel keep running during REFab
    for depth in (8, 16, 32):
        js.append(dict(name=f"hbm4_sr_ref_bankwise{depth}", std="hbm4", pattern="sr", refresh="AllBank",
                       mc={"SchedulerBuffer": "Bankwise", "RequestBufferSize": depth}))
    bw16 = {"SchedulerBuffer": "Bankwise", "RequestBufferSize": 16}
    for pat in ("sr", "sw", "rr"):
        js.append(dict(name=f"hbm4_{pat}_noref_bankwise16", std="hbm4", pattern=pat, mc=bw16))
    js.append(dict(name="hbm4_rr_ref_bankwise16", std="hbm4", pattern="rr", refresh="AllBank", mc=bw16))
    for pat in ("sr", "sw"):
        js.append(dict(name=f"lp6400_{pat}_noref_bankwise16", std="lp6400", pattern=pat, mc=bw16))
    js.append(dict(name="hbm4_sr_refpb", std="hbm4", pattern="sr", refresh="PerBank"))
    # two initiators on one channel: DRAMSys arbiters hand over at most one request per two controller cycles
    js.append(dict(name="hbm4_sr_noref_2streams", std="hbm4", pattern="sr", split_pc=True, arbiter="Fifo"))
    # mixed traffic with Ramulator's write-drain policy (separate 32/32 read/write buffers, watermarks 0.8/0.2)
    wm = {"Scheduler": "GrpFrFcfsWm", "SchedulerBuffer": "ReadWrite", "RequestBufferSizeRead": 32,
          "RequestBufferSizeWrite": 32, "HighWatermark": 25, "LowWatermark": 6}
    for std in ("lp6400", "lp8533", "hbm4"):
        js.append(dict(name=f"{std}_rm_noref_wm", std=std, pattern="rm", mc=wm))
        js.append(dict(name=f"{std}_sw_noref_wm", std=std, pattern="sw", mc=wm))
    js.append(dict(name="hbm4_rm_refpb", std="hbm4", pattern="rm", refresh="PerBank"))
    for std in ("lp6400", "lp8533"):
        js.append(dict(name=f"{std}_rm_refp2b", std=std, pattern="rm", refresh="Per2Bank"))
        js.append(dict(name=f"{std}_sr_refp2b", std=std, pattern="sr", refresh="Per2Bank"))
    # (d) planted wrong timings: DRAMSys runs with a too-small value, the checker uses the true memspec
    js.append(dict(name="plant_lp6400_RCD", std="lp6400", pattern="rr", refresh="AllBank",
                   mutate={"RCD_S": 12, "RCD_L": 12}))
    # at 6400 tFAW (16 nCK) = 4 x tRRD, so a too-small tFAW only shows together with a too-small tRRD
    js.append(dict(name="plant_lp6400_RRD", std="lp6400", pattern="rr", mutate={"RRD": 2}))
    js.append(dict(name="plant_lp6400_RRD_FAW", std="lp6400", pattern="rr", mutate={"RRD": 2, "FAW": 8}))
    js.append(dict(name="plant_lp6400_RFCpb", std="lp6400", pattern="rm", refresh="Per2Bank",
                   mutate={"RFCpb": 60}))
    js.append(dict(name="plant_lp6400_RTW_L", std="lp6400", pattern="rm", mutate={"RTW_L_16": 12}))
    js.append(dict(name="plant_lp8533_RCD_RD", std="lp8533", pattern="rr", refresh="AllBank", mutate={"RCD_L": 17}))
    js.append(dict(name="plant_lp8533_RCD_WR", std="lp8533", pattern="rm", mutate={"RCD_S": 6}))
    js.append(dict(name="plant_lp8533_RRD_FAW", std="lp8533", pattern="rr", mutate={"RRD": 2, "FAW": 8}))
    js.append(dict(name="plant_lp8533_RRD", std="lp8533", pattern="rr", mutate={"RRD": 2}))
    js.append(dict(name="plant_lp8533_RFCpb", std="lp8533", pattern="rm", refresh="Per2Bank", mutate={"RFCpb": 60}))
    js.append(dict(name="plant_lp8533_RTW_L", std="lp8533", pattern="rm", mutate={"RTW_L_16": 15}))
    js.append(dict(name="plant_hbm4_CCDL", std="hbm4", pattern="sr", mutate={"CCDL": 4}))
    js.append(dict(name="plant_hbm4_RFC", std="hbm4", pattern="rm", refresh="AllBank", mutate={"RFC": 800}))
    js.append(dict(name="plant_hbm4_RREFD", std="hbm4", pattern="rm", refresh="PerBank", mutate={"RREFD": 8}))
    return js


def fmt(v, w=8, p=2):
    return f"{v:{w}.{p}f}" if isinstance(v, (int, float)) else f"{'-':>{w}s}"


def main():
    out = os.path.abspath(sys.argv[1])
    sel = re.compile(sys.argv[2]) if len(sys.argv) > 2 else None
    os.makedirs(out, exist_ok=True)
    js = [j for j in jobs() if not sel or sel.search(j["name"])]
    cps = [dict(name=f"codepath_lp{m}_bl{bl}_{ref.lower()}", mode=m, bl=bl, refresh=ref)
           for m, bls, pb in (("16B", (16, 32), "Per2Bank"), ("8B", (32,), "PerBank"))
           for bl in bls for ref in ("AllBank", pb)]
    cps = [c for c in cps if not sel or sel.search(c["name"])]
    with ThreadPoolExecutor(max_workers=os.cpu_count()) as pool:
        results = list(pool.map(lambda j: job(out, **j), js)) + list(pool.map(lambda c: codepath_job(out, **c), cps))
    json.dump(results, open(f"{out}/results.json", "w"), indent=1)
    print(f"{'run':32s} {'DS GB/s':>8s} {'DSss':>8s} {'DS lat':>7s} {'RAM GB/s':>8s} {'RAMss':>8s} "
          f"{'RAM lat':>7s} {'viol':>6s} {'map':>4s}")
    for r in results:
        ds, ra, tc = r["dramsys"], r.get("ramulator", {}), r.get("tcheck", {})
        print(f"{r['name']:32s} {fmt(ds.get('bw'))} {fmt(ds.get('steady_bw'))} {fmt(ds.get('rd_lat_ns'), 7, 1)} "
              f"{fmt(ra.get('bw'))} {fmt(ra.get('steady_bw'))} {fmt(ra.get('rd_lat_ns'), 7, 1)} "
              f"{str(tc.get('violations')):>6s} {str(ra.get('map_mismatch', '')):>4s}"
              + (f"  E={ds['energy_J']:.4g} J P={ds.get('power_W', 0):.4g} W" if 'energy_J' in ds else "")
              + ("" if ds.get("rc") == 0 else f"  DS rc={ds.get('rc')}")
              + ("" if not ra or ra.get("rc") == 0 else f"  RAM rc={ra.get('rc')} {ra.get('error', '')[-300:]}"))


if __name__ == "__main__":
    main()
