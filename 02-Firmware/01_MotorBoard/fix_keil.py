"""Fix all source file paths in Keil project after bsp/app reorganization."""
import re

with open(r'MDK-ARM\MotorBoard.uvprojx', 'rb') as f:
    c = f.read()

# Fix the bell-character corruption first
c = c.replace(b'\x07pp', b'\\app')

# Now fix all paths: each entry is <FilePath>OLD</FilePath>
# Fix backslash-variant paths (..\Core\Src\X -> target)
path_fixes = {
    # app/
    rb'..\Core\Src\controller.c':   rb'..\app\Src\controller.c',
    rb'..\Core\Src\LADRC.c':        rb'..\app\Src\LADRC.c',
    rb'..\Core\Src\rs00_motor.c':   rb'..\app\Src\rs00_motor.c',
    rb'..\Core\Src\rs00_mit.c':     rb'..\app\Src\rs00_mit.c',
    rb'..\Core\Src\shell.c':        rb'..\app\Src\shell.c',
    rb'..\Core\Src\cmd.c':          rb'..\app\Src\cmd.c',
    # bsp/
    rb'..\Core\Src\AD7606.c':       rb'..\bsp\Src\AD7606.c',
    rb'..\Core\Src\mpu6050.c':      rb'..\bsp\Src\mpu6050.c',
    rb'..\Core\Src\i2c.c':          rb'..\bsp\Src\i2c.c',
    rb'..\Core\Src\delay.c':        rb'..\bsp\Src\delay.c',
    # Forward-slash variants
    b'../Core/Src/controller.c':    b'../app/Src/controller.c',
    b'../Core/Src/LADRC.c':         b'../app/Src/LADRC.c',
    b'../Core/Src/rs00_motor.c':    b'../app/Src/rs00_motor.c',
    b'../Core/Src/rs00_mit.c':      b'../app/Src/rs00_mit.c',
    b'../Core/Src/shell.c':         b'../app/Src/shell.c',
    b'../Core/Src/cmd.c':           b'../app/Src/cmd.c',
    b'../Core/Src/AD7606.c':        b'../bsp/Src/AD7606.c',
    b'../Core/Src/mpu6050.c':       b'../bsp/Src/mpu6050.c',
    b'../Core/Src/i2c.c':           b'../bsp/Src/i2c.c',
    b'../Core/Src/delay.c':         b'../bsp/Src/delay.c',
}

count = 0
for old, new in path_fixes.items():
    if old in c:
        c = c.replace(old, new)
        count += 1
        print(f'Fixed: {old}')

print(f'\nTotal fixes: {count}')

# Verify no remaining bell chars
if b'\x07' in c:
    print('WARNING: bell character still present!')

# Verify no old Core paths for moved files
moved = [b'controller.c', b'rs00_motor.c', b'rs00_mit.c', b'LADRC.c',
         b'shell.c', b'cmd.c', b'AD7606.c', b'mpu6050.c', b'i2c.c', b'delay.c']
for name in moved:
    # Search for Core/Src/name
    pattern = b'Core' + b'/' + name
    pattern2 = b'Core' + b'\\' + b'Src' + b'\\' + name
    if pattern in c:
        print(f'WARNING: {name} still references Core (/)')
    if pattern2 in c:
        print(f'WARNING: {name} still references Core (\\)')

with open(r'MDK-ARM\MotorBoard.uvprojx', 'wb') as f:
    f.write(c)
print('Done')
