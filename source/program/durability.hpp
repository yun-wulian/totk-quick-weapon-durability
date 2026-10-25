// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <cstddef>
#include <cstdint>

namespace quick_durability {
struct Label {
    char16_t text[32]{};
    uint32_t length{};
};

// UIShortCutSlideListPartsScreen: ordinary pouch list, melee weapon category.
constexpr bool IsWeaponEntry(uint32_t mode, uint32_t category, const char* actor) {
    return mode == 0 && category == 0 && actor && actor[0];
}

template<class ReadInt>
bool BuildLabel(uint32_t mode, uint32_t category, const char* actor,
                ReadInt read, Label& label) {
    if (!IsWeaponEntry(mode, category, actor)) return false;
    int32_t life, extra;
    if (!read(0x0355c603, life) || !read(0x054dedbe, extra)) return false;
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
