# OP-XY Extensions

Firmware extensions for the teenage engineering OP-XY, built with the
community [Open-XY SDK](https://github.com/etcetc000/open-xy). Extensions change
how the OP-XY itself behaves rather than adding a new instrument. Choose them in
the [open-xy.com](https://open-xy.com) builder's Extensions step, or build them
into your own firmware with `sdk.py --extension`.

## Drumline page 2

Stock Drumline gives each key a type, four sound controls, and pan and volume.
Drumline page 2 adds a second page to every key, with its own filter, sample
rate and effect sends, so one Drumline track can hold a dark, crunchy kick, a
bright open hat and a snare drowned in reverb.

**Using it.** On a Drumline track's engine page, press any encoder to switch to
page 2. Press again to go back. Page 2 edits the same key as page 1: the key you
last played, or the one you hold.

| Encoder | Control | Range |
| --- | --- | --- |
| 1 | FILTER | Low-pass from `L150` up to `L8.0k`, `OPEN`, then high-pass from `H50` up to `H4.0k` |
| 2 | SRR | `OFF`, then sample-rate reduction from `22k` down to `689` |
| 3 | FX I | `TRK` follows the track's FX I send; `0` to `100` sets this key's own amount |
| 4 | FX II | The same for FX II |

- **Per hit.** Each hit keeps its key's settings while it rings. Changing the key
  mid-tail affects the next hit, not the one already sounding.
- **Step locks.** Hold a step and turn a page-2 encoder to lock that setting for
  that key on that step. Other keys on the step keep their own. Turn it back to
  the key's own value to remove the lock.
- **Shift** keeps stock's row on both pages: TYPE, RANDOM, PAN and VOLUME.
- **Saved with the project.** Page 2's settings live in each key and are saved,
  copied and pasted with it. A project opened on firmware without the extension
  plays as plain stock Drumline.

**Limits.** It applies to every Drumline track in the firmware. Stock Drumline
presets don't keep page 2's settings.

With every key at `OPEN`, `OFF` and `TRK`, Drumline sounds exactly as stock.

## Building

Each extension is a folder here (`drumline_page/`). To build one into your own
firmware with the SDK:

```
python ../open-xy/sdk.py build --machine <your machine> --extension drumline_page
```

Check it with `python ../open-xy/sdk.py test --extension drumline_page`.

## Licence

MIT. See [LICENSE](LICENSE).
