# V1.5.19 (CONTRACT): post-build check that the FastAccelStepper I2S interrupt path is in IRAM.
# The I2S TX-done interrupt runs permanently (I2S_MUX refreshes the shift register) and is
# IRAM-safe, so it also runs while a flash operation (NVS, SPIFFS, OTA) has disabled the cache.
# Any function of that path placed in flash crashes the ESP32 ("Cache disabled but cached memory
# region accessed", found in H1 with FastAccelStepper 96459ed). This script fails the build if one
# of the functions below ended up in flash (address >= 0x400C0000). Functions removed by the
# linker (e.g. firmware-partial without setup()) are skipped.
Import("env")
import os, subprocess, sys

ISR_PATH = [
    "StepperQueue::fill_i2s_buffer(unsigned char*)",
    "I2sManager::handleTxDone(unsigned char*)",
    "I2sManager::init_mux_buffer(unsigned char*)",
    "I2sManager::i2sMuxSetBit(unsigned char, bool)",
    "i2s_fill_buffer_mux(StepperQueue*, unsigned char*, i2s_fill_state*, unsigned char, unsigned char)",
]
FLASH_TEXT_START = 0x400C0000

def check_iram(source, target, env):
    elf = str(target[0])
    nm = os.path.join(os.path.dirname(env.subst("$CC")), env.subst("$CC").split(os.sep)[-1].replace("gcc", "nm"))
    out = subprocess.run([nm, "-C", elf], capture_output=True, text=True).stdout
    addr = {}
    for line in out.splitlines():
        parts = line.split(" ", 2)
        if len(parts) == 3 and parts[1] in ("T", "t"):
            addr[parts[2]] = int(parts[0], 16)
    bad = [f"{n} at 0x{addr[n]:08x}" for n in ISR_PATH if n in addr and addr[n] >= FLASH_TEXT_START]
    found = [n for n in ISR_PATH if n in addr]
    if bad:
        print("check_iram: FastAccelStepper I2S interrupt code in FLASH (crashes on NVS/SPIFFS access):", file=sys.stderr)
        for b in bad:
            print("  " + b, file=sys.stderr)
        env.Exit(1)
    print(f"check_iram: {len(found)}/{len(ISR_PATH)} I2S interrupt functions linked, all in IRAM", file=sys.stderr)

env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf",
                  env.VerboseAction(check_iram, "Checking IRAM placement of the I2S interrupt path"))
