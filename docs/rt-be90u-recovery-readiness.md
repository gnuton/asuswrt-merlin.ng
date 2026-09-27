# RT-BE90U recovery and hardware readiness

Checked 2026-09-27. The router still runs stock firmware. Recovery entry,
firmware transfer, boot and candidate hardware behavior have not been tested.
The next physical step is a **probe-only recovery rehearsal**, not an
experimental firmware installation.

## Established facts

| Item | Evidence / limit |
| --- | --- |
| Live identity | Read-only collection: `productid=TUF-BE9400`, `odmpid=RT-BE90U` |
| Installed firmware | `3.0.0.6.102_58500-g482542d_1264-g48728_Q7MB`; Linux 5.4.277, AArch64; IPQ5322, SoC ID 593 |
| Firmware volumes | `linux` and `linux2`: each `0x0500b000` / 83,931,136 bytes |
| Other storage | UBI `nvram`, `Factory`, `Factory2`, `jffs2`; bootloader is a separate MTD region |
| UBI snapshot | `ro_mode=0`, `bad_peb_count=0`, `max_ec=2`, `avail_eraseblocks=0`; the last value describes unallocated UBI blocks, not free space inside JFFS |
| Installed bootloader | Version, signature/fuse policy and actual recovery behavior unknown; earlier `bl_version` query was empty |
| Operator resources | Physical access, Ethernet, Linux/macOS and separate Internet connection confirmed; no Windows machine; spare 8/16 GB USB drives untouched |
| Baseline collector | [collect-router-platform.sh](../tools/rt-be90u/collect-router-platform.sh): fixed model/version keys and kernel/MTD/UBI/module metadata; no configuration, credentials or flash contents |

Local evidence is `logs/router-platform-20260927-full.txt` and
`logs/recovery-readiness-20260927.json` in the port workspace. The latter records
rechecked artifact and source-evidence hashes. These are local preparation records,
not physical recovery results.

## Verified rollback files

ASUS lists stock **3.0.0.6.102_58500**, dated 2026-09-23, with the archive checksum
below. This matches the installed version. [ASUS RT-BE90U firmware](https://www.asus.com/us/supportonly/rt-be90u/helpdesk_bios/)

| File | Bytes | SHA-256 |
| --- | ---: | --- |
| `downloads/FW_RT-BE90U_300610258500.zip` | 62,882,106 | `05a5c8227f3ecc0724787bb333728d55449c6984a823f5143108ce649388df75` |
| `image-audit-official-58500/official-58500.trx` | 62,924,077 | `2eb672afc3903cd664d569587c13113dcfc4c1b026acbc9cc8d7fb09441c502a` |

The ZIP integrity check passes. Its sole member,
`RT-BE90U_3.0.0.6_102_58500-g482542d_1264-g48728_Q7MB.trx`, is byte-identical to
the extracted image. The image passes the strict local model, uImage CRC,
length, architecture, load/entry and observed-volume-size checks. Keep both
files on the recovery computer before any outage. A settings backup is a
separate, credential-bearing file: export it privately through the stock UI
and keep it outside Git and shared logs.

The inspected development reference is dev24, source
`a5e817f84beace83a6d5cc7b5cbc766f3f5ea63e`, local image
`TUF-BE9400_3.0.0.6_102_58138-rtbe90u-dev24-vpn.trx`, 59,068,601 bytes,
SHA-256 `0e42b92882f46d084c533433faad1f6ddd712f68ea03d45a34b41db4ce9adb65`.
It also passes [check-image.py](../tools/rt-be90u/check-image.py). This records
an inspected artifact, **not a selection or approval for flashing**. Recheck the
exact versioned BE9400 file selected for any later test; the SDK's generic
`TUF-BE9400.trx` symlink can select its BE6500 output.

## Recovery method and uncertainty

| Source | What it establishes |
| --- | --- |
| [RT-BE90U manual E27831](https://dlcdnets.asus.com/pub/ASUS/wireless/RT-BE90U/E27831_RT-BE90U_UM_EAA_WEB.pdf), pp. 7–8 | Slow-flashing Power LED indicates rescue; distinguishes Reset, WPS, power and three LAN ports |
| [ASUS rescue guide](https://www.asus.com/support/faq/1000814/) | Hold Reset while applying power; release after rescue indication. Its example uses RT-AC68U, Windows and computer `192.168.1.10/24` |
| [RT-BE90U utilities](https://www.asus.com/us/networking-iot-servers/wifi-routers/asus-wifi-routers/asus-rt-be90u/helpdesk_download?model2Name=ASUS-RT-BE90U) | ASUS Firmware Restoration 2.1.0.3 is offered for Windows; it is a rescue utility, not the normal upgrade path |
| Model's vendor source | ASUS TFTP rescue defaults: router `192.168.50.1`, computer `192.168.50.75/24`, filename `TUF-BE9400.trx`; Reset enters rescue, WPS at boot factory-resets |
| [Local TFTP fixture](../tools/rt-be90u/tests/tftp-client-test.py) | Curl transferred the complete verified stock image with option-free 512-byte blocks and counter rollover; repeated 2026-09-27 on loopback only, with no ASUS bootloader execution |

Vendor-source references are inside the checksum-pinned
[ASUS GPL archive](https://dlcdnets.asus.com/pub/ASUS/wireless/TUF-BE9400/GPL_TUF_BE9400_300610258138.zip),
SHA-256 `930142d207b1f9dfc11e2ad17c767abc59a5df5777223dfc99f12a69f1d07b9e`:
`release/src-qca-ipq53xx/uboot_v2016.01/{README.ASUS,include/configs/tuf-be9400.h,common/cmd_tftpServer.c}`.
`README.ASUS` identifies this target as RT-BE90U. U-Boot is not part of this
firmware target's build. Do not build or replace it for this test.

The Linux/macOS TFTP procedure is **inferred from this vendor source**, not an
ASUS-documented Linux/macOS procedure or a verified match for the installed
bootloader. The source handles an ordinary read request with an empty DATA
reply and no flash write. `net/net.c` captures the incoming source address;
the read handler copies it into the temporary peer address before replying.
Consequently, a read-only probe can use the existing computer address
`192.168.50.100/24`; the initial `.75` peer default does not constrain that read.
This still depends on the address surviving the Ethernet link cycle and the
installed bootloader matching the source. Write requests receive firmware in RAM; validation,
flash writes, ASUS completion packets and reboot follow reception. A successful
curl transfer or final TFTP ACK therefore cannot prove successful flashing.

Both the vendor rescue path and the
[normal upgrade path](../release/src-qca-ipq53xx/source-overlay/release/src/router/rc/services.c)
can write **both firmware volumes**. Do not count `linux2` as preserved stock
rollback. Factory/calibration, NVRAM and bootloader writes are outside the
proposed procedure. Isolated stock-library validation accepted a checksum-correct
wrong-model header in earlier tests; it does not establish live model/signature
enforcement. The candidate's 58138 base is older than stock 58500, and parity
with intervening security fixes remains unresolved.

On 2026-09-27, the official [TUF-BE9400 source listing](https://www.asus.com/support/webapi/ProductV2/GetPDDrivers?website=global&model=tuf-gaming-be9400&pdhashedid=hdynufm1kgfbbzn5&pdid=32751&cpu=&osid=8&siteID=www&sitelang=)
offered only GPL **58138**, dated 2026-01-08. The canonical
[RT-BE90U US product](https://www.asus.com/us/networking-iot-servers/wifi-routers/asus-wifi-routers/asus-rt-be90u/helpdesk_download?model2Name=ASUS-RT-BE90U)
source query also returned no packages, using product ID `35388` and hash
`yismamonagtotwda`. Matching 58500 GPL was not listed. ASUS's
[58500 release notes](https://www.asus.com/us/supportonly/rt-be90u/helpdesk_bios/)
describe fixes involving management inputs, AiCloud access, local services,
certificate/upload handling and VPN/account validation. These announcements do
not establish which fixes the candidate contains. Security reconciliation,
including relevant proprietary components, remains a candidate-entry gate;
the local review is `logs/dev24-asus-security-source-review.json`.

## Physical gates

| Gate | Action and pass criterion | Current status |
| --- | --- | --- |
| Preparation | Private settings backup; verified rollback files available offline; saved Ethernet settings; separate connection; operator present and maintenance window | Files and separate connection ready; backup/window pending |
| Rescue access | Isolated Ethernet, Reset/power sequence, documented LED observation and successful read-only TFTP reply; then normal stock boot with unchanged settings/connectivity | Not performed |
| Stock recovery | Separately scoped transfer of the exact stock image, observed completion/reboot and read-only identity/connectivity check | Not performed; read probe alone cannot satisfy this gate |
| Candidate entry | Freeze image/hash and results; resolve blocking lifecycle defects and review older-baseline security exposure; agree on rollback/stop conditions | Not ready |
| Candidate hardware | Controlled installation and tests below; preserve observations tied to exact image hash | Not performed |

### First outage: probe only

1. Operator confirms the private backup and offline stock files. Save the
   recovery computer's current Ethernet configuration and label router cables.
   Keep coordination on the separate connection.
2. Isolate the router from WAN and other clients; connect the recovery computer
   directly to a **LAN** port. The inspected host route uses `eno1` and
   `192.168.50.100`. Keep that network configuration for this first attempt.
3. Power off; hold **Reset**, power on, release when the Power LED slowly
   flashes. Do not hold WPS or press Reset during normal running firmware.
4. Check that the existing `192.168.50.100/24` address remains assigned and
   `192.168.50.1` still routes directly through that Ethernet link. A DHCP-managed
   address may disappear after link loss. If absent, stop; temporary static
   `.75/24` configuration is a separately scoped fallback. Otherwise run only
   this ordinary TFTP read request, bound to the inspected local address:

   ```sh
   curl --silent --show-error --noproxy '*' --proto '=tftp' --tftp-no-options \
     --interface 192.168.50.100 \
     --max-time 5 --output /tmp/rt-be90u-rescue-probe.bin \
     tftp://192.168.50.1/rt-be90u-probe
   ```

   Record curl status, received byte count and LED behavior. The expected reply
   is a successful zero-byte file. If it fails, stop; do not upload, reset
   settings, guess alternative flash commands or change bootloader variables.
5. With **no upload started**, release buttons and power-cycle normally. Restore
   computer settings and cables. Confirm stock identity, wired Internet, DNS
   and existing Wi-Fi before ending the outage.

This gate requires operator presence, cable/network changes and a router power
cycle; it interrupts service. Firmware upload and settings restoration are
separate actions. If a later approved upload begins, do not cut power on a timer
or infer completion from curl alone. No raw `mtd-write` command is part of the plan.

## Candidate hardware test order

Use synthetic VPN accounts and an isolated test network. Preserve stock settings
for rollback; do not restore a candidate-modified configuration over stock.

| Stage | Required observations and stop conditions |
| --- | --- |
| First boot / management | Stable boot, exact model/version, wired admin access, expected storage and services. Stop for boot loops, missing LAN or flash/storage errors |
| Wired / wireless | Every LAN and WAN port, negotiated rates, DHCP/DNS; 2.4/5/6 GHz, WPA modes and MLO with capable clients; guest isolation/VLANs. Keep a wired management path |
| VPN / firewall / DNS | Actual OpenVPN/WireGuard peers; device/default/guest bindings; UDP/TCP DNS; tunnel loss/restart and firewall/WAN restarts; packet capture proves intended exit and absence of WAN escape for protected traffic |
| IPv6 | Explicitly enable only in the isolated test setup; verify intended whole-network blocking and unaffected traffic. IPv6 VPN transport remains unsupported |
| Scripts / storage | JFFS hooks, certificates, service customization; one identified disposable USB drive, Entware install/start/stop and persistence. USB formatting needs a separately identified device |
| Persistence / rollback | Controlled reboot and normal power cycle after writes finish; saved configuration survives. Reinstall verified stock and confirm normal service before declaring recovery tested |
| Performance / longer run | Sustained traffic and service transitions, error counters, memory and thermal stability; compare with stock. Keep ECM disabled until policy enforcement on hardware is established |

Emulator and namespace results remain software evidence. Radio behavior,
acceleration, NAND persistence, bootloader acceptance, recovery and power-loss
safety require separate hardware evidence; deliberate power interruption during
flash is not part of this plan.
