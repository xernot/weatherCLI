# weathercli

A fast, AI-driven, terminal-based weather forecast application written in C. Features a split-pane interface with location management, tabbed forecasts (Today, Tomorrow, 4-Day, 9-Day) with dual weather sources (ECMWF and DWD ICON) shown side by side, and integrated AI capabilities. GPT-powered summaries adapt to your activity (motorcycling, skiing, sailing) and highlight what matters most for each. A free-form chat mode lets you ask natural language weather questions across multiple locations — ideal for planning trips and comparing conditions along a route.

```
────────────────────────────────────────────────────────────────
 > Locations              │   Vienna, AT
   /                      │   [ Today ]  Tomorrow   4-Day   9-Day  [General]
──────────────────────────│─────────────────────────────────────
   Eichgraben, AT         │  2026-04-06
   Groß Gerungs, AT       │  Morning          ECMWF       DWD ICON
 > Lech, AT               │      ⛅ Partly cloudy   ⛅ Partly cloudy
   Mariazell, AT          │      15/8C 12km/h      14/8C 11km/h
   Schladming, AT         │  ────────────────────────────────────
   St. Anton, AT          │  Afternoon
   Vienna, AT             │      ☀ Clear sky       ☀ Clear sky
                          │      19/14C 8km/h      18/13C 9km/h
                          │  ────────────────────────────────────
                          │  AI Summary (General):
                          │  A pleasant spring day with clear
                          │  skies and mild temperatures...
────────────────────────────────────────────────────────────────
 S-Tab:pane  a:activity  c:chat  r:refresh           weathercli v0.1
────────────────────────────────────────────────────────────────
```

## Features

### Location Management

- **Search**: Type in the search bar to find any city, town, or village worldwide. Uses Open-Meteo geocoding as primary search, with Nominatim (OpenStreetMap) as fallback for fuzzy matching. Informal spellings like "gross gerungs" or "st anton" work correctly.
- **Save**: Press `s` on a search result to save it to your location list.
- **Delete**: Press `x` to remove a saved location.
- **Filter**: When viewing saved locations, typing filters the list in real-time (case-insensitive substring match).
- **Alphabetical sorting**: Saved locations are always sorted alphabetically.
- **Persistent storage**: Locations are stored in `~/.weathercli/locations.json`.

### Weather Forecasts

Four tabbed views, navigable with left/right arrow keys:

| Tab | Content |
|-----|---------|
| **Today** | Time-of-day breakdown: Morning (06-12), Afternoon (12-18), Evening (18-24), Night (00-06). Each section shows condition, temp range, wind, precipitation, humidity. |
| **Tomorrow** | Same time-of-day breakdown as Today, for the next day. |
| **4-Day** | Compact overview of 4 days with condition, temperatures, wind, precipitation, humidity |
| **9-Day** | Extended compact forecast for 9 days |

All views show **dual weather sources side by side** — ECMWF and DWD ICON models via Open-Meteo — so you can compare forecasts at a glance. Weather conditions are mapped from WMO codes to human-readable descriptions and terminal-compatible icons. A last-refresh timestamp is shown in the header after loading.

### User Interface

- **Split-pane layout**: Left pane for locations, right pane for forecasts.
- **Active pane indicator**: The active pane title is shown in green bold with a `>` prefix; the inactive pane title is dimmed. The separator line under the active pane header is green.
- **Tabbed forecasts**: Today / Tomorrow / 4-Day / 9-Day with left/right arrow navigation. Active tab label in green.
- **Refresh timestamp**: The header shows the time of the last data fetch (e.g. `[14:32:07]`).
- **Auto-focus**: Selecting a location and loading its forecast automatically switches focus to the right pane.
- **Progress bar**: Loading progress is shown in the footer with a visual bar and percentage during weather and AI data fetching.
- **Refresh**: Press `r` to re-fetch weather data and AI summaries for the current location. A "Refreshed" confirmation appears in the footer for 3 seconds, then reverts to the normal hint bar.
- **Info overlay**: Press `?` to see data source information and keyboard reference.
- **Framed layout**: Header and footer are framed with horizontal border lines for a clean, structured look.
- **Quit confirmation**: Pressing `q` shows "Quit? (y/n)" in the footer bar.
- **Terminal title**: Sets the terminal window title to "WeatherCLI".
- **Version display**: `weathercli v0.1` is shown on the right side of the footer.
- **Responsive**: Adapts to terminal resize events.

## AI Features

weathercli integrates OpenAI's GPT API (gpt-5.4) for two capabilities: activity-aware forecast summaries and free-form weather chat. Both are optional — the app works fully without an API key, but skips AI output.

### Summaries

Each forecast tab includes an AI-generated natural language summary. Summaries are:

- **Per-tab**: Each tab (Today / Tomorrow / 4-Day / 9-Day) gets its own summary covering the relevant day range.
- **On-demand**: Fetched when you switch to a tab, then cached until the location or activity changes.
- **Activity-aware**: Tailored to your current activity mode (see below).

### Activity Modes

Press `a` in the right pane to open the activity picker, then use Up/Down arrows to select and Enter to confirm. Each mode changes what GPT focuses on:

| Mode | Focus |
|------|-------|
| **General** | Standard weather summary |
| **Motorbike** | Rain risk, road grip, wind gusts, visibility, riding temperature |
| **Ski** | Snowfall, freezing level, avalanche-relevant conditions, snow quality |
| **Sailing** | Wind speed/direction, gusts, precipitation, visibility, storm risk |

The current activity mode is shown in brackets on the right side of the tab bar. These are the built-in defaults — you can customize or add modes via a config file (see [Adding New Activity Modes](#adding-new-activity-modes)).

### Chat

Press `c` in the right pane to ask free-form questions about the weather:

- **Auto-detects locations**: Scans your question for saved location names (case-insensitive) and fetches weather data for each match automatically.
- **Multi-location context**: Questions like "whats about a motorbike tour today between eichgraben and gross gerungs" will fetch weather for both locations and let GPT compare conditions along the route.
- **Falls back to current location**: If no saved locations are found in the question, uses the currently selected location's forecast.
- **Displayed in right pane only**: The chat response replaces the forecast view — the left pane stays untouched.
- **Dismiss**: Press `Esc` or `q` to close the chat response and return to the forecast view.

Example questions:
- "whats about a motorbike tour today between eichgraben and gross gerungs"
- "can I go skiing in lech this weekend"
- "is it safe to sail from vienna to neusiedl am see tomorrow"
- "compare the weather in schladming and mariazell for the next 4 days"

### Graceful Degradation

Without an OpenAI API key, weathercli operates as a fully functional weather app — all forecast data, location management, and navigation work as normal. AI summaries and chat are simply skipped. No error messages or broken UI.

## Installation

### Prerequisites

- GCC with C11 support
- libcurl (development headers)
- ncurses with wide character support (development headers)

On Debian/Ubuntu:

```bash
sudo apt install build-essential libcurl4-openssl-dev libncurses-dev
```

On Arch Linux:

```bash
sudo pacman -S base-devel curl ncurses
```

On macOS (with Homebrew):

```bash
brew install curl ncurses
```

### Build

```bash
git clone <repository-url>
cd weathercli
make
```

The binary is built to `build/weathercli`.

### Run

```bash
make run
# or
./build/weathercli
```

## Configuration

### API Key

To enable AI weather summaries and chat, provide an OpenAI API key. Create the secrets file:

```bash
mkdir -p ~/.weathercli
echo "OPENAI_API_KEY=sk-your-key-here" > ~/.weathercli/secrets
chmod 600 ~/.weathercli/secrets
```

The app also checks the `OPENAI_API_KEY` environment variable as a fallback. Without a key, the app works normally but skips AI summaries and chat.

### Secrets File Format

`~/.weathercli/secrets` uses a simple key-value format:

```
# Comments start with #
OPENAI_API_KEY=sk-your-key-here
```

### All Configuration Constants

All hardcoded values (API URLs, UI dimensions, colors, key bindings, weather codes, activity prompts) are defined in `src/config.h` with descriptive comments.

## Keyboard Reference

### Global

| Key | Action |
|-----|--------|
| `Shift-Tab` | Switch between left and right pane |
| `?` | Show info overlay (data sources + keyboard reference) |

### Left Pane (Locations)

| Key | Action |
|-----|--------|
| `Up/Down` | Navigate location list |
| `Enter` | Search (if text typed) or load forecast and switch to right pane |
| `s` | Save selected search result |
| `x` | Delete selected saved location |
| `Esc` | Clear search, return to saved locations |
| Any character | Type into search bar |

### Right Pane (Forecast)

| Key | Action |
|-----|--------|
| `Left/Right` | Switch between Today / Tomorrow / 4-Day / 9-Day tabs |
| `Up/Down` | Scroll forecast content |
| `a` | Open activity picker (Up/Down to select, Enter to confirm, Esc to cancel) |
| `c` | Open chat — type a question, press Enter to send |
| `r` | Refresh forecast data |
| `Esc` | Dismiss chat response, return to forecast |
| `q` | Quit (with confirmation in footer) |

### Chat Mode (when input is active)

| Key | Action |
|-----|--------|
| Any character | Type into chat input |
| `Enter` | Send question to AI |
| `Backspace` | Delete last character |
| `Esc` | Cancel chat input |

## Project Structure

```
weathercli/
  Makefile              Build configuration
  src/
    main.c              Entry point, event loop
    ui.c / ui.h         ncurses rendering, input handling, AppState
    location.c / .h     Geocoding search (Open-Meteo + Nominatim fallback),
                        location CRUD, storage, alphabetical sorting
    weather.c / .h      Open-Meteo ECMWF + DWD ICON forecast APIs (daily + hourly),
                        WMO code mapping, time-of-day section aggregation
    gpt.c / .h          OpenAI API, activity-aware summaries, multi-location chat
    http.c / .h         libcurl wrapper (GET, POST with JSON)
    secrets.c / .h      Secrets file loader (~/.weathercli/secrets)
    activity.h          Activity mode loading (~/.weathercli/activities.conf or built-in defaults)
    config.h            All constants and configuration values
  vendor/
    cJSON.c / cJSON.h   Vendored JSON parser (MIT license)
```

## Adding New Activity Modes

Activity modes are loaded from `~/.weathercli/activities.conf` at startup. If the file is missing, the built-in defaults (General, Motorbike, Ski, Sailing) are used.

The file format is one activity per line, with the label and GPT prompt separated by `|`:

```
General|Give a general weather summary.
Motorbike|Summarize for a motorbike tour. Focus on rain risk, road grip conditions, wind gusts, visibility, and comfortable riding temperature ranges.
Ski|Summarize for ski touring or skiing. Focus on snowfall, freezing level, avalanche-relevant wind/temperature swings, visibility, and snow quality expectations.
Sailing|Summarize for sailing. Focus on wind speed and direction, gusts, wave-relevant precipitation, visibility, and storm risk.
Hiking|Summarize for a day hike. Focus on rain risk, trail conditions, temperature comfort, and UV exposure.
```

Up to 10 activity modes are supported. Changes take effect on next launch.

## APIs

| API | Purpose | Auth |
|-----|---------|------|
| [Open-Meteo Geocoding](https://open-meteo.com/en/docs/geocoding-api) | Primary location search | None (free) |
| [Nominatim / OpenStreetMap](https://nominatim.org/release-docs/develop/api/Search/) | Fallback location search (fuzzy matching) | None (free) |
| [Open-Meteo ECMWF](https://open-meteo.com/en/docs/ecmwf-api) | Daily + hourly weather forecasts (ECMWF model) | None (free) |
| [Open-Meteo DWD ICON](https://open-meteo.com/en/docs/dwd-api) | Daily + hourly weather forecasts (DWD ICON model) | None (free) |
| [OpenAI Chat Completions](https://platform.openai.com/docs/api-reference/chat) | AI weather summaries + chat (gpt-5.4) | API key required |

## License

This project is licensed under the [GNU General Public License v3.0](LICENSE).
