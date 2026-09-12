# TraderX build+sign+publish image: packs DayZ PBOs, signs them with BI's own
# DSSignFile under Wine, and publishes to the Steam Workshop.
#
# DayZ Tools is NOT baked in (BI-licensed). Signing bind-mounts its Bin/DsUtils
# folder at /dayztools — see ci/sign-with-dayztools.sh.
#
# This only works because the .p3d models are pre-binarized (ODOL) and committed
# — see ci/binarize-models.sh. There is no Linux p3d binarizer, so MLOD models
# must be converted once on a box with DayZ Tools before this image can build.
#
# Build:  docker build -t traderx-ci ./ci
# Use:    see ci/README.md (local run + GitHub Actions both use this image)

# ---- stage 1: build armake2 (PBO pack / rapify / sign) ----------------------
FROM rust:1-slim-bookworm AS armake

RUN apt-get update \
 && apt-get install -y --no-install-recommends pkg-config libssl-dev ca-certificates \
 && rm -rf /var/lib/apt/lists/*

# 0.3.0 is the last published release. Note: its -x exclude matcher panics on
# backslash path patterns, so pack-linux.sh stages sources with rsync instead.
RUN cargo install armake2 --version 0.3.0 --root /out

# ---- stage 2: runtime (steamcmd base, same debian family) -------------------
FROM cm2network/steamcmd:root

# rsync/wget for the build; the rest are runtime deps of Mikero's libdepbo
# (depbo-tools) — see its readme-linux.txt.
RUN apt-get update \
 && apt-get install -y --no-install-recommends rsync ca-certificates wget \
      liblzo2-2 libogg0 libvorbis0a libvorbisenc2 libvorbisfile3 libuchardet0 \
 && apt-get clean && rm -rf /var/lib/apt/lists/*

# Wine, purely to run BI's DSSignFile.exe — armake2's signatures are Arma3-format
# and DayZ rejects them, so the authoritative signer has to be BI's own tool.
#
# It is a PE32+ x86-64 console app, so wine64 alone is enough: no i386
# multiarch, no Steam, no Proton, no display. DayZ Tools itself is NOT baked in
# (BI-licensed — a published image must not carry it); ci/sign-with-dayztools.sh
# expects DsUtils/ bind-mounted at /dayztools.
#
# NOTE: the wine64 package ships only /usr/lib/wine/wine64 — the /usr/bin/wine
# launcher comes from the `wine` metapackage. Symlink the real binary rather than
# pulling the metapackage (which drags in i386 multiarch and X11 deps we do not
# need for a console exe).
RUN apt-get update \
 && apt-get install -y --no-install-recommends wine64 \
 && apt-get clean && rm -rf /var/lib/apt/lists/* \
 && ln -sf /usr/lib/wine/wine64 /usr/local/bin/wine \
 && ln -sf /usr/lib/wine/wine64 /usr/local/bin/wine64

# Invoked through a symlink, wine cannot find its own loader ("could not exec the
# wine loader") — WINELOADER has to point at the real binary.
ENV WINELOADER=/usr/lib/wine/wine64

# Mikero's depbo-tools (native Linux). Preferred packer: `makepbo -D` is
# DayZ-aware and emits texHeaders.bin, which armake2 does not.
#
# Vendored into the repo (ci/vendor/depbo-tools) rather than downloaded: Mikero
# publishes no stable direct URL, and the only public mirror (PMC) tops out at
# 0.9.89 via torrent — older than the 0.10.13 used here. Baking it in also keeps
# GitHub-hosted runs byte-identical to local ones.
COPY vendor/depbo-tools /opt/depbo-tools
RUN chmod +x /opt/depbo-tools/bin/*
ENV DEPBO_ROOT=/opt/depbo-tools

# Fail fast if the vendored copy can't run here. (Note: `makepbo -h` exits
# non-zero even when it works, so check the dynamic linker instead.)
RUN ldd /opt/depbo-tools/lib/libdepbo.so.* 2>&1 | grep -q "not found" \
    && { echo "depbo-tools: missing shared libs"; ldd /opt/depbo-tools/lib/libdepbo.so.*; exit 1; } \
    || echo "depbo-tools OK"

# yq (mikefarah, single static binary) — parses ci/build.yaml
ARG YQ_VERSION=v4.44.3
RUN wget -q "https://github.com/mikefarah/yq/releases/download/${YQ_VERSION}/yq_linux_amd64" \
      -O /usr/local/bin/yq \
 && chmod +x /usr/local/bin/yq

COPY --from=armake /out/bin/armake2 /usr/local/bin/armake2

# Weilbyte's TOTP helper: generates Steam Guard codes from a shared secret so
# steamcmd can log in unattended. Only used when STEAM_TFASEED is set.
RUN wget -q https://github.com/Weilbyte/steamcmd-2fa/releases/latest/download/steamcmd-2fa \
      -O /usr/local/bin/steamcmd-2fa \
 && chmod +x /usr/local/bin/steamcmd-2fa

# Fail the image build early if the armake2 binary is ABI-incompatible.
RUN armake2 --version && rsync --version | head -1 && yq --version

# NOTE: the scripts are intentionally NOT copied into the image. Run them from
# the mounted repo (`bash ci/pack-linux.sh`) so the checkout is the single source
# of truth — a baked-in copy silently goes stale after every edit.

WORKDIR /repo
ENTRYPOINT ["/bin/bash", "-lc"]
CMD ["bash ci/pack-linux.sh"]
