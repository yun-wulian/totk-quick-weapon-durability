// SPDX-License-Identifier: GPL-2.0-only
// Exlaunch framework: MaxLastBreath/TOTK-DFPS, DFPS-1.1.0.
// Native ABI and callsite verified against TotK 1.2.1 main (9B4E4365...).
#include "lib.hpp"
#include "durability.hpp"
#include <cstring>

namespace {
uintptr_t game_base;
constexpr uintptr_t NameCall = 0x01acd474;
constexpr uintptr_t SetMessage = 0x00b519b0;
constexpr uintptr_t GetStructInt = 0x00882864;
constexpr uintptr_t GameDataSlot = 0x0462e3d8;

struct MessageString {
    const char16_t* text;
    uint32_t length;
    uint32_t padding;
    uint64_t message_id;
};
static_assert(sizeof(MessageString) == 24);
static_assert(offsetof(MessageString, message_id) == 16);
using SetName = int(*)(void*, const char*, const MessageString*, uint32_t, void*);
using ReadInt = bool(*)(void*, int32_t*, const void*, uint32_t);

template<class T> const T& Field(const void* p, ptrdiff_t offset) {
    return *reinterpret_cast<const T*>(static_cast<const char*>(p) + offset);
}
template<class Fn> Fn Game(uintptr_t offset) { return reinterpret_cast<Fn>(game_base + offset); }
void Log(const char* message) { svcOutputDebugString(message, std::strlen(message)); }
}

extern "C" void quick_durability_bridge();

extern "C" int quick_durability_set_name(void* layout, const char* pane,
        const MessageString* original, uint32_t update, void* options,
        const void* slide, const void* frame) {
    quick_durability::Label label;
    const auto mode = Field<uint32_t>(slide, 0x88);
    const auto category = Field<uint32_t>(slide, 0x8c);
    const char* actor = Field<const char*>(frame, -0x20);

    auto read = [frame](uint32_t hash, int32_t& value) {
        auto slot = *reinterpret_cast<void***>(game_base + GameDataSlot);
        if (!slot || !*slot) return false;
        // The native selector already resolved ordering, empty slots and the
        // Master Sword. Reuse its selected item handle at original SP (FP-0x70).
        // Do not derive a pouch index from the visible slot number.
        const auto handle = static_cast<const char*>(frame) - 0x70;
        return Game<ReadInt>(GetStructInt)(*slot, &value, handle, hash);
    };
    if (!quick_durability::BuildLabel(mode, category, actor, read, label))
        return Game<SetName>(SetMessage)(layout, pane, original, update, options);

    // LayoutEx synchronously copies text to the textbox. Both this buffer and
    // MessageString remain alive until that native call has returned.
    const MessageString replacement{label.text, label.length, 0, UINT64_MAX};
    return Game<SetName>(SetMessage)(layout, pane, &replacement, update, options);
}

extern "C" void exl_main(void*, void*) {
    game_base = exl::util::modules::GetTargetStart();
    constexpr unsigned char build_id[] = {
        0x9b,0x4e,0x43,0x65,0x05,0x01,0xa4,0xd4,0x48,0x9b,
        0x4b,0xbf,0xdb,0x74,0x0f,0x26,0xaf,0x3c,0xf8,0x50};
    if (exl::util::GetMainModuleInfo().m_Total.m_Size < GameDataSlot + sizeof(void*) ||
        std::memcmp(reinterpret_cast<void*>(game_base + 0x041a9f88), build_id, sizeof(build_id))) {
        Log("[QuickWeaponDurability] Only TotK 1.2.1 is supported; disabled\n");
        return;
    }
    // Verify the patch and the frame/handle ABI we read, including other mods'
    // potential edits. The global naming and inventory functions stay intact.
    constexpr uintptr_t offsets[] = {NameCall, 0x01acd2f8, 0x01acd300,
                                    0x01acd864, 0x01acd888, SetMessage, GetStructInt};
    constexpr uint32_t instructions[] = {0x97c2114f, 0xd10343ff, 0x9101c3fd,
                                         0x910003e1, 0xd10083a1, 0xa9bd7bfd, 0xa9bc7bfd};
    for (unsigned i = 0; i < sizeof(offsets) / sizeof(offsets[0]); ++i) {
        if (*reinterpret_cast<const uint32_t*>(game_base + offsets[i]) != instructions[i]) {
            Log("[QuickWeaponDurability] Executable or callsite conflict; disabled\n");
            return;
        }
    }
    const auto displacement = reinterpret_cast<intptr_t>(&quick_durability_bridge)
                            - intptr_t(game_base + NameCall);
    if ((displacement & 3) || displacement < -(1LL << 27) || displacement >= (1LL << 27)) {
        Log("[QuickWeaponDurability] Module outside ARM64 BL range; disabled\n");
        return;
    }
    {
        exl::patch::CodePatcher patch(NameCall);
        patch.BranchLinkInst(reinterpret_cast<void*>(&quick_durability_bridge));
    }
    Log("[QuickWeaponDurability] 1.2.1 initialized; melee shortcut caption enabled\n");
}

extern "C" NORETURN void exl_exception_entry() {
    EXL_ABORT(0xffff, "QuickWeaponDurability exception");
}
