#!/usr/bin/env python3
"""Replace a Workshop item's gallery (additional preview images) through the running Steam client.

usage: workshop-meta.py <item_id> <remove_count> <image>...   (DRY=1 to only init and resolve symbols)
Content and title are left untouched. Env DESC=<bbcode file> sets the description, PREVIEW=<image> the main preview.
"""
import ctypes as C, os, sys, time

LIB = os.path.expanduser("~/.steam/steam/steamapps/common/DayZServer/libsteam_api.so")
APPID = 221100
SUBMIT_RESULT_CB = 3400 + 4  # SubmitItemUpdateResult_t


class SubmitResult(C.Structure):
    _fields_ = [("result", C.c_int), ("legal", C.c_bool), ("file_id", C.c_uint64)]


def sym(lib, names, restype, argtypes):
    for n in names:
        try:
            f = getattr(lib, n)
        except AttributeError:
            continue
        f.restype, f.argtypes = restype, argtypes
        return f
    raise SystemExit("missing symbol: " + "/".join(names))


def main():
    item, remove_count, images = int(sys.argv[1]), int(sys.argv[2]), [os.path.abspath(p) for p in sys.argv[3:]]
    for p in images:
        if not os.path.isfile(p):
            raise SystemExit("no such image: " + p)
    work = os.path.dirname(os.path.abspath(__file__))
    os.chdir(work)
    open("steam_appid.txt", "w").write(str(APPID))
    os.environ["SteamAppId"] = os.environ["SteamGameId"] = str(APPID)

    lib = C.CDLL(LIB)
    err = C.create_string_buffer(1024)
    if hasattr(lib, "SteamAPI_InitFlat"):
        r = sym(lib, ["SteamAPI_InitFlat"], C.c_int, [C.c_char_p])(err)
        if r != 0:
            raise SystemExit(f"SteamAPI_InitFlat failed ({r}): {err.value.decode(errors='replace')}")
    elif not sym(lib, ["SteamAPI_Init", "SteamAPI_InitSafe"], C.c_bool, [])():
        raise SystemExit("SteamAPI_Init failed: is Steam running and logged in?")
    shutdown = sym(lib, ["SteamAPI_Shutdown"], None, [])
    try:
        run_cb = sym(lib, ["SteamAPI_RunCallbacks"], None, [])
        ugc = sym(lib, [f"SteamAPI_SteamUGC_v0{v}" for v in (21, 20, 18, 17, 16, 15, 14)], C.c_void_p, [])()
        utils = sym(lib, ["SteamAPI_SteamUtils_v010", "SteamAPI_SteamUtils_v009"], C.c_void_p, [])()
        completed = sym(lib, ["SteamAPI_ISteamUtils_IsAPICallCompleted"], C.c_bool, [C.c_void_p, C.c_uint64, C.POINTER(C.c_bool)])
        get_result = sym(lib, ["SteamAPI_ISteamUtils_GetAPICallResult"], C.c_bool,
                         [C.c_void_p, C.c_uint64, C.c_void_p, C.c_int, C.c_int, C.POINTER(C.c_bool)])
        start = sym(lib, ["SteamAPI_ISteamUGC_StartItemUpdate"], C.c_uint64, [C.c_void_p, C.c_uint32, C.c_uint64])
        remove = sym(lib, ["SteamAPI_ISteamUGC_RemoveItemPreview"], C.c_bool, [C.c_void_p, C.c_uint64, C.c_uint32])
        add = sym(lib, ["SteamAPI_ISteamUGC_AddItemPreviewFile"], C.c_bool, [C.c_void_p, C.c_uint64, C.c_char_p, C.c_int])
        submit = sym(lib, ["SteamAPI_ISteamUGC_SubmitItemUpdate"], C.c_uint64, [C.c_void_p, C.c_uint64, C.c_char_p])
        if not ugc or not utils:
            raise SystemExit("ISteamUGC / ISteamUtils unavailable")
        if os.environ.get("DRY"):
            print("dry run OK: Steam initialised, symbols resolved")
            return

        h = start(ugc, APPID, item)
        if h in (0, 2**64 - 1):
            raise SystemExit("StartItemUpdate failed")
        # remove from the highest index down so lower indices stay valid
        for i in reversed(range(remove_count)):
            if not remove(ugc, h, i):
                raise SystemExit(f"RemoveItemPreview({i}) rejected")
        for p in images:
            if not add(ugc, h, p.encode(), 0):  # k_EItemPreviewType_Image
                raise SystemExit("AddItemPreviewFile rejected " + p)
        set_str = lambda n: sym(lib, [n], C.c_bool, [C.c_void_p, C.c_uint64, C.c_char_p])
        if os.environ.get("DESC"):
            text = open(os.environ["DESC"], encoding="utf-8").read().strip()
            if not set_str("SteamAPI_ISteamUGC_SetItemDescription")(ugc, h, text.encode("utf-8")):
                raise SystemExit("SetItemDescription rejected")
        if os.environ.get("PREVIEW"):
            if not set_str("SteamAPI_ISteamUGC_SetItemPreview")(ugc, h, os.path.abspath(os.environ["PREVIEW"]).encode()):
                raise SystemExit("SetItemPreview rejected")
        call = submit(ugc, h, None)
        print(f"submitted: remove {remove_count}, add {len(images)} to {item}")
        res, failed, done = SubmitResult(), C.c_bool(False), C.c_bool(False)
        deadline = time.time() + 300
        while time.time() < deadline:
            run_cb()
            if completed(utils, call, C.byref(failed)):
                ok = get_result(utils, call, C.byref(res), C.sizeof(res), SUBMIT_RESULT_CB, C.byref(failed))
                print(f"result: EResult={res.result} ok={ok and res.result == 1} legal={res.legal} failed={failed.value}")
                sys.exit(0 if ok and res.result == 1 else 1)
            time.sleep(0.2)
        raise SystemExit("timed out")
    finally:
        shutdown()


if __name__ == "__main__":
    main()
