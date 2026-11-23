# Unicode emoji catalog inputs

Pinned Unicode 16.0 data, preserved with their upstream notices:

- [emoji-test.txt](https://www.unicode.org/Public/emoji/16.0/emoji-test.txt)
- [GraphemeBreakProperty.txt](https://www.unicode.org/Public/16.0.0/ucd/auxiliary/GraphemeBreakProperty.txt)
- [emoji-data.txt](https://www.unicode.org/Public/16.0.0/ucd/emoji/emoji-data.txt)

License: [Unicode License V3](../../assets/Unicode-LICENSE.txt), also packaged in the VPK.

CMake runs `scripts/build-emoji-catalog.py` with the exact font selected for the
VPK. The host HarfBuzz shared library resolves each sequence to a glyph; only
sequences producing one nonzero glyph enter the generated table. Unicode aliases
are included for rendering; only fully qualified sequences enter the picker.
Skin-tone combinations share a base choice. The resulting glyph indices belong
to this font and must be regenerated when the packaged font changes.
No Unicode files or shaping library are loaded from the network or needed on Vita.
