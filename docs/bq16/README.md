# Experimental BQ16 support

This port is not yet approved for flashing or daily use. Test results are
tracked in `tasks/todo.md`; unchecked hardware tests remain unverified.

## Source and hardware baseline

- Merlin base: `920b77f5f92db14717a27abd5c8e1b06ae6c8ec1`, version 3006.102.6.
- ASUS donor: `GPL_BQ16_3.0.0.6.102.39256-g4282d34_1529-gfdee1_BB0B.tgz`.
- Stock image: `ZenWiFi_BQ16_3.0.0.6_102_39256-g4282d34_1529-gfdee1_BB0B_nand_squashfs.pkgtb`.
- Stock image SHA-256: `00c411ef8ec2c7be2fa071acc0a6fbfe66b6d59328a72c8ef6c1a2462cc7091a`.
- Actual unit: BQ16, EU/01, board `BQ16_2556_2`, bootloader 1.0.0.10.
- SDK: `src-rt-5.04behnd.4916`, profile `96813GW`, chip 6813, Wi-Fi impl103.
- Kernel: 4.19.294, AArch64. Router programs: 32-bit ARM soft-float ABI.
- Wireless driver: 17.10.369.39012, BSP BB0B.
- Vendor settings: BCM53134 switch, NVSIZE 128, MLO_CONFIG_556 and
  MLO_DEFAULT_ACTIVE. The kernel profile defaults to BQ16_2556, while
  the stock boot bundle also contains BQ16_2556_2.
- Existing WAN: PPPoE through VLAN 835. Keep ISP credentials private.

The supplied image and live router have matching hashes for `wl`,
`libnvram.so`, `cfg_server`, and `dhd.ko`. The GPL's `cfg_server` matches
the image. Matching version strings alone do not establish ABI compatibility.

## Build isolation

Use independent disposable copies for ASUS BQ16, Merlin GT-BE98, and
the BQ16 port. Makefile evaluation creates configurations and symlinks,
including with `make -n`. Do not run it in the donor or development tree.
Use the fork's `bcm-hnd-ax-4.19be_soft.sh` toolchain environment.

BQ16 leaves NFS file sharing disabled, following the working Wi-Fi 7
profiles and the user's selected default. Enabling it currently selects
BusyBox's legacy RPC mount code, whose headers are absent from this
glibc 2.32 toolchain. Scripts, DNS services, VPN, and AiMesh remain enabled.

The pinned image and source hashes are in `inputs.json`. Start the
top-level build with `make bq16 PARALLEL_BUILD=-j4`, not `make -j4 bq16`.
The latter starts package configuration before prerequisite libraries
are staged, as reproduced by the GT-BE98 control's Neon failure.

Run `python3 -B tools/import-bq16.py --source /path/to/asuswrt
--destination /path/to/merlin --dry-run` to check donor inputs without
copying them. Run from the donor with `REPO=/path/to/merlin
/path/to/merlin/tools/copy-prebuilt-be BQ16` to perform the import.

Run `python3 -B tools/verify-bq16-image.py image.pkgtb --report report.json`
for offline integrity and ABI checks, and `python3 -B -m unittest discover
-s tools/tests -v` for corruption regression tests.

Validation uses the separate Ubuntu 24.04 image pinned in `inputs.json`.
Install `python3 binutils squashfs-tools lzop` there. The old compiler
container's SquashFS utility lacks `-cat`; avoid extracting firmware
contents onto the host as a workaround.

The recorded UBI layout has 126976-byte logical eraseblocks, 192 free
blocks, and 1140 blocks reserved for the two bootfs/rootfs pairs. The
validator checks that two copies of the candidate fit this budget without
shrinking metadata, data, defaults, or JFFS. This is a capacity check,
not proof of the updater's resize behavior. Re-record the layout before
validating a different unit or a unit whose UBI allocation changed.

## Stock wireless limitation

Before any custom firmware was installed, `/tmp/boot_dmesg.log` reported
PCIe core 1 link down. Cores 0, 2, and 3 were up. Only wl0, wl1, and wl2
were present, and the stock MLO mapping was `2 1 0 -1`. This explains why
the stock unit exposes three wireless interfaces, but does not establish
the reason for the PCIe link failure. Retain the complete board/radio
definitions and investigate this separately before four-band acceptance.

## Recovery preparation

The ASUS procedure is documented at
https://www.asus.com/us/support/faq/1000814/ . It is a starting procedure,
not evidence that recovery has been exercised on this specific BQ16.

Before the first custom flash:

- Verify private configuration and JFFS backups and retain the stock image.
- Prepare a recovery computer with wired Ethernet and the applicable ASUS
  restoration utility or the unit's verified bootloader recovery interface.
- Arrange alternative internet access and a maintenance window.
- Verify rescue-mode reachability on this unit and record the actual
  recovery interface, address, and stock-restoration procedure.
- Confirm image verification requirements for bootloader 1.0.0.10.
- Review the candidate hash and receive explicit approval to flash it.

The router's `firmware_check` applet is not a read-only probe. The source
in rc/services.c calls `fw_check_pre()` and changes the `firmware_check`
NVRAM status. Its `check_imagefile()` implementation comes from a vendor
object. Do not invoke it as an assumed harmless check on the live gateway.
The BSP U-Boot configuration enables FIT signature support, while the
stock bundles contain no signature nodes. That combination does not
establish the factory loader's fuse state or enforcement policy.

Use the normal BQ16 firmware updater. Do not write raw MTD partitions,
erase factory data, or install loader-inclusive images. The standard
pkgtb bootfs contains required ATF and U-Boot components; do not remove
them merely because they are bootloader components.

## Hardware acceptance

Require working SSH/web administration, correct board selection,
persistent settings, three reboot cycles, and a verified rollback path.
Check PPPoE/VLAN 835, DHCP, DNS, IPv6 where configured, LAN switching,
guest isolation, and WAN reconnects. Validate all logical wireless bands,
EU regulatory settings, and MLO. Explain the stock unit's three visible
wl interfaces before changing band/interface assignments.

Test JFFS scripts, custom configuration hooks, DNSSEC/DNS privacy,
WireGuard/OpenVPN, policy routing, and traffic monitoring. Test stock
AiMesh nodes over wired and wireless backhaul, including onboarding,
configuration sync, roaming, and reboot reconnects. Complete a 72-hour
traffic test without unexpected reboot or kernel fault and compare
throughput with stock firmware.
