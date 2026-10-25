// SPDX-License-Identifier: GPL-2.0-only
#include "durability.hpp"
#include <cassert>
#include <string>
#include <cstdio>
using namespace quick_durability;

int main() {
    int reads = 0;
    auto finite = [&](uint32_t hash, int32_t& out) {
        ++reads;
        if (hash == 0x0355c603) { out = 17; return true; }
        if (hash == 0x054dedbe) { out = 8; return true; }
        return false;
    };
    Label label;
    assert(BuildLabel(0, 0, "Weapon_Sword_001", finite, label));
    assert(std::u16string(label.text) == u"25" && label.length == 2 && reads == 2);

    // Bow/shield/material/ability lists and the unequip slot must never read
    // a weapon record or replace their native captions.
    reads = 0;
    for (unsigned category : {1u, 2u, 3u, 4u, 5u})
        assert(!BuildLabel(0, category, "Item", finite, label));
    assert(!BuildLabel(1, 0, "Item", finite, label));
    assert(!BuildLabel(0, 0, "", finite, label));
    assert(!BuildLabel(0, 0, nullptr, finite, label));
    assert(reads == 0);

    auto check = [&](int life, int extra, const char16_t* expected) {
        auto read = [&](uint32_t hash, int32_t& out) {
            out = hash == 0x0355c603 ? life : extra;
            return true;
        };
        assert(BuildLabel(0, 0, "Weapon", read, label));
        assert(std::u16string(label.text) == expected);
        assert(std::char_traits<char16_t>::length(label.text) == label.length);
    };
    check(30, 0, u"30");  // Unfused.
    check(30, 25, u"55"); // Fresh fuse bonus.
    check(30, 24, u"54"); // One point consumed from bonus.
    check(29, 0, u"29");  // Bonus exhausted; base durability consumed.
    check(0, 0, u"0");
    check(INT32_MAX, INT32_MAX, u"4294967294");
    auto unavailable = [](uint32_t, int32_t&) { return false; };
    assert(!BuildLabel(0, 0, "Weapon", unavailable, label));
    auto missing_extra = [](uint32_t hash, int32_t& out) {
        out = 30; return hash == 0x0355c603;
    };
    assert(!BuildLabel(0, 0, "Weapon", missing_extra, label));
    auto sentinel = [](uint32_t, int32_t& out) { out = -1; return true; };
    assert(!BuildLabel(0, 0, "Weapon", sentinel, label));
    std::puts("PASS: scope, unequip slot, fused/unfused wear, zero, overflow, unavailable values");
}
