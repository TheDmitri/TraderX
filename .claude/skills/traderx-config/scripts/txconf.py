#!/usr/bin/env python3
"""TraderX config helper.

  txconf.py tq --buy FULL --sell FULL [--buy-qty N] [--sell-qty N]   encode tradeQuantity
  txconf.py tq-decode <int>                                          decode tradeQuantity
  txconf.py ss --behavior 0|1|2 [--destock PCT]                      encode stockSettings
  txconf.py ss-decode <int>                                          decode stockSettings
  txconf.py newid prod|cat <className or name> [--dir Products/]     next free id
  txconf.py lint <profileDir>                                        check a server profile

Bit layouts mirror src/TraderX/scripts/4_World/classes/TraderX/Utilities/TraderXTradeQuantity.c
and src/TraderX/scripts/3_Game/Domain/Product/Model/TraderXProduct.c.
"""
import argparse
import glob
import json
import os
import re
import sys

SELL = {"NO_MATTER": 0, "EMPTY": 1, "FULL": 2, "COEFFICIENT": 3, "STATIC": 4}
BUY = {"EMPTY": 0, "FULL": 2, "COEFFICIENT": 3, "STATIC": 4}
BEHAVIOR = {0: "keep", 1: "reset to maxStock", 2: "random 0..max", 3: "keep"}
ID_RE = {"prod": re.compile(r"^prod_[A-Za-z0-9_]+_(\d{3})$"), "cat": re.compile(r"^cat_[A-Za-z0-9_]+_(\d{3})$")}
GARAGE_ID, BANK_ID = 125, 126


def encode_tq(buy, sell, buy_qty=0, sell_qty=0):
    return SELL[sell] | (BUY[buy] << 3) | ((sell_qty & 0x1FFF) << 6) | (buy_qty << 19)


def decode_tq(v):
    names_s = {n: k for k, n in SELL.items()}
    names_b = {n: k for k, n in BUY.items()}
    s, b = v & 0x7, (v >> 3) & 0x7
    sq, bq = (v >> 6) & 0x1FFF, v >> 19
    def part(name, mode, q):
        if name == "COEFFICIENT":
            return f"{name} {min(q, 100)}%"
        if name == "STATIC":
            return f"{name} {q}"
        return name if name else f"INVALID({mode})"
    return f"buy {part(names_b.get(b), b, bq)} / sell {part(names_s.get(s), s, sq)}"


def encode_ss(behavior, destock):
    return (min(destock, 100) & 0x7F) | ((behavior & 0x3) << 7)


def decode_ss(v):
    return f"restart: {BEHAVIOR[(v >> 7) & 3]}, destock {v & 0x7F}% per restart"


def slug(name):
    return re.sub(r"[ .\-]", "_", name.lower())


def load(path, issues):
    try:
        with open(path, encoding="utf-8-sig") as f:
            return json.load(f)
    except Exception as e:  # noqa: BLE001
        issues.append(("ERROR", path, f"invalid JSON: {e}"))
        return None


CSV_LIST_COLS = {"attachments", "variants", "productkeys", "licensesrequired"}


def read_csv(path):
    """Rows of a Source/*.csv as dicts (lower-case keys) + line numbers, parsed like TraderXCsvLoader.c."""
    import csv as _csv
    with open(path, encoding="utf-8-sig", newline="") as f:
        lines = [(i + 1, l.rstrip("\r\n")) for i, l in enumerate(f)]
    lines = [(n, l) for n, l in lines if l.strip()]
    if not lines:
        return []
    head = lines[0][1]
    delim = "\t" if "\t" in head else ("," if "," in head else (";" if ";" in head else "\t"))
    header = [re.sub(r"^[^a-z]+", "", h.strip().lower()) for h in next(_csv.reader([head], delimiter=delim))]
    rows = []
    for n, l in lines[1:]:
        cells = [c.strip() for c in next(_csv.reader([l], delimiter=delim))]
        if not cells or not cells[0]:
            continue
        row = {"_line": n}
        for i, h in enumerate(header):
            v = cells[i] if i < len(cells) else ""
            row[h] = [x.strip() for x in v.split(";") if x.strip()] if h in CSV_LIST_COLS else v
        rows.append(row)
    return rows


def num(v, default):
    if v in ("", None):
        return default
    return float(str(v).replace(",", "."))


def csv_catalog(src_dir, add):
    """products/categories/presets from Source/*.csv, merged in file-name order like the compiler."""
    products, categories, presets = {}, {}, {}
    files = sorted(os.path.basename(p) for p in glob.glob(os.path.join(src_dir, "*.csv")))
    for name in files:
        path = os.path.join(src_dir, name)
        low = name.lower()
        kind = "product" if "product" in low else "categor" if "categor" in low else "preset" if "preset" in low else None
        if not kind:
            add("WARN", path, "file name must contain 'product', 'categor' or 'preset': ignored")
            continue
        seen = {}
        for r in read_csv(path):
            key = r.get({"product": "productkey", "categor": "categorykey", "preset": "presetkey"}[kind], "")
            if not key:
                continue
            if key in seen:
                add("ERROR", f"{path}:{r['_line']}", f"duplicate key '{key}' (first at line {seen[key]})")
            seen[key] = r["_line"]
            r["_file"] = f"{path}:{r['_line']}"
            if kind == "product":
                products[key] = (r["_file"], {
                    "className": r.get("classname", ""),
                    "buyPrice": int(num(r.get("buyprice"), -1)), "sellPrice": int(num(r.get("sellprice"), -1)),
                    "maxStock": int(num(r.get("maxstock"), -1)), "coefficient": num(r.get("coefficient"), 1.0),
                    "buyQtyMode": (r.get("buyqtymode") or "FULL").upper(), "buyQtyValue": num(r.get("buyqtyvalue"), 0),
                    "sellQtyMode": (r.get("sellqtymode") or "FULL").upper(), "sellQtyValue": num(r.get("sellqtyvalue"), 0),
                    "destock": num(r.get("destockcoefficient"), 0), "behavior": int(num(r.get("stockbehavioratrestart"), 0)),
                    "attachments": r.get("attachments", []), "variants": r.get("variants", []), "_csv": True})
            elif kind == "categor":
                vis = (r.get("isvisible") or "1").lower() in ("1", "true", "yes")
                categories[key] = (r["_file"], {"categoryName": r.get("categoryname", ""), "isVisible": vis,
                                                "licensesRequired": r.get("licensesrequired", []), "productIds": r.get("productkeys", [])})
            else:
                presets[key] = (r["_file"], {"productId": r.get("productkey", ""), "attachments": r.get("attachments", [])})
    return products, categories, presets


def check_csv_product(p, d, add):
    if d["buyQtyMode"] not in BUY:
        add("ERROR", p, f"buyQtyMode '{d['buyQtyMode']}' invalid (EMPTY, FULL, COEFFICIENT, STATIC)")
    if d["sellQtyMode"] not in SELL:
        add("ERROR", p, f"sellQtyMode '{d['sellQtyMode']}' invalid (NO_MATTER, EMPTY, FULL, COEFFICIENT, STATIC)")
    for side in ("buy", "sell"):
        mode, v = d[f"{side}QtyMode"], d[f"{side}QtyValue"]
        if mode == "COEFFICIENT" and not 0 < v <= 100:
            add("WARN", p, f"{side}QtyValue {v}: COEFFICIENT is a fraction (0.5) or percent (50); clamped")
        if mode == "STATIC" and not (v >= 1 and float(v).is_integer()):
            add("WARN", p, f"{side}QtyValue {v}: STATIC is a whole number of units; clamped/rounded")
    if d["behavior"] not in (0, 1, 2):
        add("ERROR", p, f"stockBehaviorAtRestart {d['behavior']} invalid (0 keep, 1 reset, 2 random)")
    if not 0 <= d["destock"] <= 100:
        add("WARN", p, f"destockCoefficient {d['destock']} out of range (fraction 0.1 or percent 10)")
    if d["buyPrice"] == -1 and d["sellPrice"] == -1:
        add("WARN", p, "buyPrice and sellPrice both -1/blank: neither buyable nor sellable")
    if d["maxStock"] == -1 and (d["behavior"] or d["destock"]):
        add("WARN", p, "stock settings have no effect with maxStock -1")



def lint(profile):
    root = os.path.join(profile, "TraderX")
    if not os.path.isdir(root):
        sys.exit(f"{root} not found (pass the -profiles folder that contains TraderX/)")
    cfg = os.path.join(root, "TraderXConfig")
    issues = []
    rel = lambda p: os.path.relpath(p, profile)  # noqa: E731

    def add(level, path, msg):
        issues.append((level, rel(path) if os.path.isabs(path) or os.path.exists(path) else path, msg))

    src_path = os.path.join(cfg, "SourceConfig.json")
    src = load(src_path, issues) if os.path.exists(src_path) else None
    fmt = str((src or {}).get("preferredFormat", "LEGACY")).upper()
    if fmt not in ("LEGACY", "CSV", "COMPILED"):
        add("WARN", src_path, f"preferredFormat '{fmt}' unknown: the server uses LEGACY")
        fmt = "LEGACY"
    compiled = os.path.join(cfg, "Compiled")
    source = os.path.join(cfg, "Source")
    print(f"catalog format: {fmt}")

    products, categories, csv_presets = {}, {}, {}
    if fmt == "CSV":
        products, categories, csv_presets = csv_catalog(source, add)
        if not products:
            add("WARN", source, "no product row in Source/*.csv: traders will be empty")
        for pid, (p, d) in products.items():
            check_csv_product(p, d, add)
            if any(ch in pid for ch in "/\\:"):
                add("ERROR", p, f"productKey '{pid}' contains / \\ or :")
        for cid, (p, d) in categories.items():
            if any(ch in cid for ch in "/\\:"):
                add("ERROR", p, f"categoryKey '{cid}' contains / \\ or :")
        for key, (p, d) in csv_presets.items():
            if d["productId"] not in products:
                add("WARN", p, f"preset '{key}': unknown product '{d['productId']}'")
            for a in d["attachments"]:
                if a not in products:
                    add("WARN", p, f"preset '{key}': unknown attachment '{a}'")
    elif fmt == "COMPILED":
        for name, store, idk in (("Products_compiled.json", products, "productId"), ("Categories_compiled.json", categories, "categoryId")):
            path = os.path.join(compiled, name)
            data = load(path, issues) if os.path.exists(path) else None
            if data is None:
                add("ERROR", path, "missing: COMPILED format needs both compiled files")
                continue
            for d in data:
                store[d.get(idk, "")] = (path, d)
    else:
        if glob.glob(os.path.join(compiled, "*_compiled.json")):
            add("WARN", compiled, "LEGACY format: Compiled/ is ignored by the server (delete it to avoid confusion)")
        for kind, folder, store in (("prod", "Products", products), ("cat", "Categories", categories)):
            for p in sorted(glob.glob(os.path.join(cfg, folder, "*.json"))):
                pid = os.path.basename(p)[:-5]
                if not re.match(rf"^{kind}_.+_(\d{{3}})$", pid) or pid.endswith("_000"):
                    add("ERROR", p, f"file name is not a valid {kind}_<name>_NNN id: skipped by the server")
                elif not ID_RE[kind].match(pid):
                    add("WARN", p, "id has characters outside [A-Za-z0-9_]: loaded with a sanitize warning, prefer a clean name")
                data = load(p, issues)
                if isinstance(data, dict):
                    store[pid] = (p, data)

    for pid, (p, d) in products.items():
        for key in ("attachments", "variants"):
            for ref in d.get(key) or []:
                if ref not in products:
                    add("WARN", p, f"{key}: unknown product '{ref}' (ignored by the server)")
        if not d.get("className"):
            add("ERROR", p, "empty className")
        if d.get("_csv"):
            continue
        ss = int(d.get("stockSettings", 0) or 0)
        if (ss & 0x7F) > 100:
            add("ERROR", p, f"stockSettings destock {ss & 0x7F}% > 100")
        if ss and int(d.get("maxStock", -1)) == -1:
            add("WARN", p, "stockSettings set but maxStock is -1 (unlimited): no effect")
        tq = int(d.get("tradeQuantity", 0) or 0)
        if (tq & 7) > 4 or ((tq >> 3) & 7) not in BUY.values():
            add("ERROR", p, f"tradeQuantity {tq} has an invalid mode ({decode_tq(tq)})")
        coef = float(d.get("coefficient", 1.0) or 0)
        if (ss >> 7) & 3 == 3:
            add("WARN", p, "stockSettings restart behaviour 3 is unused (acts as 0 keep)")
        if int(d.get("maxStock", -1)) > 1 and coef <= 0:
            add("WARN", p, "coefficient <= 0 with limited stock: price collapses to 1")
        if int(d.get("buyPrice", 0)) < -1 or int(d.get("sellPrice", 0)) < -1:
            add("ERROR", p, "price below -1")

    gs_path = os.path.join(cfg, "TraderXGeneralSettings.json")
    gs = load(gs_path, issues) if os.path.exists(gs_path) else None
    licenses = {str(l.get("licenseName", "")).lower() for l in (gs or {}).get("licenses") or []}

    for cid, (p, d) in categories.items():
        for ref in d.get("productIds") or []:
            if ref not in products:
                add("WARN", p, f"productIds: unknown product '{ref}' (ignored by the server)")
        for lic in d.get("licensesRequired") or []:
            if lic.lower() not in licenses:
                add("ERROR", p, f"licensesRequired: unknown license '{lic}'")

    cur_path = os.path.join(cfg, "TraderXCurrencySettings.json")
    cur = load(cur_path, issues) if os.path.exists(cur_path) else None
    cur_names = set()
    for t in (cur or {}).get("currencyTypes") or []:
        cur_names.add(t.get("currencyName"))
        if not t.get("currencies"):
            add("WARN", cur_path, f"currency type '{t.get('currencyName')}' has no denominations")

    core_ids = {}
    if gs is not None:
        if gs.get("acceptedStates") is None:
            add("WARN", gs_path, "acceptedStates is null: every state except ruined can be sold")
        elif not any(gs["acceptedStates"].get(k) for k in ("acceptWorn", "acceptDamaged", "acceptBadlyDamaged")):
            add("WARN", gs_path, "acceptedStates all 0: only pristine items can be sold")
        for t in gs.get("traders") or []:
            nid = t.get("npcId")
            tag = f"trader npcId {nid} ({t.get('givenName', '')})"
            if nid in core_ids:
                add("ERROR", gs_path, f"{tag}: duplicate npcId")
            core_ids[nid] = t
            cats = t.get("categoriesId") or []
            if nid in (GARAGE_ID, BANK_ID) and cats:
                add("ERROR", gs_path, f"{tag}: 125/126 are reserved (garage/bank); a trading NPC there is moved at boot")
            for c in cats:
                if c not in categories:
                    add("ERROR", gs_path, f"{tag}: unknown category '{c}'")
            for c in t.get("currenciesAccepted") or []:
                if cur is not None and c not in cur_names:
                    add("ERROR", gs_path, f"{tag}: unknown currency '{c}' (case-sensitive)")
            for lic in t.get("licensesRequired") or []:
                if lic.lower() not in licenses:
                    add("ERROR", gs_path, f"{tag}: unknown license '{lic}'")
            pos = t.get("position") or [0, 0, 0]
            if isinstance(pos, list) and all(abs(float(v)) < 1 for v in pos):
                add("WARN", gs_path, f"{tag}: position is the world origin")

    park_dir = os.path.join(cfg, "TraderXVehicleParking")
    for p in sorted(glob.glob(os.path.join(park_dir, "parkings_*.json"))):
        d = load(p, issues) or {}
        key = os.path.basename(p)[len("parkings_"):-5]
        if str(d.get("traderId", key)) != key:
            add("WARN", p, f"traderId '{d.get('traderId')}' differs from file name id '{key}'")
        if gs is not None and key.isdigit() and int(key) not in core_ids:
            add("WARN", p, f"no core trader with npcId {key}")
        for i, spot in enumerate(d.get("positions") or []):
            if "orientation" in spot and "rotation" not in spot:
                add("ERROR", p, f"positions[{i}] uses 'orientation': the key is 'rotation' (ignored -> 0)")
            if all(abs(float(v)) < 50 for v in spot.get("position") or [0, 0, 0]):
                add("WARN", p, f"positions[{i}] is near the world origin (default placeholder?)")

    pr_dir = os.path.join(root, "TraderXPresets")
    for p in sorted(glob.glob(os.path.join(pr_dir, "presets_*.json"))):
        d = load(p, issues) or {}
        key = os.path.basename(p)[len("presets_"):-5]
        if d.get("productId") != key:
            add("ERROR", p, f"productId '{d.get('productId')}' differs from file name '{key}'")
        if key not in products:
            add("ERROR", p, f"unknown product '{key}'")
        ids = {x.get("presetId") for x in d.get("presets") or []}
        if d.get("defaultPresetId") and d["defaultPresetId"] not in ids:
            add("WARN", p, "defaultPresetId matches no preset")
        allowed = set((products.get(key, (None, {}))[1]).get("attachments") or [])
        for x in d.get("presets") or []:
            for a in x.get("attachments") or []:
                if a not in products:
                    add("ERROR", p, f"preset '{x.get('presetName')}': unknown product '{a}'")
                elif allowed and a not in allowed:
                    add("WARN", p, f"preset '{x.get('presetName')}': '{a}' is not in the product's attachments")

    bank_path = os.path.join(root, "TraderXBanking", "TraderXBankingConfig.json")
    if os.path.exists(bank_path):
        b = load(bank_path, issues) or {}
        fee = float(b.get("transactionFeePercent", 0.05))
        if fee > 1:
            add("ERROR", bank_path, f"transactionFeePercent {fee} is a fraction (0.05 = 5%)")
        if b.get("bankNpcs") is None:
            add("ERROR", bank_path, "bankNpcs must be an array (use [])")
        if gs is not None and BANK_ID not in core_ids:
            add("WARN", gs_path, "no core trader with npcId 126: the bank has no terminal")

    gar_path = os.path.join(root, "TraderXGarage", "TraderXGarageConfig.json")
    if os.path.exists(gar_path):
        g = load(gar_path, issues) or {}
        if g.get("npcs") is None or g.get("currencies") is None:
            add("ERROR", gar_path, "npcs and currencies must be arrays")
        seen = set()
        for n in g.get("npcs") or []:
            nid = n.get("npcId")
            if nid in seen:
                add("ERROR", gar_path, f"garage npcId {nid} duplicated")
            seen.add(nid)
            if nid != GARAGE_ID and nid in core_ids:
                add("ERROR", gar_path, f"garage npcId {nid} collides with a core trader")
            if not n.get("parkSpots"):
                add("ERROR", gar_path, f"garage npcId {nid} has no parkSpots")
        for k in ("repairBodyPercent",):
            if 0 < float(g.get(k, 10)) < 1:
                add("WARN", gar_path, f"{k} is a real percent (10 = 10%), value looks like a fraction")
        ins = g.get("insurance") or {}
        if ins.get("recoveryMode", "INSURED_ONLY") not in ("NONE", "INSURED_ONLY", "ALL_OWNED"):
            add("ERROR", gar_path, f"insurance.recoveryMode '{ins.get('recoveryMode')}' invalid")
        if ins.get("replacementSpec", "CATALOG_PRESET") not in ("CATALOG_PRESET", "BARE"):
            add("ERROR", gar_path, f"insurance.replacementSpec '{ins.get('replacementSpec')}' invalid")

    sz_path = os.path.join(root, "TraderXSafeZone", "TXSafeZoneSettings.json")
    if os.path.exists(sz_path):
        s = load(sz_path, issues) or {}
        if "76561198000000000" in (s.get("adminSteamUIDs") or []):
            add("WARN", sz_path, "placeholder admin 76561198000000000 still present")
        ids = set()
        for z in s.get("locations") or []:
            if z.get("id") in ids:
                add("ERROR", sz_path, f"zone id {z.get('id')} duplicated")
            ids.add(z.get("id"))
            if all(abs(float(v)) < 1 for v in z.get("position") or [0, 0, 0]):
                add("WARN", sz_path, f"zone '{z.get('name')}' at the world origin (first-boot placeholder?)")

    cl_path = os.path.join(root, "TraderXCarLock", "TXCLSettings.json")
    if os.path.exists(cl_path):
        c = load(cl_path, issues) or {}
        for fam, conf in (c.get("lockpickingSettings") or {}).items():
            if conf and conf.get("lockpickClassName") in ("CarLockPick", "BoatLockPick", "HeliLockPick"):
                add("WARN", cl_path, f"{fam}.lockpickClassName '{conf['lockpickClassName']}' is not defined by TraderX: use a real item class")

    order = {"ERROR": 0, "WARN": 1}
    for level, path, msg in sorted(issues, key=lambda i: (order[i[0]], i[1])):
        print(f"{level:5} {path}: {msg}")
    errors = sum(1 for i in issues if i[0] == "ERROR")
    print(f"\n{len(products)} products, {len(categories)} categories, {len(core_ids)} traders; "
          f"{errors} errors, {len(issues) - errors} warnings")
    return 1 if errors else 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    t = sub.add_parser("tq")
    t.add_argument("--buy", choices=BUY, required=True)
    t.add_argument("--sell", choices=SELL, required=True)
    t.add_argument("--buy-qty", type=int, default=0, help="percent for COEFFICIENT, units for STATIC")
    t.add_argument("--sell-qty", type=int, default=0, help="percent for COEFFICIENT, units for STATIC (max 8191)")
    sub.add_parser("tq-decode").add_argument("value", type=int)
    s = sub.add_parser("ss")
    s.add_argument("--behavior", type=int, choices=(0, 1, 2), required=True)
    s.add_argument("--destock", type=int, default=0, help="percent removed at each restart")
    sub.add_parser("ss-decode").add_argument("value", type=int)
    n = sub.add_parser("newid")
    n.add_argument("kind", choices=("prod", "cat"))
    n.add_argument("name")
    n.add_argument("--dir", help="Products/ or Categories/ folder to avoid existing ids")
    sub.add_parser("lint").add_argument("profile")
    a = ap.parse_args()

    if a.cmd == "tq":
        v = encode_tq(a.buy, a.sell, a.buy_qty, a.sell_qty)
        print(v, "#", decode_tq(v))
    elif a.cmd == "tq-decode":
        print(decode_tq(a.value))
    elif a.cmd == "ss":
        v = encode_ss(a.behavior, a.destock)
        print(v, "#", decode_ss(v))
    elif a.cmd == "ss-decode":
        print(decode_ss(a.value))
    elif a.cmd == "newid":
        base = f"{a.kind}_{slug(a.name)}"
        taken = {os.path.basename(p)[:-5] for p in glob.glob(os.path.join(a.dir, "*.json"))} if a.dir else set()
        for i in range(1, 1000):
            cand = f"{base}_{i:03d}"
            if cand not in taken:
                print(cand)
                break
    elif a.cmd == "lint":
        sys.exit(lint(a.profile))


if __name__ == "__main__":
    main()
