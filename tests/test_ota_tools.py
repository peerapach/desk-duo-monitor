import importlib.util
import pathlib
import struct
import tempfile
import unittest
import sys
import contextlib
import io
from unittest.mock import patch

ROOT = pathlib.Path(__file__).resolve().parents[1]
def module(name):
    spec = importlib.util.spec_from_file_location(name, ROOT / 'scripts' / (name + '.py'))
    obj = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(obj)
    return obj
package = module('package_firmware')
sys.path.insert(0,str(ROOT/'scripts'))

def application():
    data=bytearray(24);data[0]=0xe9;data[1]=1
    data+=struct.pack('<II',0x3f400020,256)
    desc=bytearray(256);struct.pack_into('<I',desc,0,0xabcd5432);desc[48:57]=b'desk_duo\0'
    data+=desc
    checksum=0xef
    for byte in desc:checksum^=byte
    data+=bytes(15)+bytes([checksum])
    return bytes(data)

def fixture(directory):
    pieces=[(0x1000,'bootloader.bin',b'\xe9'+bytes(100)),(0x8000,'partitions.bin',table()),(0x20000,'ota_data_initial.bin',b'\xff'*8192),(0x30000,'desk-duo-app.bin',application())]
    merged=bytearray(b'\xff'*(0x30000+len(application())))
    segments=[]
    for offset,name,data in pieces:
        (directory/name).write_bytes(data);merged[offset:offset+len(data)]=data
        segments.append(dict(offset=hex(offset),file=name,size=len(data),sha256=package.digest(data)))
    (directory/'desk-duo-cyd.bin').write_bytes(merged)
    metadata=dict(formatVersion=2,chip='esp32',flashBytes=4194304,image='desk-duo-cyd.bin',size=len(merged),sha256=package.digest(merged),segments=segments)
    import json
    (directory/'desk-duo-cyd.json').write_text(json.dumps(metadata))
    return metadata

def table(changes=None):
    entries=[('nvs',1,2,0x9000,0x16000),('phy_init',1,1,0x1f000,0x1000),('otadata',1,0,0x20000,0x2000),('ota_0',0,16,0x30000,0x1e0000),('ota_1',0,17,0x210000,0x1e0000)]
    if changes: entries[changes[0]]=changes[1]
    return b''.join(struct.pack('<HBBII16sI',0x50aa,t,s,o,n,label.encode(),0) for label,t,s,o,n in entries)+b'\xff'*32

class Packaging(unittest.TestCase):
    def test_actual_partition_format(self):
        self.assertTrue(hasattr(package,'validate_partitions'), 'OTA partition validator is missing')
        self.assertEqual(package.validate_partitions(table(),0x30000,1500000),0x1e0000)
    def test_rejects_unsafe_geometry(self):
        self.assertTrue(hasattr(package,'validate_partitions'), 'OTA partition validator is missing')
        for index,entry in [(0,('nvs',1,2,0xa000,0x15000)),(1,('phy_init',1,1,0x1e000,0x1000)),(3,('ota_0',0,16,0x20000,0x1e0000)),(4,('ota_1',0,17,0x310000,0x1e0000)),(4,('ota_1',0,17,0x210000,0x100000))]:
            with self.assertRaises(ValueError):package.validate_partitions(table((index,entry)),0x30000,1500000)
        for offset,size in [(0x20000,1500000),(0x30000,0x1e0001)]:
            with self.assertRaises(ValueError):package.validate_partitions(table(),offset,size)
    def test_merged_is_not_application(self):
        self.assertTrue(hasattr(package,'validate_application'), 'Application identity validator is missing')
        with self.assertRaises(ValueError):package.validate_application(b'\xff'*10000)
    def test_valid_image_and_corruption(self):
        package.validate_application(application())
        for bad in (application()[:-1],application()+b'junk',application()[:80]+b'otherapp\0'+application()[89:]):
            with self.assertRaises(ValueError):package.validate_application(bad)
    def test_package_segments_and_padding(self):
        with tempfile.TemporaryDirectory() as tmp:
            directory=pathlib.Path(tmp);meta=fixture(directory)
            self.assertEqual([p[0] for p in package.verify_package(directory,meta)],[0x1000,0x8000,0x20000,0x30000])
            meta['segments'].pop(2)
            with self.assertRaises(ValueError):package.verify_package(directory,meta)
            meta=fixture(directory);meta['segments'][0]['offset']='0x9000'
            with self.assertRaises(ValueError):package.verify_package(directory,meta)
            meta=fixture(directory);image=bytearray((directory/'desk-duo-cyd.bin').read_bytes());image[0x9000]=0
            (directory/'desk-duo-cyd.bin').write_bytes(image);meta['sha256']=package.digest(image)
            with self.assertRaises(ValueError):package.verify_package(directory,meta)
    def test_probe_cannot_be_packaged_as_release(self):
        with tempfile.TemporaryDirectory() as tmp:
            directory=pathlib.Path(tmp);meta=fixture(directory)
            data=bytearray(application());data[48:62]=b'rollback-probe'
            checksum=0xef
            for byte in data[32:288]:checksum^=byte
            data[-1]=checksum
            (directory/'desk-duo-app.bin').write_bytes(data)
            meta['segments'][-1]['sha256']=package.digest(data)
            with self.assertRaises(ValueError):package.verify_segments(directory,meta['segments'])


class Migration(unittest.TestCase):
    def test_migration_exists(self):
        self.assertTrue((ROOT/'scripts/migrate_ota.py').exists(),'Dry-run migration tool is missing')
    def run_migration(self,flash=False,skip=False,backup_failure=False):
        migration=module('migrate_ota')
        with tempfile.TemporaryDirectory() as tmp:
            root=pathlib.Path(tmp);fixture(root);calls=[]
            def run(command):
                calls.append(command)
                if 'read_flash' in command:
                    if backup_failure:raise RuntimeError('read failure')
                    pathlib.Path(command[-1]).write_bytes(b'\xff'*4194304)
                if 'write_flash' in command and not skip:
                    backups=list((root/'backups').glob('*/flash.bin'))
                    self.assertEqual(len(backups),1)
                    self.assertEqual(backups[0].stat().st_size,4194304)
                    self.assertEqual(backups[0].stat().st_mode & 0o777,0o600)
                    self.assertEqual(backups[0].parent.stat().st_mode & 0o777,0o700)
                    self.assertTrue((backups[0].parent/'recovery.json').is_file())
                return 'Chip is ESP32-D0WD\nDetected flash size: 4MB\n'
            args=['--package',str(root),'--backup-dir',str(root/'backups')]
            if flash:args+=['--flash','--port','/dev/test-only']
            if skip:args+=['--skip-backup']
            if backup_failure:
                with self.assertRaises(RuntimeError), contextlib.redirect_stdout(io.StringIO()):migration.main(args,run=run)
                self.assertFalse(any('write_flash' in c for c in calls));return
            with contextlib.redirect_stdout(io.StringIO()):migration.main(args,run=run)
            if not flash:self.assertEqual(calls,[]);return
            self.assertEqual(any('read_flash' in c for c in calls),not skip)
            writes=[c for c in calls if 'write_flash' in c];self.assertEqual(len(writes),1)
            index=writes[0].index('4MB')+1
            self.assertEqual(writes[0][index::2],['0x1000','0x8000','0x20000','0x30000'])
    def test_dry_run_no_device(self):self.run_migration()
    def test_backup_before_segmented_writes(self):self.run_migration(flash=True)
    def test_explicit_skip_makes_no_backup(self):self.run_migration(flash=True,skip=True)
    def test_failed_backup_never_writes(self):self.run_migration(flash=True,backup_failure=True)

if __name__=='__main__':unittest.main()
