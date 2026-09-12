# CI: Build → Sign → Steam Workshop

Pure-Linux, containerised pipeline. Packs every folder under `src/` into a signed
PBO, assembles `@TraderX/`, publishes to the Steam Workshop. Runs identically
**locally** and on a **GitHub-hosted runner** — no Wine, no DayZ Tools, no
self-hosted box.

| Script | Does | Needs creds |
|---|---|---|
| `ci/pack-linux.sh` | stage → rapify (armake2) → pack (Mikero makepbo) → sign → `meta.cpp` | **no** |
| `ci/publish-workshop.sh` | steamcmd `workshop_build_item` upload | yes |
| `ci/binarize-models.sh` | one-time MLOD→ODOL model conversion | no (needs DayZ Tools box) |
| `ci/sign-with-dayztools.sh` | re-sign with BI's DSSignFile (authoritative) | no (needs DayZ Tools box) |

## Config: `ci/build.yaml`

```yaml
mod_name: "@TraderX"
workshop_item_id: 0
skip:            # NOT rebuilt; their prebuilt .pbo in the output is left alone
  - DayZular
  - TraderPlusMoney
exclude_files:   # rsync patterns excluded from every PBO
  - "*.html"
```

**The build is incremental.** Packages under `skip:` are never touched — drop
release-ready `.pbo` + `.bisign` into `target/@TraderX/addons/` by hand and only
the non-skipped packages get replaced. Verified: preserved PBOs come out
byte-for-byte identical.

A skipped package with **no** `.pbo` in the output is reported as a warning
(shipping it would produce an incomplete mod). Use `--clean` to wipe the output
folder instead of building incrementally.

Env overrides win over the YAML: `MOD_NAME`, `WORKSHOP_ITEM_ID`, `BUILD_SKIP`.

> Always pass `--user "$(id -u):$(id -g)"` to `docker run`. The image's base runs
> as root, so without it your `target/` ends up root-owned and unwritable.

## ⚠️ Signature validity is unproven — read before relying on it

`pack-linux.sh` signs with armake2, but **no Linux tool can authoritatively
verify a DayZ signature**:

- `armake2 verify` rejects 4 of 5 of your real AddonBuilder-signed PBOs
  (`Hash 3 doesn't match`).
- `hemtt utils verify` rejects **all 5** (`Signature does not match`).
- Both are Arma3-oriented; DayZ's PBO/signature format differs subtly.
- armake2 has an open upstream bug about signing correctness
  ([issue #55](https://github.com/KoffeinFlummi/armake2/issues/55)).

So the build only checks that a `.bisign` **exists**, not that it validates —
a crypto check here would be a false alarm either way.

**What this means for you:** if any server running your mod uses
`verifySignatures=2`, armake2's signatures may be rejected and players kicked.
Two safe options:

1. **Test it**: load the built mod on a server with `verifySignatures=2`. If it
   passes, armake2 signing is fine and CI is fully self-contained.
2. **Re-sign authoritatively** before publishing, on a box with DayZ Tools:
   ```bash
   ./ci/sign-with-dayztools.sh          # DSSignFile via Proton, replaces armake2 sigs
   ```
   Packing still happens in the container; only signing moves to BI's own tool.

## The one catch: models must be pre-binarized

There is **no Linux p3d binarizer**. Mikero's depbo-tools *does* run on Linux,
but its `dep3d` goes the wrong way (ODOL → MLOD, de-binarizing); the binarizing
tools (pboProject/binarize) remain Windows-only. HEMTT binarizes only on
Windows, and armake's P3D conversion is incomplete. BI's `binarize.exe` is the
only option.

So the models are converted **once** (they change rarely), committed as ODOL,
and after that the whole build is Linux-native. Only 3 of 9 packages have models:

| Package | .p3d |
|---|---|
| TraderPlusMoney | 36 |
| Keypads | 1 |
| TraderPlusATM | 1 |
| other 6 | 0 |

`pack-linux.sh` **refuses to build** a package containing MLOD models, so a
forgotten conversion fails the build instead of shipping broken models.

Run on a box with DayZ Tools (Steam running, `DISPLAY` set, DayZ **not** running):
```bash
./ci/binarize-models.sh --check     # report MLOD/ODOL counts
./ci/binarize-models.sh             # convert; MLOD originals kept in models-src/
```
Then commit the ODOL `.p3d` (and `models-src/` for the editable sources).

## Local test (do this first)

```bash
docker build -t traderx-ci ./ci

# put your signing key in place (gitignored)
cp ~/DayZWorkspace/keys/YourKey.biprivatekey ci/keys/TraderX.biprivatekey
cp ~/DayZWorkspace/keys/YourKey.bikey        ci/keys/TraderX.bikey

docker run --rm --user "$(id -u):$(id -g)" \
  -v "$PWD:/repo" -v ~/depbo-tools-0.10.13:/opt/depbo-tools:ro -w /repo \
  -e WORKSHOP_ITEM_ID=0 traderx-ci 'bash ci/pack-linux.sh' 
```
Output → `target/@TraderX/{addons,keys,meta.cpp}`. The script checks that every
PBO has a `.bisign` (validity itself isn't machine-checkable — see above).

To skip the model packages while testing before binarizing:
```bash
-e BUILD_SKIP="DayZular,TraderPlusMoney,Keypads,TraderPlusATM"
```

### Publishing: avoid retyping the Guard code

steamcmd caches a refresh token after one successful login, but `docker run --rm`
throws it away — which is why the 30s code feels relentless. Mount a persistent
HOME and the session survives for weeks:

```bash
# FIRST run only: password + a fresh Guard code
docker run --rm --user "$(id -u):$(id -g)" \
  -v "$PWD:/repo" -v "$PWD/ci/.steam-session:/steamhome" -e HOME=/steamhome -w /repo \
  -e STEAM_USERNAME=cretidivada -e STEAM_PASSWORD='...' -e STEAM_GUARD=ABCDE \
  traderx-ci 'bash ci/publish-workshop.sh'

# EVERY run after: username only, no password, no Guard code
docker run --rm --user "$(id -u):$(id -g)" \
  -v "$PWD:/repo" -v "$PWD/ci/.steam-session:/steamhome" -e HOME=/steamhome -w /repo \
  -e STEAM_USERNAME=cretidivada \
  traderx-ci 'bash ci/publish-workshop.sh'
```

`ci/.steam-session/` is gitignored — it holds a live Steam refresh token, treat it
like a credential. When it eventually expires, supply a Guard code once more.

For fully unattended runs (CI), use `STEAM_TFASEED` instead — but read the
public-repo warning below first.

Publish locally (per-run Guard code, nothing 2FA stored):
```bash
docker run --rm --user "$(id -u):$(id -g)" -v "$PWD:/repo" -w /repo \
  -e STEAM_USERNAME=cretidivada -e STEAM_PASSWORD='...' -e STEAM_GUARD=ABCDE \
  -e WORKSHOP_ITEM_ID=123456789 \
  traderx-ci 'bash ci/publish-workshop.sh' 
```

## GitHub Actions

`.github/workflows/publish-workshop.yml`, manual dispatch, two gated jobs:
1. **build** — no secrets at all. Always runs, uploads the mod as an artifact.
2. **publish** — only if build succeeded *and* `publish=true`. Gated behind the
   `steam-publish` environment.

### Repo config
| Kind | Name | Value |
|---|---|---|
| Secret | `DAYZ_PRIVATE_KEY_B64` | `base64 -w0 TraderX.biprivatekey` |
| Secret | `STEAM_USERNAME` | Steam login |
| Secret | `STEAM_PASSWORD` | Steam password |
| Secret | `STEAM_TFASEED` | Steam Guard **shared secret** (TOTP) |
| Variable | `WORKSHOP_ITEM_ID` | published item id (public anyway) |

Create the `steam-publish` environment (Settings → Environments) with **yourself
as a required reviewer**, so every publish needs an explicit approval click.

> ### ⚠️ This repo is public — read before setting `STEAM_TFASEED`
> The TOTP seed is a **permanent 2FA bypass**. Seed + password = full account
> takeover, with no expiry and no per-use limit. Anyone who can get a workflow to
> run (or who gains write access) can exfiltrate both.
>
> Mitigations, in order of effectiveness:
> - **Best:** don't set `STEAM_TFASEED` at all. Publish from your own machine
>   with the `STEAM_GUARD` one-shot code (see local publish above); let CI do
>   build-only. Nothing 2FA-related ever lives on GitHub.
> - Keep the `steam-publish` environment reviewer gate on.
> - Restrict who has write access; treat any workflow edit as security-relevant.

`publish-workshop.sh` accepts **either** `STEAM_GUARD` (per-run code, preferred)
or `STEAM_TFASEED` (unattended). It fails loudly if neither is set.

## Change note (what shows in the Workshop changelog)

Edit **`ci/CHANGENOTE.md`** — its contents become the change note on the next
publish. Lines starting with `#` are stripped, so you can keep instructions in
the file.

Steam renders **BBCode**, not Markdown:
`[b]bold[/b]`, `[i]italic[/i]`, `[list][*]item[/list]`, `[url=..]text[/url]`.

Resolution order:
1. `CHANGENOTE` env var — one-off override
2. `changenote_file:` in `ci/build.yaml` (default `ci/CHANGENOTE.md`)
3. `changenote:` inline in `ci/build.yaml`
4. `CI build <date>` fallback

Multi-line notes are folded to `\n` when written into the VDF, so formatting
survives. The publish script prints the resolved note before uploading — check it
there before the upload goes out.

One-off without touching the file:
```bash
-e CHANGENOTE='[b]Hotfix[/b] fixed money dupe'
```

## Publishing a mod that isn't on the Workshop yet

Set `workshop_item_id: 0` in `ci/build.yaml` and steamcmd creates the item on
first publish. Optionally fill in the new-item fields (they're only read when the
id is 0 — after that the Workshop page owns them):

```yaml
workshop_item_id: 0
title: "My Mod"
description: "What it does."
preview_image: "ci/preview.png"   # 512x512+, <1MB
visibility: 2                      # 0=public 1=friends 2=private 3=unlisted
```

**New items default to `visibility: 2` (private)** so a first upload can't go
public by accident. Flip it on the Workshop page once you've checked it in-game.

Flow:
```bash
# 1. see exactly what will be sent, without uploading
DRY_RUN=1 ... traderx-ci 'bash ci/publish-workshop.sh'

# 2. real publish — creates the item
... traderx-ci 'bash ci/publish-workshop.sh'

# 3. the script writes the new id back into ci/build.yaml automatically.
#    REBUILD so meta.cpp picks it up, then publish again:
... traderx-ci 'bash ci/pack-linux.sh'
```

Step 3 matters: the id must land in both `build.yaml` and `meta.cpp`. If the id
isn't persisted, the next publish creates a **duplicate item** instead of
updating. The script auto-writes it and warns loudly if it can't detect it.

`DRY_RUN=1` prints the generated VDF and exits before contacting Steam — use it
whenever you're unsure what a run would do.

## Building a second mod with this CI

One config file = one Workshop item. Copy the template and select it with `CONFIG=`:

```bash
cp ci/build.template.yaml ci/build.mymod.yaml
# edit mod_name / src_dir / title ...

docker run --rm --user "$(id -u):$(id -g)" -v "$PWD:/repo" -w /repo \
  -e CONFIG=/repo/ci/build.mymod.yaml traderx-ci 'bash ci/pack-linux.sh'
```

`src_dir` points at the folder whose SUBFOLDERS each become a `.pbo`. It may be
absolute, so the mod's sources can live in a different checkout entirely:

```yaml
mod_name: "@MyMod"
src_dir: "/home/dmitri/DayZWorkspace/SomeOtherMod/src"
workshop_item_id: 0
changenote_file: "/home/dmitri/DayZWorkspace/SomeOtherMod/CHANGENOTE.md"
```

If the mod lives outside this repo, mount it too (`-v /path/to/mod:/mod`) and use
container paths in the config. Outputs go to `target/<mod_name>/`, so mods never
collide — verified: building `@DemoMod` left `@TraderXExperimental` untouched.

`changenote_file` is relative to the repo root unless absolute — give a second
mod its own note file, or it inherits TraderX's.

## Workshop item id
Your local `~/p_drive/@TraderXExperimental/meta.cpp` says `publishedid = 0` —
DayZ Tools Publisher never wrote the id back. Take the real id from the mod's
Workshop URL (`.../filedetails/?id=<THIS>`) and set the `WORKSHOP_ITEM_ID`
variable. Leaving it `0` makes steamcmd create a **new** item.

`publish-workshop.sh` refuses to upload if `meta.cpp`'s `publishedid` disagrees
with `WORKSHOP_ITEM_ID`, so you can't silently update the wrong item.

## How the build works (and why these choices)

**Packer: Mikero's `makepbo` (depbo-tools), with armake2 as fallback.**
depbo-tools is Mikero's *native Linux* build — `makepbo -D` is DayZ-aware and
emits **`texHeaders.bin`**, which AddonBuilder also produces and **armake2
silently omits**. Mount it into the container:

```bash
-v /home/dmitri/depbo-tools-0.10.13:/opt/depbo-tools:ro
```
Its runtime deps (lzo2, ogg, vorbis, uchardet) are installed in the image.
Without the mount, `pack-linux.sh` falls back to armake2 and says so.

**The config is rapified by armake2, not makepbo.** `makepbo` runs an
Arma-centric dependency check while binarising `config.cpp`: DayZ references
sounds extension-less (`TraderX\datasets\sounds\coins` → `coins.ogg`) but
makepbo looks for Arma's `.wss` and aborts with `DePbo:missing file(s)` even
though the `.ogg` files are present. `-B` alone does **not** skip it — the
config must already be gone. So the build does
`armake2 rapify config.cpp → config.bin`, deletes the `.cpp`, then packs with
`makepbo -B`. Same `config.bin`, no false positive.

- armake2 fallback sets header extensions mirroring FileBank:
  `product=dayz ugc`, `version=2`, `prefix=<PackageName>` — verified against a
  real AddonBuilder-produced PBO.
- Scripts are **not** baked into the image; run them from the mounted repo
  (`bash ci/pack-linux.sh`) so a stale copy can't silently shadow your edits.
- Sources are staged with `rsync --exclude` rather than armake2's `-x`, because
  **armake2 panics on backslash path patterns** (`end byte index N is out of
  bounds`) and silently emits a 0-byte PBO. Extension patterns are fine; path
  patterns are not.
- Excluded from PBOs: DZML authoring artifacts (`*.html`, `*.css`, `*.js`,
  `_fixtures/`) — the game only needs the compiled `.layout`. Plus the usual
  `*.psd/*.log/*.bak/*.bisign/...`.
- armake2 packs uncompressed (`Method 0`) where FileBank compresses, so PBOs are
  somewhat larger. Valid either way.
- `armake2 build -k` does **not** emit a signature — signing is a separate
  `armake2 sign` call.

## Fallback: Proton build (handles MLOD models directly)

`ci/build-workshop.sh` builds via DayZ Tools AddonBuilder under Proton
(`protontricks-launch --appid 221100`) on a box that has DayZ Tools. It needs no
pre-binarization, but requires Steam running, a display, DayZ not running, and
paths under `$HOME`. Kept as a fallback / for regenerating binarized models.

## Prior art
- [SWARMDayZ/Swarm](https://github.com/SWARMDayZ/Swarm) — AddonBuilder per `src/`
  folder, `DSSignFile` signing, `meta.cpp` templating, steamcmd publish.
  Windows `.bat` only, no CI.
- [smokey-edgy/DayZ-ModStarter](https://github.com/smokey-edgy/DayZ-ModStarter) — `build.bat` around AddonBuilder.
- [team-gsri/actions-make-pbo](https://github.com/team-gsri/actions-make-pbo) — Action packing PBOs with armake2.
- [Weilbyte/steamcmd-2fa](https://github.com/Weilbyte/steamcmd-2fa) — TOTP helper used in the image.
- DayZ Tools `Publisher.exe` is **GUI-only** (no CLI surface), so it can't be automated.
