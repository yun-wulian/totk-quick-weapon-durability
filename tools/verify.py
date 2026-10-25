from pathlib import Path
import argparse, hashlib, struct, sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/python-libs'))
import lz4.block
from elftools.elf.elffile import ELFFile

def read_nso(path):
    b = path.read_bytes()
    assert b[:4] == b'NSO0', path
    flags = struct.unpack_from('<I', b, 12)[0]
    image = bytearray()
    regions = []
    for i in range(3):
        offset, address, size = struct.unpack_from('<III', b, 0x10+0x10*i)
        packed = struct.unpack_from('<I', b, 0x60+4*i)[0]
        raw = b[offset:offset+(packed if flags & (1<<i) else size)]
        if flags & (1<<i): raw = lz4.block.decompress(raw, uncompressed_size=size)
        assert len(raw) == size
        if flags & (1 << (3+i)): assert hashlib.sha256(raw).digest() == b[0xa0+i*32:0xc0+i*32]
        image.extend(bytes(max(0, address+size-len(image))))
        image[address:address+size] = raw
        regions.append((address, size))
    return b, image, regions

def exports(path):
    header, image, regions = read_nso(path)
    ro = regions[1][0]
    strings_off, strings_len = struct.unpack_from('<II', header, 0x90)
    symbols_off, symbols_len = struct.unpack_from('<II', header, 0x98)
    strings = image[ro+strings_off:ro+strings_off+strings_len]
    names = set()
    for offset in range(ro+symbols_off, ro+symbols_off+symbols_len, 24):
        name, info, other, section, value, size = struct.unpack_from('<IBBHQQ', image, offset)
        if section and name: names.add(strings[name:strings.index(0,name)].decode())
    return names

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Check the module; optionally check against your own game dump.')
    parser.add_argument('--game-exefs', type=Path, help='Directory containing main.nso, sdk.nso and rtld.nso')
    args = parser.parse_args()
    output = ROOT / 'dist/Quick Weapon Durability/exefs/subsdk6'
    header, image, regions = read_nso(output)
    with (ROOT / 'build/QuickWeaponDurability.elf').open('rb') as f:
        elf = ELFFile(f)
        imported = {s.name for s in elf.get_section_by_name('.dynsym').iter_symbols()
                    if s['st_shndx'] == 'SHN_UNDEF' and s.name and s['st_info']['bind'] != 'STB_WEAK'}
        if args.game_exefs:
            available = set().union(*(exports(args.game_exefs/name) for name in ['main.nso','sdk.nso','rtld.nso']))
            missing = imported - available
            print('Unresolved game imports:', sorted(missing))
            assert not missing
        mod = struct.unpack_from('<I', image, 4)[0]
        assert image[mod:mod+4] == b'MOD0'
        dynamic = mod + struct.unpack_from('<i', image, mod+4)[0]
        tags = {}
        for offset in range(dynamic, len(image), 16):
            tag, value = struct.unpack_from('<QQ', image, offset)
            if tag == 0: break
            tags[tag] = value
        init = elf.get_section_by_name('.symtab').get_symbol_by_name('exl_module_init')[0]['st_value']
        assert tags[12] == init
        assert tags.get(1) is None, 'Unexpected external library dependency'
        bss_start = mod + struct.unpack_from('<i', image, mod+8)[0]
        bss_end = mod + struct.unpack_from('<i', image, mod+12)[0]
        data_end = sum(regions[2])
        bss_alloc = struct.unpack_from('<I', header, 0x3c)[0]
        assert data_end <= bss_start <= bss_end <= data_end+bss_alloc
        symbols = elf.get_section_by_name('.symtab')
        bridge = symbols.get_symbol_by_name('quick_durability_bridge')[0]
        callback = symbols.get_symbol_by_name('quick_durability_set_name')[0]
        address = bridge['st_value']
        assert bridge['st_size'] == 28
        words = struct.unpack_from('<7I', image, address)
        assert words[:4] == (0xa9bf7bfd, 0xaa1303e5, 0xaa1d03e6, 0x910003fd)
        assert words[5:] == (0xa8c17bfd, 0xd65f03c0)
        assert words[4] >> 26 == 0x25 # BL: preserves the return through our bridge.
        displacement = words[4] & 0x3ffffff
        if displacement & 0x2000000: displacement -= 0x4000000
        assert address + 16 + displacement * 4 == callback['st_value']
    print('NSO hashes, segments, MOD0, init and ARM64 bridge verified.')
    if args.game_exefs:
        _, main, _ = read_nso(args.game_exefs/'main.nso')
        assert main[0x41a9f88:0x41a9f9c].hex() == '9b4e43650501a4d4489b4bbfdb740f26af3cf850'
        for offset, expected in [(0x1acd474,0x97c2114f),(0x1acd2f8,0xd10343ff),
                                 (0x1acd300,0x9101c3fd),(0x1acd864,0x910003e1),
                                 (0x1acd888,0xd10083a1),(0xb519b0,0xa9bd7bfd),
                                 (0x882864,0xa9bc7bfd)]:
            assert struct.unpack_from('<I',main,offset)[0] == expected
        print('Game imports and TotK 1.2.1 callsite/frame verified against the supplied dump.')
    else:
        print('Game import and callsite checks skipped; pass --game-exefs to check your own dump.')
