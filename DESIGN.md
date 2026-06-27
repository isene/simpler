# Simpler — Design Guide

The visual language follows the spoken one: warm, plain, unhurried,
nothing decorative that does not earn its place. Orange for warmth and
energy, serif type for calm and readability.

## Logo

The mark is an italic serif **s** with a single dot to its lower right:
`s.` The dot is the heart of the language, the `.` of every message
send, and the full stop that says "this is simple, we can stop here."

- File: `logo.svg` (scales to any size; convert to PDF/PNG as needed).
- Clear space: keep at least the width of the dot on every side.
- Minimum size: 24 px tall on screen, 8 mm in print.
- Backgrounds: the flame tile works on light or dark. On busy
  backgrounds, place it on a plain paper or ink panel first.
- Don't: recolor the tile, stretch it, add a stroke, or rotate it.

For production use, outline the `s` so the logo never depends on a font
being installed at render time.

## Color

A six-stop orange palette. Flame is the brand color; ember anchors
headings; amber highlights; peach fills; ink is text; paper is ground.

| Token | Hex | Use |
|-------|-----|-----|
| Ember | `#B23A0E` | headings, strong accents |
| Flame | `#EE6C1A` | brand color, links, the logo tile |
| Amber | `#F4A024` | highlights, callouts, the gradient top |
| Peach | `#FCE7D2` | code backgrounds, tints, panels |
| Ink   | `#241B16` | body text (a warm near-black) |
| Paper | `#FFFCF7` | page background (a warm near-white) |

Pairings that hold their contrast: ink on paper for body, paper on
flame for the logo, ember on peach for inline code.

## Typography

Serif throughout, because calm reads as simple. One serif family, one
mono family, nothing else.

- **Body and headings:** PT Serif. Regular for text, Bold for headings,
  Italic for emphasis and the logo letter.
- **Code:** DejaVu Sans Mono, set a touch smaller than body so a code
  block sits quietly inside the prose.
- **Scale:** body 11 pt, line height ~1.5. Headings step up in size and
  switch to ember; the body stays ink.
- **Fallbacks:** Georgia, then Noto Serif, then any serif.

Set generous margins and short measure. White space is the cheapest way
to look simple.

## Voice

The words match the look: lead with the answer, keep sentences short,
prefer one clear statement to two hedged ones. No exclamation, no
jargon where a plain word exists. The documentation should feel like
the language: you can hold all of it in your head.

## Applying it (this PDF)

The spec PDF is the reference implementation of this guide: paper
background, ink body in PT Serif, ember headings, peach code blocks in
DejaVu Sans Mono, the logo on the title page. To regenerate, see the
build command kept alongside the sources.

---

By Geir Isene. Public domain (Unlicense).
