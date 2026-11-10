// Drumline page 2: a second page on stock Drumline with each key's FILTER, SRR,
// FX I and FX II, and their step locks. The SDK keeps the page, its storage,
// locks and sends (extension.json's drumline_page); this supplies the sound.
#include <opxy/extension.hpp>
#include "page_dsp.hpp"

namespace opxy { namespace extensions { namespace drumline_page {
namespace {
void start(drumline::Voice&, const drumline::Choices&) {}
void process(drumline::Voice& voice, const drumline::Choices& choices, i32* block) {
    process_block(voice, choices, block, drumline::frames);
}
}
// No events: the page is the whole extension.
extern constexpr Hooks hooks = Hooks();
extern const drumline::Processor drumline_processor = {start, process};
}}} // namespace opxy::extensions::drumline_page
