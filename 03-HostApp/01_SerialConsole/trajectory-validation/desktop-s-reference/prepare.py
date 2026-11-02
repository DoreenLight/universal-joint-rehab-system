"""Restore this tested release and certify bounded host-only source changes."""
from pathlib import Path
import hashlib
import json
import re
import zipfile

HERE = Path(__file__).resolve().parent
manifest = json.loads((HERE / 'baseline-sha256.json').read_text(encoding='utf-8'))
tested = json.loads((HERE / 'tested-release.json').read_text(encoding='utf-8'))
source, before = HERE / 'build/source', HERE / 'build/before'
source.mkdir(parents=True, exist_ok=True)
before.mkdir(parents=True, exist_ok=True)
files = list(manifest['source_sha256'])
with zipfile.ZipFile(HERE / 'before-adjustable-source.zip') as old_zip, zipfile.ZipFile(HERE / 'tested-host-source.zip') as new_zip:
    assert set(old_zip.namelist()) == set(new_zip.namelist()) == set(files)
    for name in files:
        old, new = old_zip.read(name), new_zip.read(name)
        assert hashlib.sha256(old).hexdigest() == manifest['source_sha256'][name]
        if name in tested['source_sha256']:
            assert hashlib.sha256(new).hexdigest() == tested['source_sha256'][name]
        else:
            assert old == new, name
        (before / name).write_bytes(old)
        (source / name).write_bytes(new)

def normalized(path):
    return path.read_bytes().replace(b'\r\n', b'\n')

def region(data, signature):
    start = data.index(signature)
    end = data.index(b'\n}', start) + 2
    return start, end

old, new = normalized(before / 'mainwindow.cpp'), normalized(source / 'mainwindow.cpp')
protected = [b'void MainWindow::onTelemetryReceived(', b'void MainWindow::feedTrackerSample(',
             b'void MainWindow::startExperiment(', b'void MainWindow::updateTrackerViews(',
             b'void MainWindow::buildTrackerChart(']
for signature in protected:
    a, b = region(old, signature)
    c, d = region(new, signature)
    assert old[a:b] == new[c:d], signature

def strip_allowed(data):
    for signature in [b'void MainWindow::createRehabPage(', b'void MainWindow::loadTrackerParams(', b'void MainWindow::applyReference(']:
        a, b = region(data, signature)
        data = data[:a] + signature + b' /* scoped change */' + data[b:]
    a = data.index(b'// Local S reference:')
    b = data.index('// 图例条目固定为三条数据曲线'.encode(), a)
    return data[:a] + b'/* scoped reference generator */\n' + data[b:]

assert strip_allowed(old) == strip_allowed(new), 'Unrelated implementation changed'
command_lines = lambda d: [line.strip() for line in d.splitlines() if re.search(rb'(send[A-Z]\w*|Q_ARG\(|QMetaObject::invokeMethod)', line)]
assert command_lines(old) == command_lines(new), 'Device command lines changed'
old_h, new_h = normalized(before / 'mainwindow.h'), normalized(source / 'mainwindow.h')
expected_h = old_h.replace(b'#include <QSpinBox>', b'#include <QSpinBox>\n#include <QDoubleSpinBox>')
expected_h = expected_h.replace('        float speed = 30.0f;    // °/s'.encode(), b'        qreal moveSeconds = 6.0; // Local reference only; one-way duration')
expected_h = expected_h.replace(b'    QSpinBox *trackerCyclesSpin = nullptr;', b'    QSpinBox *trackerCyclesSpin = nullptr;\n    QDoubleSpinBox *trackerM1MoveSpin = nullptr;\n    QDoubleSpinBox *trackerM2MoveSpin = nullptr;')
assert new_h == expected_h, 'Unrelated header change'
# Original trapezoid is retained as a historical 40-second timing oracle.
with zipfile.ZipFile(HERE / 'desktop-original-source.zip') as original:
    legacy = original.read('mainwindow.cpp')
a = legacy.index(b'static QList<QPointF> makeRefTrajectory(')
b = legacy.index('// 图例条目固定为三条数据曲线'.encode(), a)
(HERE / 'build/legacy-reference.inc').write_bytes(legacy[a:b].replace(b'makeRefTrajectory(', b'legacyMakeRefTrajectory(', 1))
(HERE / 'scope-results.txt').write_text(
    'PASS: only local duration controls, snapshot loading/locking and reference timing changed.\n'
    'PASS: telemetry, feedback, recording, experiment clocks and tracker rendering functions unchanged.\n'
    'PASS: device command lines and all six other production source files unchanged.\n', encoding='utf-8')
print('PASS: bounded host-only changes; feedback, protocol and clock paths unchanged')
