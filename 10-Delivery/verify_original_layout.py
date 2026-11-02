from pathlib import Path
import zipfile,json,hashlib,time
out=Path(__file__).resolve().parent
old=out/'腕关节康复项目_完整分享包_20261026.zip'
new=out/'腕关节康复项目_原目录结构分享包_20261026.zip'
prefix='universal-joint-rehab-system/'
with zipfile.ZipFile(old) as archive:
    old_rows=[json.loads(line) for line in archive.read('WristRehab/PACKAGE_MANIFEST.jsonl').decode('utf-8').splitlines()]
with zipfile.ZipFile(new) as archive:
    rows=[json.loads(line) for line in archive.read(prefix+'PACKAGE_MANIFEST.jsonl').decode('utf-8').splitlines()]
    info=json.loads(archive.read(prefix+'PACKAGE_INFO.json'))
    paths={row['path']:row for row in rows}
    members=archive.namelist()
    assert len(members)==len(set(members)), 'Duplicate entries'
    assert all(name.startswith(prefix) for name in members), 'Incorrect outer folder'
    assert not any(name.startswith(prefix+part+'/') for name in members for part in ['Project','DesktopHost','WristRehab','tools']), 'Extra regrouping layer'
    for name in ['01-CANAnalyzer','02-Firmware','03-HostApp','09-Archive','腕部驱动设备']:
        assert any(key.startswith(name+'/') for key in paths),name
    preserved=0
    for row in old_rows:
        key=row['path']
        if key.startswith('Project/'):key=key[len('Project/'):]
        elif key.startswith('DesktopHost/'):key='腕部驱动设备/'+key[len('DesktopHost/'):]
        else:continue
        assert key in paths, 'Missing original path: '+key
        assert paths[key]['sha256']==row['sha256'] and paths[key]['bytes']==row['bytes'], 'Original file changed: '+key
        preserved+=1
    expected={prefix+row['path'] for row in rows}|{prefix+'PACKAGE_INFO.json',prefix+'PACKAGE_MANIFEST.jsonl'}
    assert {entry.filename for entry in archive.infolist() if not entry.is_dir()}==expected
    checked=0;total=0;last=time.monotonic()
    for row in rows:
        digest=hashlib.sha256();size=0
        with archive.open(prefix+row['path']) as source:
            for block in iter(lambda:source.read(1024*1024),b''):
                size+=len(block);digest.update(block)
        assert size==row['bytes'] and digest.hexdigest()==row['sha256'],row['path']
        checked+=1;total+=size
        if time.monotonic()-last>=5:
            print(json.dumps({'verified':checked,'total':len(rows)}),flush=True);last=time.monotonic()
    assert checked==info['file_count'] and total==info['uncompressed_file_bytes']
result={'status':'PASS','original_relative_paths_and_contents_preserved':preserved,'zip_crc_sha256_verified_files':checked,'uncompressed_bytes':total,'archive_bytes':new.stat().st_size,'project_outer_folder':'universal-joint-rehab-system','desktop_folder':'腕部驱动设备','extra_regrouping_layers':False,'portable_validation':json.loads((out/'original-layout-portable-validation.json').read_text(encoding='utf-8-sig'))}
(out/'original-layout-verification.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps({key:value for key,value in result.items() if key!='portable_validation'},ensure_ascii=True),flush=True)
