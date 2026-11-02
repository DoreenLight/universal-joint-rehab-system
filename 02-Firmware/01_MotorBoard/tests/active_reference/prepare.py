"""Generate tests from the saved pre-change source and current production wrappers."""
from pathlib import Path
import hashlib
import json
import re

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
FIRMWARE = HERE.parents[1]
build = HERE / 'build'
build.mkdir(exist_ok=True)
old = (HERE / 'main.before.bin').read_bytes()
new = (FIRMWARE / 'Core/Src/main.c').read_bytes()

def functions(data, start):
    first = data.index(start)
    end = data.index(b'void Order_Creat_process(void)', first)
    return data[first:end].decode('ascii')

legacy = functions(old, b'/* --- T-wave virtual trajectory tick (100Hz) --- */')
legacy = legacy.replace('actv_twave_tick_m2', 'legacy_tick_m2').replace('actv_twave_tick(', 'legacy_tick(')
(build / 'legacy.inc').write_text(legacy, encoding='ascii')
current = functions(new, b'/* Visual reference only: retain original call sites')
(build / 'wrappers.inc').write_text(current, encoding='ascii')

def strip_reference(data):
    data = re.sub(rb'#include "active_reference.h"\r?\n', b'', data)
    start = data.index(b'/* --- Active rehab ')
    end = data.index(b'static float        g_actv_traj_pos', start)
    data = data[:start] + data[end:]
    data = re.sub(rb'static actv_rehab_t g_actv_m2 = .*?;\r?\n', b'', data)
    for motor in (1, 2):
        pattern = rb'            (?:g_actv_m%d.active = 1u;|actv_ref_reset\(&g_actv_m%d,)' % (motor, motor)
        start = re.search(pattern, data).start()
        name = b'g_actv_traj_pos' + (b'_m2' if motor == 2 else b'')
        end = data.index(b'\n', data.index(name, start)) + 1
        data = data[:start] + data[end:]
    start = data.index(b'/* --- T-wave virtual trajectory tick') if b'/* --- T-wave virtual trajectory tick' in data else data.index(b'/* Visual reference only: retain original call sites')
    end = data.index(b'void Order_Creat_process(void)', start)
    return data[:start] + data[end:]

assert strip_reference(old) == strip_reference(new), 'Unrelated main.c bytes changed'
changed = []
for relative, expected in json.loads((HERE / 'baseline-sha256.json').read_text()).items():
    path = ROOT / relative
    if hashlib.sha256(path.read_bytes()).hexdigest() != expected:
        changed.append(relative.replace('\\', '/'))
assert changed == ['02-Firmware/01_MotorBoard/Core/Src/main.c'], changed
(HERE / 'scope-results.txt').write_text(
    'PASS: all baseline source hashes unchanged except main.c\n'
    'PASS: main.c bytes identical after excluding only reference include/types/reset/wrappers\n', encoding='utf-8')
print('PASS source boundary checks; extracted actual old functions and new wrappers')
