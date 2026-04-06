# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**weathercli** is a terminal-based weather forecast application written in C. It features a split-pane TUI (ncurses) with location management on the left and tabbed weather forecasts on the right. Weather data comes from the Open-Meteo API (free, no key required) with hourly detail for today. Forecasts are summarized by GPT (gpt-5.4) via the OpenAI API with activity-specific context (general, motorbike, ski, sailing). A chat mode allows free-form weather questions with multi-location context. Location search uses Open-Meteo geocoding with Nominatim (OpenStreetMap) as fallback for fuzzy matching.

## Build & Run

```bash
make            # build to build/weathercli
make run        # build and run
make clean      # remove build artifacts
```

### Dependencies

- **gcc** with C11 support
- **libcurl** (dev) — HTTP requests
- **libncursesw** (dev) — terminal UI with wide character support
- **cJSON** — vendored in `vendor/` (no install needed)

On Debian/Ubuntu: `sudo apt install build-essential libcurl4-openssl-dev libncurses-dev`

### API Key Setup

Store the OpenAI API key in `~/.weathercli/secrets`:
```
OPENAI_API_KEY=sk-your-key-here
```
Falls back to the `OPENAI_API_KEY` environment variable. The app works without a key but skips AI summaries and chat.

## Architecture

Module-per-concern structure. All modules communicate through structs defined in headers; no global mutable state outside `AppState` in main.

```
src/
  main.c        — entry point, event loop
  ui.c/h        — ncurses rendering, input handling, owns AppState
  location.c/h  — geocoding search (Open-Meteo + Nominatim fallback), saved location CRUD, sorting
  weather.c/h   — daily + hourly forecast from Open-Meteo API, time-of-day section aggregation
  gpt.c/h       — OpenAI API: activity-aware summaries + multi-location chat
  http.c/h      — libcurl wrapper (GET and POST with JSON)
  secrets.c/h   — loads KEY=value pairs from ~/.weathercli/secrets, falls back to env vars
  activity.h    — activity mode definitions (general, bike, ski, sail) referencing config.h constants
  config.h      — all constants: API URLs, UI dimensions, key bindings, color pairs, WMO codes, activity prompts
vendor/
  cJSON.c/h     — vendored JSON parser (MIT license)
```

### Data Flow

1. User types in search bar -> `location_search()` tries Open-Meteo geocoding, falls back to Nominatim
2. User selects a location -> `weather_fetch()` fetches 9-day daily forecast + `weather_fetch_today_detail()` fetches hourly data
3. Today's hourly data is aggregated into 4 sections: Morning (06-12), Afternoon (12-18), Evening (18-24), Night (00-06)
4. On tab view, `fetch_gpt_for_current_tab()` sends the relevant day range + activity prompt to GPT on-demand
5. `ui_render()` draws both panes from `AppState`
6. After loading, focus auto-switches to the right pane

### Chat Flow

1. User presses `c` -> chat input opens in footer
2. `submit_chat()` scans the question for saved location names (case-insensitive match)
3. Fetches weather for each matched location via `weather_fetch()`
4. `gpt_format_forecast()` builds weather context for each location
5. `gpt_chat()` sends combined context + question to GPT
6. Response displayed in right pane only; left pane stays unchanged
7. `Esc` or `q` dismisses the response

### Key Bindings

- **Shift-Tab** — switch between left/right panes
- **Arrow Up/Down** — navigate locations (left pane) or scroll forecast (right pane)
- **Arrow Left/Right** — switch forecast tabs (Today / 4-Day / 9-Day) in right pane
- **Tab** — cycle activity mode (General / Motorbike / Ski / Sailing) in right pane
- **Enter** — trigger API search (when text is typed) or load forecast and switch to right pane
- **c** — open chat input for free-form weather questions (right pane)
- **r** — refresh current forecast data (right pane)
- **s** — save selected search result to stored locations
- **x** — delete selected saved location
- **?** — show info overlay (data sources + keyboard reference)
- **Esc** — clear search / return to saved locations / dismiss chat response
- **q** — quit with confirmation in footer (when right pane is focused)

### Adding New Activity Modes

1. Add label + prompt constants in `config.h`
2. Increment `ACTIVITY_COUNT`
3. Add entry to the `ACTIVITIES` array in `activity.h`

### Storage

- **Locations**: `~/.weathercli/locations.json` — JSON array, auto-sorted alphabetically
- **Secrets**: `~/.weathercli/secrets` — one `KEY=value` per line, `#` for comments

### UI Notes

- Terminal title is set via `fprintf(stderr, ...)` — do NOT use `putp()` or `printf` to stdout
- Active pane has green bold `>` prefix on title, inactive is dimmed
- Header and footer are framed with horizontal border lines
- Progress bar shown in footer during data loading
- Text rendering in panes uses `addch()` char-by-char — never `mvprintw` with strings containing `\n` (breaks pane boundaries)
- GPT API uses `max_completion_tokens` (not `max_tokens`) for gpt-5.4

## APIs Used

- **Open-Meteo Geocoding**: `https://geocoding-api.open-meteo.com/v1/search` — free, no auth
- **Nominatim (OpenStreetMap)**: `https://nominatim.openstreetmap.org/search` — free fallback for fuzzy search
- **Open-Meteo Forecast**: `https://api.open-meteo.com/v1/forecast` — free, no auth, daily + hourly data
- **OpenAI Chat Completions**: `https://api.openai.com/v1/chat/completions` — requires API key, uses gpt-5.4
