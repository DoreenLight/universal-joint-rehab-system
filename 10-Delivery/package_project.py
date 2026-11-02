"""Bundle both actual projects without changing their sources or deployed runtime."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import os
import shutil
import time
import zipfile

OUT = Path(__file__).resolve().parent
PROJECT = OUT.parent
DESKTOP = Path(r'C:\Users\Lenovo\Desktop\腕部驱动设备')
RUNTIME_REL = Path('build/Desktop_Qt_6_8_2_MinGW_64_bit_Release')
RUNTIME = DESKTOP / RUNTIME_REL
WORK = OUT / 'work_20261026'
PAYLOAD = OUT / 'launch-support'
RELOCATED = WORK / 'relocated' / '分享包 测试' / 'WristRehab'
ARCHIVE = OUT / '腕关节康复项目_完整分享包_20261026.zip'
PACKAGE = 'WristRehab/'
RECEIPT = PROJECT / '03-HostApp/01_SerialConsole/trajectory-validation/desktop-s-reference/deployment-receipt.json'

def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()

def inventory(root, prefix):
    files, excluded, empty = [], [], []
    for parent, dirs, names in os.walk(root, followlinks=False):
        parent = Path(parent)
        dirs.sort()
        for name in dirs[:]:
            item = parent / name
            reason = None
            if name in {'.git', '.claude'}:
                reason = 'local version-control or AI metadata'
            elif root == PROJECT and parent == root and name == '10-Delivery':
                reason = 'package output; prevent self inclusion'
            elif item.is_junction() or item.is_symlink():
                reason = 'directory link; canonical source already included'
            if reason:
                dirs.remove(name)
                excluded.append({'path': prefix + '/' + item.relative_to(root).as_posix(), 'reason': reason})
        if not dirs and not names:
            empty.append(prefix + '/' + parent.relative_to(root).as_posix().rstrip('/') + '/')
        for name in sorted(names):
            item = parent / name
            if item.is_symlink():
                excluded.append({'path': prefix + '/' + item.relative_to(root).as_posix(), 'reason': 'file link'})
            else:
                files.append((prefix + '/' + item.relative_to(root).as_posix(), item))
    return files, excluded, empty

receipt = json.loads(RECEIPT.read_text(encoding='utf-8-sig'))
assert digest(RUNTIME / 'Uart_controller.exe') == receipt['executableSha256'], 'Deployed program changed; re-audit version'
for name, expected in receipt['sourceSha256'].items():
    assert digest(DESKTOP / '01_上位机程序' / name) == expected, name

entries, exclusions, empty_dirs = [], [], []
for root, prefix in [(PROJECT, 'Project'), (DESKTOP, 'DesktopHost')]:
    members, omitted, empty = inventory(root, prefix)
    entries.extend(members)
    exclusions.extend(omitted)
    empty_dirs.extend(empty)

qt_key = 'DesktopHost/' + RUNTIME_REL.as_posix() + '/qt.conf'
entries = [(key, path) for key, path in entries if key != qt_key]
for item in sorted(PAYLOAD.rglob('*')):
    if not item.is_file():
        continue
    relative = item.relative_to(PAYLOAD).as_posix()
    entries.append((qt_key if relative == 'qt.conf' else relative, item))
entries.sort(key=lambda entry: entry[0])
assert len(entries) == len({key.casefold() for key, _ in entries}), 'Duplicate package path'
assert all(OUT not in path.parents or PAYLOAD in path.parents for _, path in entries), 'Self inclusion'
source_bytes = sum(path.stat().st_size for _, path in entries)
print(json.dumps({'files': len(entries), 'input_bytes': source_bytes, 'excluded': len(exclusions)}, ensure_ascii=True), flush=True)

# Prepare only the portable runtime and launch helpers at a different, spaced,
# Chinese-containing path. Source archives are streamed directly into the ZIP.
runtime_test = RELOCATED / 'DesktopHost' / RUNTIME_REL
runtime_test.mkdir(parents=True, exist_ok=True)
for item in RUNTIME.rglob('*'):
    if item.is_file() and (item.suffix.lower() == '.dll' or item.name == 'Uart_controller.exe'):
        target = runtime_test / item.relative_to(RUNTIME)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(item, target)
shutil.copy2(PAYLOAD / 'qt.conf', runtime_test / 'qt.conf')
for item in PAYLOAD.rglob('*'):
    if item.is_file() and item.name != 'qt.conf':
        target = RELOCATED / item.relative_to(PAYLOAD)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(item, target)

manifest, processed, last_report = [], 0, time.monotonic()
already_compressed = {'.zip', '.rar', '.7z', '.gz', '.xz', '.mp4', '.jpg', '.jpeg', '.png'}
with zipfile.ZipFile(ARCHIVE, 'w', allowZip64=True, compresslevel=4) as archive:
    for name in sorted(set(empty_dirs)):
        archive.writestr(PACKAGE + name, b'')
    for index, (key, path) in enumerate(entries, 1):
        before = path.stat()
        info = zipfile.ZipInfo.from_file(path, PACKAGE + key)
        info.compress_type = zipfile.ZIP_STORED if path.suffix.lower() in already_compressed else zipfile.ZIP_DEFLATED
        info._compresslevel = 4
        h = hashlib.sha256()
        with path.open('rb') as src, archive.open(info, 'w', force_zip64=before.st_size >= 2**31) as dest:
            for chunk in iter(lambda: src.read(1024 * 1024), b''):
                h.update(chunk)
                dest.write(chunk)
        after = path.stat()
        assert (before.st_size, before.st_mtime_ns) == (after.st_size, after.st_mtime_ns), f'File changed during packaging: {key}'
        manifest.append({'path': key, 'bytes': before.st_size, 'sha256': h.hexdigest()})
        processed += before.st_size
        if time.monotonic() - last_report >= 5:
            print(json.dumps({'progress_files': index, 'total_files': len(entries), 'processed_bytes': processed, 'total_bytes': source_bytes}), flush=True)
            last_report = time.monotonic()
    metadata = {
        'date': '2026-10-26', 'created_utc': datetime.now(timezone.utc).isoformat(),
        'file_count': len(manifest), 'uncompressed_file_bytes': source_bytes,
        'project_snapshot': 'Project', 'desktop_snapshot': 'DesktopHost',
        'desktop_executable_sha256': receipt['executableSha256'],
        'default_single_leg_seconds': {'Motor1': 6, 'Motor2': 6},
        'default_endpoint_hold_seconds': 2, 'default_cycles': 5,
        'default_total_seconds': 80, 'exclusions': exclusions,
        'package_only_change': qt_key + ': locate bundled Qt plugins',
        'flashed': False,
    }
    archive.writestr(PACKAGE + 'PACKAGE_INFO.json', json.dumps(metadata, ensure_ascii=False, indent=2).encode('utf-8'), compress_type=zipfile.ZIP_DEFLATED)
    archive.writestr(PACKAGE + 'PACKAGE_MANIFEST.jsonl', ''.join(json.dumps(row, ensure_ascii=False) + '\n' for row in manifest).encode('utf-8'), compress_type=zipfile.ZIP_DEFLATED)

(OUT / 'package-info.json').write_text(json.dumps(metadata, ensure_ascii=False, indent=2), encoding='utf-8')
(OUT / 'package-manifest.jsonl').write_text(''.join(json.dumps(row, ensure_ascii=False) + '\n' for row in manifest), encoding='utf-8')
print(json.dumps({'archive_written': str(ARCHIVE), 'archive_bytes': ARCHIVE.stat().st_size}, ensure_ascii=True), flush=True)
archive_hash = digest(ARCHIVE)
(OUT / (ARCHIVE.name + '.sha256')).write_text(archive_hash + '  ' + ARCHIVE.name + '\n', encoding='utf-8')
print(json.dumps({'archive_sha256': archive_hash}, ensure_ascii=True), flush=True)
