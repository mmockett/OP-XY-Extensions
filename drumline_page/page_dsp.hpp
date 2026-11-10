// Drumline page 2's sound: SRR, then FILTER, on one stock Drumline voice's mono
// block. Both are MD KIT's pad effects (OP-XY-MNMD, machines/mdkit), written from
// scratch. Choices are the indexes of extension.json's lists.
#pragma once
#include <opxy/drumline_page.hpp>
#include <opxy/q31.hpp>

namespace opxy { namespace extensions { namespace drumline_page {
// The parameters, in extension.json's order.
enum Parameter { kFilter, kSrr, kFx1, kFx2 };
const u32 kFilterOpen = 16;  // FILTER's default: choices 0..15 low-pass, 17..31 high-pass

// SRR: each held sample stands for this many, as a lower sample rate would; the
// captions are 44.1 kHz over the hold.
// The table sits at file scope: a function-local static in an inline function is
// a Blackfin link hazard.
const u8 kSrrHolds[16] = {1, 2, 3, 4, 5, 6, 8, 10, 12, 16, 20, 24, 32, 40, 48, 64};
inline u32 srr_hold(u32 srr) { return kSrrHolds[srr & 15u]; }
// One block held: every hold-th sample stands until the next, `left` frames of
// the current hold carried between blocks.
inline void hold_block(i32* out, u32 frames, u32 hold, i32& held, i32& left) {
    i32 value = held;
    i32 remaining = left;
    for (u32 i = 0; i < frames; ++i) {
        if (remaining <= 0) {
            value = out[i];
            remaining = static_cast<i32>(hold);
        }
        --remaining;
        out[i] = value;
    }
    held = value;
    left = remaining;
}

// FILTER: a state-variable filter at one resonance, low-pass below choice 16 and
// high-pass above. Its coefficients a1, a2, a3 (Q31) by choice (16, open, unused),
// half its k = 1 / Q, and the input's share, under a sixteenth so the states stay
// in range; the output is given back sixteen times over.
const i32 kFilterCoefficients[32][3] = {
    {2128288038, 22743090, 243035}, {2122433141, 29566363, 411872}, {2114782444, 38404531, 697428},
    {2104779036, 49829889, 1179705}, {2091690046, 64561071, 1992710}, {2074549919, 83488560, 3359929},
    {2052085841, 107695064, 5651921}, {2022621153, 138460806, 9478490}, {1983953855, 177235797, 15833296},
    {1933211607, 225548624, 26314854}, {1866695103, 284803101, 43452627}, {1779741995, 355890268, 71166429},
    {1666676100, 438516317, 115377283}, {1520947531, 530118894, 184770372}, {1335604780, 624194313, 291716941},
    {1104279013, 707657675, 453489906}, {0, 0, 0}, {2141101139, 7626413, 27165},
    {2138751322, 10417896, 50746}, {2135534394, 14225393, 94759}, {2131128727, 19413685, 176850},
    {2125092072, 26474118, 329811}, {2116815364, 36064613, 614440}, {2105458249, 49058752, 1143106},
    {2089859025, 66602455, 2122577}, {2068409103, 90172191, 3931052}, {2038879717, 121619212, 7254588},
    {1998188157, 163166183, 13323672}, {1942097355, 217289978, 24311312}, {1864866263, 286370497, 43975304},
    {1758925876, 371906456, 78635725}, {1614762720, 473001421, 138553077}};
const i32 kFilterHalfK = 894784853, kFilterFeed = 103021760;
// Stock Drumline plays above half scale, so the filter stops only short of full scale.
const i32 kFilterLimit = q31::maximum >> 4;
inline i32 filter_limit(i32 value) { return value < -kFilterLimit ? -kFilterLimit : value > kFilterLimit ? kFilterLimit : value; }
inline void filter_block(i32* out, u32 frames, u32 choice, i32& ic1_state, i32& ic2_state) {
    using q31::multiply;
    const i32* c = kFilterCoefficients[choice & 31u];
    const i32 a1 = c[0], a2 = c[1], a3 = c[2];
    const bool high = choice > kFilterOpen;
    i32 ic1 = ic1_state, ic2 = ic2_state;
    for (u32 i = 0; i < frames; ++i) {
        const i32 v0 = multiply(out[i], kFilterFeed), v3 = v0 - ic2;
        const i32 v1 = multiply(a1, ic1) + multiply(a2, v3);
        const i32 v2 = ic2 + multiply(a2, ic1) + multiply(a3, v3);
        ic1 = 2 * v1 - ic1;
        ic2 = 2 * v2 - ic2;
        out[i] = filter_limit(high ? v0 - 2 * multiply(kFilterHalfK, v1) - v2 : v2) * 16;
    }
    ic1_state = ic1;
    ic2_state = ic2;
}

// A voice's state words: SRR's held sample and frames left, FILTER's two states.
enum StateWord { kHeld, kLeft, kIc1, kIc2 };

inline void process_block(drumline::Voice& voice, const drumline::Choices& choices, i32* block, u32 frames) {
    if (const u32 srr = choices.choice[kSrr])
        hold_block(block, frames, srr_hold(srr), voice.word[kHeld], voice.word[kLeft]);
    if (choices.choice[kFilter] != kFilterOpen)
        filter_block(block, frames, choices.choice[kFilter], voice.word[kIc1], voice.word[kIc2]);
}
}}} // namespace opxy::extensions::drumline_page
