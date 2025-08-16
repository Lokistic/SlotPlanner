# Slot Planner

A tiny ImGui + DX11 app that spits out a "slot session plan."
Built while sleep-deprived - this is a joke, don't @ me.

## Features

- Pick a game (Medium/Extreme/Insane vibes), set bankroll \& risk.
- Monte-Carlo sim with heavy-tail wins (rare spikes).
- Suggested bet, stop-loss, take-profit.
- Optional ImPlot chart!!!



## Quick Start (Recommended)

1. Grab the EXE from [Releases](../../releases).
2. Run it (Windows 10/11) - if it works.

## Notes

- Chart shows median + bands (or a simple line) - kept down on purpose.
- Extrabets (Xbet) add cost, base game pays the wins.
- Crash? Create/Destroy ImPlot context and pass float* to plot.
