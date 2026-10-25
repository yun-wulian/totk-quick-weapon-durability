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

    // Bow records have no ExtraLife field: only their own Life may be read.
    reads = 0;
    auto bow = [&](uint32_t hash, int32_t& out) {
        ++reads;
        if (hash != 0x0355c603) return false;
        out = 43;
        return true;
    };
    assert(BuildLabel(0, 1, "Weapon_Bow_001", bow, label));
    assert(std::u16string(label.text) == u"43" && label.length == 2 && reads == 1);

    reads = 0;
    assert(BuildLabel(0, 3, "Weapon_Shield_001", finite, label));
    assert(std::u16string(label.text) == u"25" && reads == 2);

    // Non-equipment lists and unequip slots keep their native captions.
    reads = 0;
    for (unsigned category : {2u, 4u, 5u, 6u})
        assert(!BuildLabel(0, category, "Item", finite, label));
    for (unsigned category : {0u, 1u, 3u}) {
        assert(!BuildLabel(1, category, "Item", finite, label));
        assert(!BuildLabel(0, category, "", finite, label));
        assert(!BuildLabel(0, category, nullptr, finite, label));
    }
    assert(reads == 0);

    auto check = [&](unsigned category, int life, int extra, const char16_t* expected) {
        auto read = [&](uint32_t hash, int32_t& out) {
            out = hash == 0x0355c603 ? life : extra;
            return true;
        };
        assert(BuildLabel(0, category, "Equipment", read, label));
        assert(std::u16string(label.text) == expected);
        assert(std::char_traits<char16_t>::length(label.text) == label.length);
    };
    for (unsigned category : {0u, 3u}) {
        check(category, 30, 0, u"30");  // Unfused.
        check(category, 30, 25, u"55"); // Fresh fuse bonus.
        check(category, 30, 24, u"54"); // One point consumed from bonus.
        check(category, 29, 0, u"29");  // Bonus exhausted; base durability consumed.
        check(category, 0, 0, u"0");
        check(category, INT32_MAX, INT32_MAX, u"4294967294");
    }
    check(1, 43, 0, u"43");
    check(1, 42, 0, u"42"); // One bow durability point consumed.
    check(1, 0, 0, u"0");
    check(1, INT32_MAX, 0, u"2147483647");
    auto unavailable = [](uint32_t, int32_t&) { return false; };
    auto missing_extra = [](uint32_t hash, int32_t& out) {
        out = 30; return hash == 0x0355c603;
    };
    for (unsigned category : {0u, 3u})
        assert(!BuildLabel(0, category, "Equipment", missing_extra, label));
    auto sentinel = [](uint32_t, int32_t& out) { out = -1; return true; };
    for (unsigned category : {0u, 1u, 3u}) {
        assert(!BuildLabel(0, category, "Equipment", unavailable, label));
        assert(!BuildLabel(0, category, "Equipment", sentinel, label));
    }
    std::puts("PASS: weapons, bows without ExtraLife, shields, excluded lists, unequip slots, wear and boundaries");
}
