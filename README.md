# weathercli

A fast, terminal-based weather forecast application written in C. Features a split-pane interface with location management, tabbed forecasts with time-of-day breakdowns, AI-powered weather summaries tailored to outdoor activities, and a chat mode for free-form weather questions.

```
────────────────────────────────────────────────────────────────
 > Locations              │   Vienna, AT
   /                      │   [ Today ]  4-Day   9-Day  [General]
──────────────────────────│─────────────────────────────────────
   Eichgraben, AT         │  2026-04-06
   Groß Gerungs, AT       │  Morning      ⛅ Partly cloudy
 > Lech, AT               │      15/8C  Wind 12 km/h  0.0 mm  65%
   Mariazell, AT          │  ────────────────────────────────────
   Schladming, AT         │  Afternoon    ☀ Clear sky
   St. Anton, AT          │      19/14C  Wind 8 km/h  0.0 mm  52%
   Vienna, AT             │  ────────────────────────────────────
                          │  Evening      ⛅ Partly cloudy
                          │      16/12C  Wind 5 km/h  0.0 mm  70%
                          │  ────────────────────────────────────
                          │  AI Summary (General):
                          │  A pleasant spring day with clear
                          │  skies and mild temperatures...
────────────────────────────────────────────────────────────────
 S-Tab:pane  Tab:activity  c:chat  r:refresh        weathercli v0.1
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

Three tabbed views, navigable with left/right arrow keys:

| Tab | Content |
|-----|---------|
| **Today** | Time-of-day breakdown: Morning (06-12), Afternoon (12-18), Evening (18-24), Night (00-06). Each section shows condition, temp range, wind, precipitation, humidity. |
| **4-Day** | Compact overview of 4 days with condition, temperatures, wind, precipitation, humidity |
| **9-Day** | Extended compact forecast for 9 days |

Weather data is sourced from hourly (Today tab) and daily (4-Day/9-Day tabs) Open-Meteo API endpoints. All weather conditions are mapped from WMO weather codes to human-readable descriptions and terminal-compatible icons.

### AI-Powered Summaries

Each forecast tab includes an AI-generated natural language summary powered by OpenAI's GPT API (gpt-5.4). Summaries are:

- **Per-tab**: Each tab gets its own summary covering the relevant day range.
- **On-demand**: Fetched when you switch to a tab, then cached until the location or activity changes.
- **Activity-aware**: Tailored to your current activity mode (see below).

### Activity Modes

Cycle through activity modes with `Tab` in the right pane. Each mode changes what GPT focuses on:

| Mode | Focus |
|------|-------|
| **General** | Standard weather summary |
| **Motorbike** | Rain risk, road grip, wind gusts, visibility, riding temperature |
| **Ski** | Snowfall, freezing level, avalanche-relevant conditions, snow quality |
| **Sailing** | Wind speed/direction, gusts, precipitation, visibility, storm risk |

The current activity mode is shown in brackets on the right side of the tab bar.

### AI Chat

Press `c` in the right pane to ask free-form questions about the weather. The chat feature:

- **Auto-detects locations**: Scans your question for saved location names (case-insensitive) and fetches weather data for each match automatically.
- **Multi-location context**: Questions like "whats about a motorbike tour today between eichgraben and gross gerungs" will fetch weather for both locations and let GPT compare conditions along the route.
- **Falls back to current location**: If no saved locations are found in the question, uses the currently selected location's forecast.
- **Displayed in right pane only**: The chat response replaces the forecast view — the left pane with your locations stays untouched.
- **Dismiss**: Press `Esc` or `q` to close the chat response and return to the forecast view.

Example questions:
- "whats about a motorbike tour today between eichgraben and gross gerungs"
- "can I go skiing in lech this weekend"
- "is it safe to sail from vienna to neusiedl am see tomorrow"
- "compare the weather in schladming and mariazell for the next 4 days"

### User Interface

- **Split-pane layout**: Left pane for locations, right pane for forecasts.
- **Active pane indicator**: The active pane title is shown in green bold with a `>` prefix; the inactive pane title is dimmed. The separator line under the active pane header is green.
- **Tabbed forecasts**: Today / 4-Day / 9-Day with left/right arrow navigation. Active tab label in green.
- **Auto-focus**: Selecting a location and loading its forecast automatically switches focus to the right pane.
- **Progress bar**: Loading progress is shown in the footer with a visual bar and percentage during weather and AI data fetching.
- **Refresh**: Press `r` to re-fetch weather data and AI summaries for the current location.
- **Info overlay**: Press `?` to see data source information and keyboard reference.
- **Framed layout**: Header and footer are framed with horizontal border lines for a clean, structured look.
- **Quit confirmation**: Pressing `q` shows "Quit? (y/n)" in the footer bar.
- **Terminal title**: Sets the terminal window title to "WeatherCLI".
- **Version display**: `weathercli v0.1` is shown on the right side of the footer.
- **Responsive**: Adapts to terminal resize events.

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
| `Left/Right` | Switch between Today / 4-Day / 9-Day tabs |
| `Up/Down` | Scroll forecast content |
| `Tab` | Cycle activity mode (General / Motorbike / Ski / Sailing) |
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
    weather.c / .h      Open-Meteo forecast API (daily + hourly),
                        WMO code mapping, time-of-day section aggregation
    gpt.c / .h          OpenAI API, activity-aware summaries, multi-location chat
    http.c / .h         libcurl wrapper (GET, POST with JSON)
    secrets.c / .h      Secrets file loader (~/.weathercli/secrets)
    activity.h          Activity mode definitions (references config.h)
    config.h            All constants and configuration values
  vendor/
    cJSON.c / cJSON.h   Vendored JSON parser (MIT license)
```

## Adding New Activity Modes

Activity modes are designed to be easily extended. To add a new mode:

1. **Define constants** in `src/config.h`:
   ```c
   #define ACTIVITY_LABEL_HIKE "Hiking"
   #define ACTIVITY_PROMPT_HIKE \
     "Summarize for a day hike. Focus on rain risk, " \
     "trail conditions, temperature comfort, and UV exposure."
   ```

2. **Increment** `ACTIVITY_COUNT` in `src/config.h`.

3. **Add the entry** to the `ACTIVITIES` array in `src/activity.h`:
   ```c
   {ACTIVITY_LABEL_HIKE, ACTIVITY_PROMPT_HIKE},
   ```

Rebuild with `make` and the new mode appears in the Tab cycle.

## APIs

| API | Purpose | Auth |
|-----|---------|------|
| [Open-Meteo Geocoding](https://open-meteo.com/en/docs/geocoding-api) | Primary location search | None (free) |
| [Nominatim / OpenStreetMap](https://nominatim.org/release-docs/develop/api/Search/) | Fallback location search (fuzzy matching) | None (free) |
| [Open-Meteo Forecast](https://open-meteo.com/en/docs) | Daily + hourly weather forecasts with WMO codes | None (free) |
| [OpenAI Chat Completions](https://platform.openai.com/docs/api-reference/chat) | AI weather summaries + chat (gpt-5.4) | API key required |

## License

MIT
