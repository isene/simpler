#!/usr/bin/env bash
# Regenerate Simpler.pdf from SPEC.md + logo.svg, themed per DESIGN.md.
# Needs: pandoc, xelatex, rsvg-convert, and the PT Serif / DejaVu Sans Mono fonts.
set -e
DIR="$(cd "$(dirname "$0")" && pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

# Logo as vector PDF for crisp embedding
rsvg-convert -f pdf "$DIR/logo.svg" -o "$WORK/logo.pdf"

# Theme preamble (orange palette, serif headings, peach code panels)
cat > "$WORK/header.tex" <<'EOF'
\usepackage{graphicx}
\usepackage{xcolor}
\definecolor{ember}{HTML}{B23A0E}
\definecolor{flame}{HTML}{EE6C1A}
\definecolor{amber}{HTML}{F4A024}
\definecolor{peach}{HTML}{FCE7D2}
\definecolor{ink}{HTML}{241B16}
\definecolor{paper}{HTML}{FFFCF7}
\pagecolor{paper}
\color{ink}
\usepackage{titlesec}
\titleformat{\section}{\Large\bfseries\color{flame}}{}{0em}{}
\titleformat{\subsection}{\large\bfseries\color{ember}}{}{0em}{}
\titleformat{\subsubsection}{\normalsize\bfseries\color{ember}}{}{0em}{}
\titlespacing*{\section}{0pt}{1.4em}{0.5em}
\usepackage{listings}
\lstset{
  basicstyle=\ttfamily\footnotesize\color{ink},
  backgroundcolor=\color{peach},
  breaklines=true, columns=fullflexible, keepspaces=true,
  showstringspaces=false, frame=none,
  xleftmargin=12pt, framexleftmargin=12pt, framexrightmargin=12pt,
  framextopmargin=4pt, framexbottommargin=4pt, aboveskip=11pt, belowskip=11pt,
}
EOF

# Title page (logo + wordmark)
cat > "$WORK/titlepage.tex" <<'EOF'
\begin{titlepage}
\centering
\vspace*{3.2cm}
\includegraphics[width=4.6cm]{logo.pdf}\\[1.1cm]
{\fontsize{56}{60}\selectfont\bfseries\color{ember}simpler{\color{flame}.}}\\[0.55cm]
{\Large\itshape\color{flame}A programming language whose only goal is to be simple.}\\[2.4cm]
{\large\color{ink}Language sketch \textperiodcentered\ v0.1}\\[0.25cm]
{\color{ink}Public domain (Unlicense)}
\vfill
\end{titlepage}
EOF

# Drop the H1 + tagline (the title page covers them)
tail -n +5 "$DIR/SPEC.md" > "$WORK/body.md"

cd "$WORK"
pandoc body.md \
  -o "$DIR/Simpler.pdf" \
  --pdf-engine=xelatex \
  -H header.tex -B titlepage.tex \
  --toc --toc-depth=2 --listings \
  -V mainfont="PT Serif" \
  -V monofont="DejaVu Sans Mono" -V monofontoptions="Scale=0.85" \
  -V geometry:margin=1in -V fontsize=11pt -V linestretch=1.4 \
  -V colorlinks=true -V linkcolor=flame -V urlcolor=flame -V toccolor=ember

echo "wrote $DIR/Simpler.pdf"
