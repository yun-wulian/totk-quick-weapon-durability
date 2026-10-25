// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <cstddef>
#include <cstdint>

namespace quick_durability {
struct Label {
    char16_t text[32]{};
    uint32_t length{};
};

// UIShortCutSlideListPartsScreen: melee weapons (0), bows (1), shields (3).
constexpr bool IsEquipmentEntry(uint32_t mode, uint32_t category, const char* actor) {
    return mode == 0 && (category == 0 || category == 1 || category == 3)
           && actor && actor[0];
}

template<class ReadInt>
bool BuildLabel(uint32_t mode, uint32_t category, const char* actor,
                ReadInt read, Label& label) {
    if (!IsEquipmentEntry(mode, category, actor)) return false;
    int32_t life, extra = 0;
    if (!read(0x0355c603, life)) return false;
    // The bow pouch stores Life only. Weapons and shields also store ExtraLife.
    if (category != 1 && !read(0x054dedbe, extra)) return false;
    // Negative values are not a finite durability; retain the native caption.
    if (life < 0 || extra < 0) return false;
    uint64_t remaining = uint64_t(life) + uint64_t(extra);
    label.length = 0;
    char16_t digits[10]; // Two nonnegative int32 fields sum to at most 4294967294.
    unsigned count = 0;
    do {
        digits[count++] = char16_t(u'0' + remaining % 10);
        remaining /= 10;
    } while (remaining);
    while (count) label.text[label.length++] = digits[--count];
    label.text[label.length] = 0;
    return true;
}
}
