// Drumline page 2's sound (page_dsp.hpp): SRR's hold, FILTER's response and
// range, and that state carries across blocks. Returns 0 when every check passes.
#include <cstdio>
#include <cstdlib>
#include "page_dsp.hpp"

using namespace opxy;
using namespace opxy::extensions::drumline_page;

#define CHECK(condition) do { if (!(condition)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #condition); std::exit(1); } } while (0)

namespace {
const u32 kFrames = drumline::frames;
const i32 kLevel = 1 << 29;  // a quarter of full scale

drumline::Choices choices(u32 filter, u32 srr) {
    drumline::Choices c = {{static_cast<u8>(filter), static_cast<u8>(srr), 0, 0}};
    return c;
}
drumline::Voice fresh() {
    drumline::Voice v;
    for (u32 i = 0; i < drumline::voice_words; ++i) v.word[i] = 0;
    return v;
}
i32 magnitude(i32 x) { return x < 0 ? -x : x; }

void srr_holds_samples_across_blocks() {
    // The table: 1 at OFF, then 2..64, rising.
    CHECK(srr_hold(0) == 1 && srr_hold(1) == 2 && srr_hold(15) == 64);
    for (u32 s = 1; s < 16; ++s) CHECK(srr_hold(s) > srr_hold(s - 1));
    for (u32 srr = 1; srr < 16; ++srr) {
        drumline::Voice v = fresh();
        const drumline::Choices c = choices(kFilterOpen, srr);
        const u32 hold = srr_hold(srr);
        for (u32 block = 0; block < 4; ++block) {
            i32 out[kFrames];
            for (u32 i = 0; i < kFrames; ++i) out[i] = static_cast<i32>(block * kFrames + i) * 1000 + 1;
            process_block(v, c, out, kFrames);
            for (u32 i = 0; i < kFrames; ++i) {
                const u32 n = block * kFrames + i;
                CHECK(out[i] == static_cast<i32>(n - n % hold) * 1000 + 1);
            }
        }
    }
}

void open_and_off_leave_the_block_alone() {
    drumline::Voice v = fresh();
    i32 out[kFrames];
    for (u32 i = 0; i < kFrames; ++i) out[i] = static_cast<i32>(i * 7919u) - 200000;
    i32 before[kFrames];
    for (u32 i = 0; i < kFrames; ++i) before[i] = out[i];
    process_block(v, choices(kFilterOpen, 0), out, kFrames);
    for (u32 i = 0; i < kFrames; ++i) CHECK(out[i] == before[i]);
}

// The filter's settled output for a constant input, and the largest it gives an
// alternating (Nyquist) input, after `blocks` blocks.
void settle(u32 filter, i32& dc, i32& nyquist) {
    drumline::Voice a = fresh(), b = fresh();
    const drumline::Choices c = choices(filter, 0);
    i32 out[kFrames];
    for (u32 block = 0; block < 200; ++block) {
        for (u32 i = 0; i < kFrames; ++i) out[i] = kLevel;
        process_block(a, c, out, kFrames);
    }
    dc = out[kFrames - 1];
    nyquist = 0;
    for (u32 block = 0; block < 200; ++block) {
        for (u32 i = 0; i < kFrames; ++i) out[i] = i & 1 ? -kLevel : kLevel;
        process_block(b, c, out, kFrames);
        if (block >= 190)
            for (u32 i = 0; i < kFrames; ++i) if (magnitude(out[i]) > nyquist) nyquist = magnitude(out[i]);
    }
}

void low_pass_passes_dc_and_high_pass_blocks_it() {
    for (u32 filter = 0; filter < 32; ++filter) {
        if (filter == kFilterOpen) continue;
        i32 dc, nyquist;
        settle(filter, dc, nyquist);
        if (filter < kFilterOpen) {
            // Low-pass: a constant comes through at a steady level; the top octave doesn't.
            CHECK(dc > kLevel / 2 && dc < kLevel + kLevel / 4);
            CHECK(nyquist < kLevel / 4);
        } else {
            // High-pass: a constant dies away; the top octave comes through.
            CHECK(magnitude(dc) < kLevel / 100);
            CHECK(nyquist > kLevel / 2);
        }
    }
}

void full_scale_stays_in_range() {
    const i32 ceiling = kFilterLimit * 16;
    for (u32 filter = 0; filter < 32; ++filter) {
        if (filter == kFilterOpen) continue;
        drumline::Voice v = fresh();
        const drumline::Choices c = choices(filter, 0);
        for (u32 block = 0; block < 64; ++block) {
            i32 out[kFrames];
            for (u32 i = 0; i < kFrames; ++i) out[i] = (i / 8 + block) & 1 ? q31::maximum : -q31::maximum;
            process_block(v, c, out, kFrames);
            for (u32 i = 0; i < kFrames; ++i) CHECK(out[i] >= -ceiling && out[i] <= ceiling);
        }
    }
}

void state_carries_between_blocks() {
    // Two half blocks give what one whole block gives, for every choice pair.
    for (u32 filter = 0; filter < 32; filter += 5) {
        for (u32 srr = 0; srr < 16; srr += 3) {
            drumline::Voice whole = fresh(), halves = fresh();
            const drumline::Choices c = choices(filter, srr);
            i32 a[kFrames], b[kFrames];
            for (u32 round = 0; round < 3; ++round) {
                for (u32 i = 0; i < kFrames; ++i) a[i] = b[i] = static_cast<i32>((i * 2654435761u + round * 40503u) >> 3) - (1 << 28);
                process_block(whole, c, a, kFrames);
                process_block(halves, c, b, kFrames / 2);
                process_block(halves, c, b + kFrames / 2, kFrames / 2);
                for (u32 i = 0; i < kFrames; ++i) CHECK(a[i] == b[i]);
            }
        }
    }
}
}

int main() {
    srr_holds_samples_across_blocks();
    open_and_off_leave_the_block_alone();
    low_pass_passes_dc_and_high_pass_blocks_it();
    full_scale_stays_in_range();
    state_carries_between_blocks();
    // The processor the SDK calls runs the same block.
    drumline::Voice v = fresh();
    i32 out[kFrames];
    for (u32 i = 0; i < kFrames; ++i) out[i] = static_cast<i32>(i) * 4096;
    process_block(v, choices(kFilterOpen, 3), out, kFrames);
    CHECK(out[5] == 4 * 4096 && out[7] == 4 * 4096 && out[8] == 8 * 4096);
    std::puts("drumline_page ok");
    return 0;
}
