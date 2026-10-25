from pathlib import Path
import argparse, concurrent.futures, hashlib, os, struct, subprocess, sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/python-libs'))
from elftools.elf.elffile import ELFFile
import lz4.block

parser = argparse.ArgumentParser(description='Build the TotK 1.2.1 durability module with Zig 0.14.1.')
parser.add_argument('--zig', default=os.environ.get('ZIG', 'zig'), help='Path to Zig 0.14.1')
ZIG = parser.parse_args().zig
BUILD = ROOT / 'build'
BUILD.mkdir(exist_ok=True)
base = ['-target', 'aarch64-linux-musl', '-O2', '-ffreestanding', '-fPIC',
        f'-ffile-prefix-map={ROOT}=.',
        '-fvisibility=hidden', '-fno-stack-protector', '-mno-outline-atomics',
        '-ffunction-sections', '-fdata-sections', '-D__SWITCH__', '-D__RTLD_6XX__',
        '-DEXL_LOAD_KIND=Module', '-DEXL_LOAD_KIND_ENUM=2', '-DEXL_PROGRAM_ID=0x0100F2C0115B6000']
for path in ['source', 'source/lib', 'source/nn', 'source/program']:
    base += ['-I', str(ROOT / path)]

sources = sorted(p for p in (ROOT / 'source').rglob('*')
                 if p.suffix in ('.cpp', '.c', '.s'))

def compile(p):
    obj = BUILD / (str(p.relative_to(ROOT / 'source')).replace('\\', '_').replace('/', '_') + '.o')
    cpp = p.suffix == '.cpp'
    args = [str(ZIG), 'c++' if cpp else 'cc', *base]
    if cpp: args += ['-std=c++20', '-fno-rtti', '-fno-exceptions', '-fno-unwind-tables', '-fno-asynchronous-unwind-tables']
    if p.suffix == '.s': args += ['-x', 'assembler-with-cpp']
    args += ['-c', str(p), '-o', str(obj)]
    result = subprocess.run(args, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(str(p.relative_to(ROOT)) + '\n' + result.stdout + result.stderr)
    return obj

objects, errors = [], []
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
    futures = [pool.submit(compile, p) for p in sources]
    for future in futures:
        try: objects.append(future.result())
        except RuntimeError as error: errors.append(str(error))
if errors:
    print('\n'.join(errors), file=sys.stderr)
    raise SystemExit(1)
print(f'Compiled {len(objects)} source files')
elf = BUILD / 'QuickWeaponDurability.elf'
command = [str(ZIG), 'ld.lld', '-m', 'aarch64elf', '-shared', '--gc-sections',
           '--version-script=' + str(ROOT / 'tools/exports.map'), '--build-id=sha1', '-init', 'exl_module_init',
           '-T', str(ROOT / 'tools/link.ld'), '-Map', str(BUILD / 'QuickWeaponDurability.map'),
           *map(str, objects), '-o', str(elf)]
subprocess.run(command, check=True)

with elf.open('rb') as f:
    e = ELFFile(f)
    segments = [s for s in e.iter_segments() if s['p_type'] == 'PT_LOAD']
    assert len(segments) == 3 and [s['p_flags'] for s in segments] == [5, 4, 6]
    assert segments[0]['p_vaddr'] == 0
    header = bytearray(0x100)
    header[:4] = b'NSO0'
    struct.pack_into('<I', header, 0xc, 0x3f)
    build_id = hashlib.sha256(elf.read_bytes()).digest()
    header[0x40:0x60] = build_id
    body = bytearray()
    for i, seg in enumerate(segments):
        raw = seg.data()
        packed = lz4.block.compress(raw, store_size=False)
        struct.pack_into('<III', header, 0x10 + 0x10*i, len(body) + 0x100, seg['p_vaddr'], len(raw))
        struct.pack_into('<I', header, 0x60 + 4*i, len(packed))
        header[0xa0 + 0x20*i:0xc0 + 0x20*i] = hashlib.sha256(raw).digest()
        body += packed
    struct.pack_into('<I', header, 0x3c, segments[2]['p_memsz'] - segments[2]['p_filesz'])
    ro_base = segments[1]['p_vaddr']
    for section, offset in [('.dynstr', 0x90), ('.dynsym', 0x98)]:
        s = e.get_section_by_name(section)
        struct.pack_into('<II', header, offset, s['sh_addr'] - ro_base, s['sh_size'])
    dynamic = e.get_section_by_name('.dynsym')
    imports = [s.name for s in dynamic.iter_symbols() if s['st_shndx'] == 'SHN_UNDEF' and s.name]
    print('Dynamic imports:', imports)
    output = ROOT / 'dist/Quick Weapon Durability/exefs/subsdk6'
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(header + body)
    print(f'Built {output} ({output.stat().st_size} bytes)')
