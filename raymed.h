/*******************************************************************************************
*
*   raymed v1.0 - Medical-themed immediate-mode widgets for raylib, in the style of raygui
*
*   DESCRIPTION:
*       Patient-monitor style controls: sweep waveforms (ECG/pleth/resp), vital-sign tiles,
*       blood pressure tiles, trend charts with alarm limit bands, NIBP whisker trends,
*       alarm banners, bar gauges, dual-thumb alarm limit editors, a 0-10 pain scale,
*       infusion status and a patient banner.
*
*       Like raygui, every control is a single call per frame: pass bounds + data, it draws
*       and returns an int result (clicked / changed / hovered index). No retained state
*       except what you pass by pointer (e.g. a GuiWaveBuffer for waveforms).
*
*   USAGE:
*       #define RAYMED_IMPLEMENTATION
*       #include "raymed.h"
*
*       Works standalone or next to raygui. If raygui.h is included BEFORE the implementation,
*       raymed respects GuiLock()/GuiDisable() and falls back to GuiGetFont().
*
*   STYLE:
*       Properties are ints, colors stored with ColorToInt() just like raygui:
*           GuiMedSetStyle(MED_COLOR_ECG, ColorToInt(GREEN));
*           Color c = GuiMedColor(MED_COLOR_SPO2);
*       Built-in themes: GuiMedLoadStyleDark() (default), GuiMedLoadStyleLight().
*
*   NOT A MEDICAL DEVICE. For visualisation, training, simulation and dashboards only.
*
*   LICENSE: zlib/libpng (same as raylib/raygui), see LICENSE
*
*   Copyright (c) 2026 Jon Harsem
*
**********************************************************************************************/

#ifndef RAYMED_H
#define RAYMED_H

#include "raylib.h"
#include <stdbool.h>

#ifndef RAYMEDAPI
    #define RAYMEDAPI
#endif

//----------------------------------------------------------------------------------
// Types and Structures
//----------------------------------------------------------------------------------

// Style properties (colors are stored as ColorToInt() values)
typedef enum {
    MED_COLOR_BACKGROUND = 0,   // Screen background (for the app to clear with)
    MED_COLOR_PANEL,            // Widget panel fill
    MED_COLOR_BORDER,           // Widget border
    MED_COLOR_FOCUS,            // Border when hovered
    MED_COLOR_TEXT,             // Primary text
    MED_COLOR_TEXT_DIM,         // Secondary text (units, limits, axes)
    MED_COLOR_GRID,             // Chart gridlines / ECG paper
    MED_COLOR_ALARM_HIGH,       // High priority alarm (red)
    MED_COLOR_ALARM_MEDIUM,     // Medium priority alarm (yellow)
    MED_COLOR_ALARM_LOW,        // Low priority / advisory (cyan)
    MED_COLOR_ALARM_TEXT,       // Text drawn on top of alarm fills
    MED_COLOR_ECG,              // Parameter colors, by monitor convention
    MED_COLOR_SPO2,
    MED_COLOR_BP,
    MED_COLOR_RESP,
    MED_COLOR_TEMP,
    MED_COLOR_CO2,
    MED_BORDER_WIDTH,           // Panel border thickness (px)
    MED_ROUNDNESS,              // Panel corner roundness (percent, 0..100)
    MED_TEXT_SIZE,              // Base label text size (px)
    MED_TEXT_SPACING,           // Glyph spacing as percent of text size
    MED_LINE_THICKNESS,         // Trace / chart line thickness (px)
    MED_WAVE_GAP,               // Sweep erase-bar width (px)
    MED_WAVE_GRID,              // Draw ECG paper grid behind waveforms (0/1)
    MED_TREND_SAMPLE_SECONDS,   // Seconds between trend samples, used for time axis labels
    MED_FLASH_ENABLED,          // Allow alarm flashing (0/1), disable for photosensitivity
    MED_STYLE_PROPERTY_COUNT
} GuiMedProperty;

// Alarm priorities (IEC 60601-1-8 inspired: high flashes fast, medium slow, low steady)
typedef enum {
    MED_ALARM_NONE = 0,
    MED_ALARM_LOW,
    MED_ALARM_MEDIUM,
    MED_ALARM_HIGH
} GuiMedAlarmPriority;

// Sweep waveform sample buffer. Storage is owned by the caller (no allocation).
typedef struct GuiWaveBuffer {
    float *samples;             // Caller-provided storage, capacity floats
    int capacity;               // Samples across the full width of the display
    int head;                   // Next write index (the sweep position)
    int count;                  // Valid samples written (saturates at capacity)
    float minValue;             // Value mapped to the bottom of the trace
    float maxValue;             // Value mapped to the top of the trace
} GuiWaveBuffer;

#if defined(__cplusplus)
extern "C" {
#endif

//----------------------------------------------------------------------------------
// Global state / style
//----------------------------------------------------------------------------------
RAYMEDAPI void GuiMedSetStyle(int property, int value);     // Set one style property
RAYMEDAPI int GuiMedGetStyle(int property);                 // Get one style property
RAYMEDAPI Color GuiMedColor(int property);                  // Get a color style property as Color
RAYMEDAPI void GuiMedLoadStyleDark(void);                   // Bedside monitor theme (default)
RAYMEDAPI void GuiMedLoadStyleLight(void);                  // Chart/record theme for light UIs
RAYMEDAPI void GuiMedSetFont(Font font);                    // Set font (use a large TTF for crisp numerics)
RAYMEDAPI Font GuiMedGetFont(void);
RAYMEDAPI void GuiMedLock(void);                            // Disable interaction (drawing continues)
RAYMEDAPI void GuiMedUnlock(void);
RAYMEDAPI bool GuiMedIsLocked(void);
RAYMEDAPI int GuiMedCheckLimits(float value, float lowLimit, float highLimit); // -1 below, 0 inside, 1 above (NaN limit = off)

//----------------------------------------------------------------------------------
// Waveform buffer
//----------------------------------------------------------------------------------
RAYMEDAPI void GuiWaveBufferInit(GuiWaveBuffer *wave, float *storage, int capacity, float minValue, float maxValue);
RAYMEDAPI void GuiWaveBufferPush(GuiWaveBuffer *wave, float sample);
RAYMEDAPI void GuiWaveBufferClear(GuiWaveBuffer *wave);

//----------------------------------------------------------------------------------
// Controls
//----------------------------------------------------------------------------------
// Sweep-mode trace with erase bar, like a bedside monitor. Returns 1 when clicked
RAYMEDAPI int GuiWaveform(Rectangle bounds, const char *label, GuiWaveBuffer *wave, Color color);

// Large numeric parameter tile with limits; flashes when out of limits. NaN value shows "-?-". Returns 1 when clicked
RAYMEDAPI int GuiVitalSign(Rectangle bounds, const char *label, const char *units, float value, int decimals, float lowLimit, float highLimit, Color color);

// Systolic/diastolic tile with computed MAP; limits apply to systolic. Returns 1 when clicked
RAYMEDAPI int GuiBloodPressure(Rectangle bounds, const char *label, float systolic, float diastolic, float sysLowLimit, float sysHighLimit, Color color);

// Trend line with shaded out-of-limit bands, time axis and hover readout. NaN = gap. Returns hovered sample index or -1
RAYMEDAPI int GuiTrendChart(Rectangle bounds, const char *label, const float *values, int count, float minValue, float maxValue, float lowLimit, float highLimit, Color color);

// NIBP style trend: a whisker per measurement from diastolic to systolic, MAP dot. Returns hovered index or -1
RAYMEDAPI int GuiBloodPressureTrend(Rectangle bounds, const char *label, const float *systolic, const float *diastolic, int count, float minValue, float maxValue, Color color);

// Alarm message bar; flashing depends on priority. Returns 1 when clicked (acknowledge)
RAYMEDAPI int GuiAlarmBanner(Rectangle bounds, const char *text, int priority);

// Vertical (h > w) or horizontal level gauge with limit markers. Returns 1 when clicked
RAYMEDAPI int GuiBarGauge(Rectangle bounds, const char *label, float value, float minValue, float maxValue, float lowLimit, float highLimit, Color color);

// Dual-thumb alarm limit editor, shows the current value as a marker. Returns 1 when a limit changed
RAYMEDAPI int GuiAlarmLimits(Rectangle bounds, const char *label, float *lowLimit, float *highLimit, float minValue, float maxValue, float step, float current, Color color);

// 0-10 numeric pain rating scale with faces. Returns 1 when value changed
RAYMEDAPI int GuiPainScale(Rectangle bounds, int *value);

// Infusion pump status: rate, volume infused / VTBI, time remaining, animated drip. Returns 1 when clicked
RAYMEDAPI int GuiInfusion(Rectangle bounds, const char *drug, float rateMlPerHour, float infusedMl, float volumeMl, bool running);

// Patient identification strip; allergies highlighted (NULL or "" shows NKDA). Returns 1 when clicked
RAYMEDAPI int GuiPatientBanner(Rectangle bounds, const char *name, const char *details, const char *allergies);

#if defined(__cplusplus)
}
#endif

#endif // RAYMED_H

/***********************************************************************************
*
*   RAYMED IMPLEMENTATION
*
************************************************************************************/

#if defined(RAYMED_IMPLEMENTATION) && !defined(RAYMED_IMPLEMENTATION_DONE)
#define RAYMED_IMPLEMENTATION_DONE

#include <math.h>
#include <stdio.h>
#include <string.h>

//----------------------------------------------------------------------------------
// Global state
//----------------------------------------------------------------------------------
static int medStyle[MED_STYLE_PROPERTY_COUNT] = { 0 };
static bool medStyleLoaded = false;
static bool medLocked = false;
static Font medFont = { 0 };
static bool medFontSet = false;

static Rectangle medDragRec = { 0 };        // Control currently being dragged (GuiAlarmLimits)
static int medDragThumb = -1;

//----------------------------------------------------------------------------------
// Internal helpers
//----------------------------------------------------------------------------------
static void MedEnsureStyle(void)
{
    if (!medStyleLoaded) GuiMedLoadStyleDark();
}

static Color MedCol(int property) { return GuiMedColor(property); }

static bool MedIsNaN(float v) { return v != v; }

static float MedClamp(float v, float lo, float hi) { return (v < lo)? lo : ((v > hi)? hi : v); }

static bool MedRecEquals(Rectangle a, Rectangle b)
{
    return (a.x == b.x) && (a.y == b.y) && (a.width == b.width) && (a.height == b.height);
}

static bool MedInteractive(void)
{
    if (medLocked) return false;
#if defined(RAYGUI_H)
    if (GuiIsLocked() || (GuiGetState() == STATE_DISABLED)) return false;
#endif
    return true;
}

static bool MedHovered(Rectangle bounds)
{
    return MedInteractive() && CheckCollisionPointRec(GetMousePosition(), bounds);
}

static bool MedClicked(Rectangle bounds)
{
    return MedHovered(bounds) && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}

// Returns true during the "on" half of a flash cycle
static bool MedFlash(float hz)
{
    if (!GuiMedGetStyle(MED_FLASH_ENABLED)) return true;
    return fmodf((float)GetTime()*hz, 1.0f) < 0.5f;
}

static Font MedFont(void)
{
    if (medFontSet) return medFont;
#if defined(RAYGUI_H)
    return GuiGetFont();
#else
    return GetFontDefault();
#endif
}

static float MedSpacing(float size) { return size*(float)GuiMedGetStyle(MED_TEXT_SPACING)/100.0f; }

static Vector2 MedMeasure(const char *text, float size)
{
    return MeasureTextEx(MedFont(), text, size, MedSpacing(size));
}

static void MedText(const char *text, float x, float y, float size, Color color)
{
    DrawTextEx(MedFont(), text, (Vector2){ roundf(x), roundf(y) }, size, MedSpacing(size), color);
}

// Draw text inside rect: hAlign/vAlign 0 = start, 1 = center, 2 = end
static void MedTextAligned(const char *text, Rectangle rec, float size, int hAlign, int vAlign, Color color)
{
    Vector2 m = MedMeasure(text, size);
    float x = rec.x + ((hAlign == 1)? (rec.width - m.x)/2 : ((hAlign == 2)? rec.width - m.x : 0));
    float y = rec.y + ((vAlign == 1)? (rec.height - m.y)/2 : ((vAlign == 2)? rec.height - m.y : 0));
    MedText(text, x, y, size, color);
}

// Largest size <= maxHeight such that text fits in maxWidth
static float MedFitSize(const char *text, float maxWidth, float maxHeight)
{
    float size = maxHeight;
    float w = MedMeasure(text, size).x;
    if (w > maxWidth && w > 0) size *= maxWidth/w;
    return (size < 6)? 6 : size;
}

static void MedPanel(Rectangle bounds, Color fill, Color border)
{
    float round = (float)GuiMedGetStyle(MED_ROUNDNESS)/100.0f;
    float minSide = (bounds.width < bounds.height)? bounds.width : bounds.height;
    float roundness = (minSide > 0)? MedClamp(round*24.0f/minSide, 0, 1) : 0;
    int bw = GuiMedGetStyle(MED_BORDER_WIDTH);

    if (roundness > 0)
    {
        DrawRectangleRounded(bounds, roundness, 6, fill);
        if (bw > 0) DrawRectangleRoundedLinesEx(bounds, roundness, 6, (float)bw, border);
    }
    else
    {
        DrawRectangleRec(bounds, fill);
        if (bw > 0) DrawRectangleLinesEx(bounds, (float)bw, border);
    }
}

static void MedDashedLineH(float x0, float x1, float y, float thick, Color color)
{
    for (float x = x0; x < x1; x += 8) DrawLineEx((Vector2){ x, y }, (Vector2){ fminf(x + 4, x1), y }, thick, color);
}

static const char *MedFormatValue(float value, int decimals)
{
    if (MedIsNaN(value)) return "-?-";
    return TextFormat("%.*f", decimals, value);
}

static const char *MedFormatLimit(float value)
{
    if (MedIsNaN(value)) return "OFF";
    return (fabsf(value - roundf(value)) < 0.01f)? TextFormat("%.0f", value) : TextFormat("%.1f", value);
}

static const char *MedFormatAgo(float seconds)
{
    int s = (int)roundf(seconds);
    if (s <= 0) return "now";
    if (s < 60) return TextFormat("-%ds", s);
    if (s < 3600) return TextFormat("-%dm", s/60);
    if ((s % 3600) == 0) return TextFormat("-%dh", s/3600);
    return TextFormat("-%dh%02d", s/3600, (s % 3600)/60);
}

static Color MedAlarmColor(int priority)
{
    switch (priority)
    {
        case MED_ALARM_HIGH: return MedCol(MED_COLOR_ALARM_HIGH);
        case MED_ALARM_MEDIUM: return MedCol(MED_COLOR_ALARM_MEDIUM);
        case MED_ALARM_LOW: return MedCol(MED_COLOR_ALARM_LOW);
        default: return MedCol(MED_COLOR_PANEL);
    }
}

// Shared chart chrome: panel, title, y gridlines + labels, time axis. Returns the plot area
static Rectangle MedChartFrame(Rectangle bounds, const char *label, int count, float minValue, float maxValue, Color color, bool hovered)
{
    float ts = (float)GuiMedGetStyle(MED_TEXT_SIZE);
    float small = ts*0.8f;
    Color dim = MedCol(MED_COLOR_TEXT_DIM);
    Color grid = MedCol(MED_COLOR_GRID);

    MedPanel(bounds, MedCol(MED_COLOR_PANEL), hovered? MedCol(MED_COLOR_FOCUS) : MedCol(MED_COLOR_BORDER));
    if (label) MedText(label, bounds.x + 8, bounds.y + 6, ts, color);

    const char *wideLabel = MedFormatLimit(fabsf(maxValue) > fabsf(minValue)? maxValue : minValue);
    float axisW = MedMeasure(wideLabel, small).x + 14;
    Rectangle plot = { bounds.x + axisW, bounds.y + ts + 14, bounds.width - axisW - 12, bounds.height - ts - 14 - small - 12 };
    if (plot.width < 4) plot.width = 4;
    if (plot.height < 4) plot.height = 4;

    // Y gridlines at quarters
    for (int i = 0; i <= 4; i++)
    {
        float t = (float)i/4.0f;
        float y = plot.y + plot.height*(1.0f - t);
        float v = minValue + (maxValue - minValue)*t;
        DrawLineEx((Vector2){ plot.x, y }, (Vector2){ plot.x + plot.width, y }, 1, grid);
        MedTextAligned(MedFormatLimit(v), (Rectangle){ bounds.x, y - small/2, axisW - 6, small }, small, 2, 1, dim);
    }

    // Time axis: pick a tick interval giving <= 6 ticks
    int sampleSeconds = GuiMedGetStyle(MED_TREND_SAMPLE_SECONDS);
    float span = (float)((count > 1)? count - 1 : 1)*(float)sampleSeconds;
    static const int nice[] = { 10, 30, 60, 300, 600, 900, 1800, 3600, 7200, 10800, 14400, 21600, 43200, 86400 };
    int interval = nice[13];
    for (int i = 0; i < 14; i++) { if (span/(float)nice[i] <= 6.0f) { interval = nice[i]; break; } }

    for (float ago = 0; ago <= span + 0.5f; ago += (float)interval)
    {
        float x = plot.x + plot.width*(1.0f - ago/span);
        DrawLineEx((Vector2){ x, plot.y }, (Vector2){ x, plot.y + plot.height }, 1, Fade(grid, 0.5f));
        const char *t = MedFormatAgo(ago);
        float tw = MedMeasure(t, small).x;
        float tx = MedClamp(x - tw/2, bounds.x + 4, bounds.x + bounds.width - tw - 4);
        MedText(t, tx, plot.y + plot.height + 5, small, dim);
    }

    return plot;
}

static float MedMapY(float v, Rectangle plot, float minValue, float maxValue)
{
    float t = (maxValue != minValue)? (v - minValue)/(maxValue - minValue) : 0.5f;
    return plot.y + plot.height*(1.0f - MedClamp(t, 0, 1));
}

static float MedMapX(int i, int count, Rectangle plot)
{
    return (count > 1)? plot.x + plot.width*(float)i/(float)(count - 1) : plot.x + plot.width;
}

static int MedHoverIndex(Rectangle plot, int count)
{
    if (count <= 0 || !MedHovered(plot)) return -1;
    float t = (GetMousePosition().x - plot.x)/plot.width;
    int i = (int)roundf(t*(float)(count - 1));
    return (int)MedClamp((float)i, 0, (float)(count - 1));
}

static void MedTooltip(float x, Rectangle plot, Rectangle bounds, const char *line1, const char *line2, Color accent)
{
    float ts = (float)GuiMedGetStyle(MED_TEXT_SIZE);
    DrawLineEx((Vector2){ x, plot.y }, (Vector2){ x, plot.y + plot.height }, 1, MedCol(MED_COLOR_TEXT_DIM));

    float w = fmaxf(MedMeasure(line1, ts).x, MedMeasure(line2, ts*0.8f).x) + 16;
    float h = ts*1.8f + 12;
    float bx = (x + 10 + w < bounds.x + bounds.width)? x + 10 : x - 10 - w;
    Rectangle box = { bx, plot.y + 4, w, h };
    DrawRectangleRec(box, MedCol(MED_COLOR_BACKGROUND));
    DrawRectangleLinesEx(box, 1, accent);
    MedText(line1, box.x + 8, box.y + 5, ts, MedCol(MED_COLOR_TEXT));
    MedText(line2, box.x + 8, box.y + 7 + ts, ts*0.8f, MedCol(MED_COLOR_TEXT_DIM));
}

static void MedDrawBell(Rectangle r, Color color)
{
    float cx = r.x + r.width/2;
    float dome = r.width*0.3f;
    float top = r.y + r.height*0.12f + dome;
    float rim = r.y + r.height*0.72f;
    DrawCircleSector((Vector2){ cx, top }, dome, 180, 360, 12, color);
    // Flared body: narrow under the dome, wide at the rim
    DrawTriangle((Vector2){ cx - dome, top }, (Vector2){ cx - r.width*0.44f, rim }, (Vector2){ cx + r.width*0.44f, rim }, color);
    DrawTriangle((Vector2){ cx - dome, top }, (Vector2){ cx + r.width*0.44f, rim }, (Vector2){ cx + dome, top }, color);
    DrawRectangleRec((Rectangle){ cx - r.width*0.5f, rim, r.width, r.height*0.08f }, color);
    DrawCircleV((Vector2){ cx, rim + r.height*0.16f }, r.width*0.09f, color);
    DrawCircleV((Vector2){ cx, r.y + r.height*0.1f }, r.width*0.06f, color);
}

//----------------------------------------------------------------------------------
// Global state / style
//----------------------------------------------------------------------------------
void GuiMedSetStyle(int property, int value)
{
    MedEnsureStyle();
    if (property >= 0 && property < MED_STYLE_PROPERTY_COUNT) medStyle[property] = value;
}

int GuiMedGetStyle(int property)
{
    MedEnsureStyle();
    return (property >= 0 && property < MED_STYLE_PROPERTY_COUNT)? medStyle[property] : 0;
}

Color GuiMedColor(int property)
{
    return GetColor((unsigned int)GuiMedGetStyle(property));
}

static void MedLoadStyleCommon(void)
{
    medStyle[MED_BORDER_WIDTH] = 1;
    medStyle[MED_ROUNDNESS] = 20;
    medStyle[MED_TEXT_SIZE] = 16;
    medStyle[MED_TEXT_SPACING] = 6;
    medStyle[MED_LINE_THICKNESS] = 2;
    medStyle[MED_WAVE_GAP] = 18;
    medStyle[MED_WAVE_GRID] = 0;
    medStyle[MED_TREND_SAMPLE_SECONDS] = 60;
    medStyle[MED_FLASH_ENABLED] = 1;
}

void GuiMedLoadStyleDark(void)
{
    medStyleLoaded = true;
    MedLoadStyleCommon();
    medStyle[MED_COLOR_BACKGROUND]   = ColorToInt((Color){ 6, 9, 14, 255 });
    medStyle[MED_COLOR_PANEL]        = ColorToInt((Color){ 14, 19, 27, 255 });
    medStyle[MED_COLOR_BORDER]       = ColorToInt((Color){ 36, 45, 58, 255 });
    medStyle[MED_COLOR_FOCUS]        = ColorToInt((Color){ 92, 112, 138, 255 });
    medStyle[MED_COLOR_TEXT]         = ColorToInt((Color){ 232, 238, 245, 255 });
    medStyle[MED_COLOR_TEXT_DIM]     = ColorToInt((Color){ 128, 142, 160, 255 });
    medStyle[MED_COLOR_GRID]         = ColorToInt((Color){ 28, 36, 47, 255 });
    medStyle[MED_COLOR_ALARM_HIGH]   = ColorToInt((Color){ 230, 40, 48, 255 });
    medStyle[MED_COLOR_ALARM_MEDIUM] = ColorToInt((Color){ 245, 196, 0, 255 });
    medStyle[MED_COLOR_ALARM_LOW]    = ColorToInt((Color){ 0, 180, 220, 255 });
    medStyle[MED_COLOR_ALARM_TEXT]   = ColorToInt((Color){ 10, 10, 10, 255 });
    medStyle[MED_COLOR_ECG]          = ColorToInt((Color){ 60, 230, 90, 255 });
    medStyle[MED_COLOR_SPO2]         = ColorToInt((Color){ 40, 210, 240, 255 });
    medStyle[MED_COLOR_BP]           = ColorToInt((Color){ 255, 80, 80, 255 });
    medStyle[MED_COLOR_RESP]         = ColorToInt((Color){ 250, 220, 60, 255 });
    medStyle[MED_COLOR_TEMP]         = ColorToInt((Color){ 225, 140, 255, 255 });
    medStyle[MED_COLOR_CO2]          = ColorToInt((Color){ 235, 235, 235, 255 });
}

void GuiMedLoadStyleLight(void)
{
    medStyleLoaded = true;
    MedLoadStyleCommon();
    medStyle[MED_COLOR_BACKGROUND]   = ColorToInt((Color){ 238, 241, 245, 255 });
    medStyle[MED_COLOR_PANEL]        = ColorToInt((Color){ 255, 255, 255, 255 });
    medStyle[MED_COLOR_BORDER]       = ColorToInt((Color){ 205, 212, 222, 255 });
    medStyle[MED_COLOR_FOCUS]        = ColorToInt((Color){ 110, 130, 160, 255 });
    medStyle[MED_COLOR_TEXT]         = ColorToInt((Color){ 22, 28, 38, 255 });
    medStyle[MED_COLOR_TEXT_DIM]     = ColorToInt((Color){ 100, 112, 128, 255 });
    medStyle[MED_COLOR_GRID]         = ColorToInt((Color){ 228, 232, 238, 255 });
    medStyle[MED_COLOR_ALARM_HIGH]   = ColorToInt((Color){ 205, 30, 40, 255 });
    medStyle[MED_COLOR_ALARM_MEDIUM] = ColorToInt((Color){ 232, 170, 0, 255 });
    medStyle[MED_COLOR_ALARM_LOW]    = ColorToInt((Color){ 0, 140, 190, 255 });
    medStyle[MED_COLOR_ALARM_TEXT]   = ColorToInt((Color){ 255, 255, 255, 255 });
    medStyle[MED_COLOR_ECG]          = ColorToInt((Color){ 20, 150, 60, 255 });
    medStyle[MED_COLOR_SPO2]         = ColorToInt((Color){ 0, 130, 175, 255 });
    medStyle[MED_COLOR_BP]           = ColorToInt((Color){ 200, 40, 50, 255 });
    medStyle[MED_COLOR_RESP]         = ColorToInt((Color){ 175, 125, 0, 255 });
    medStyle[MED_COLOR_TEMP]         = ColorToInt((Color){ 140, 60, 180, 255 });
    medStyle[MED_COLOR_CO2]          = ColorToInt((Color){ 70, 80, 95, 255 });
}

void GuiMedSetFont(Font font) { medFont = font; medFontSet = (font.texture.id > 0); }
Font GuiMedGetFont(void) { return MedFont(); }
void GuiMedLock(void) { medLocked = true; }
void GuiMedUnlock(void) { medLocked = false; }
bool GuiMedIsLocked(void) { return medLocked; }

int GuiMedCheckLimits(float value, float lowLimit, float highLimit)
{
    if (MedIsNaN(value)) return 0;
    if (!MedIsNaN(lowLimit) && value < lowLimit) return -1;
    if (!MedIsNaN(highLimit) && value > highLimit) return 1;
    return 0;
}

//----------------------------------------------------------------------------------
// Waveform buffer
//----------------------------------------------------------------------------------
void GuiWaveBufferInit(GuiWaveBuffer *wave, float *storage, int capacity, float minValue, float maxValue)
{
    wave->samples = storage;
    wave->capacity = capacity;
    wave->minValue = minValue;
    wave->maxValue = maxValue;
    GuiWaveBufferClear(wave);
}

void GuiWaveBufferPush(GuiWaveBuffer *wave, float sample)
{
    if (!wave || !wave->samples || wave->capacity <= 0) return;
    wave->samples[wave->head] = sample;
    wave->head = (wave->head + 1) % wave->capacity;
    if (wave->count < wave->capacity) wave->count++;
}

void GuiWaveBufferClear(GuiWaveBuffer *wave)
{
    wave->head = 0;
    wave->count = 0;
}

//----------------------------------------------------------------------------------
// Controls
//----------------------------------------------------------------------------------
int GuiWaveform(Rectangle bounds, const char *label, GuiWaveBuffer *wave, Color color)
{
    float ts = (float)GuiMedGetStyle(MED_TEXT_SIZE);
    bool hovered = MedHovered(bounds);

    MedPanel(bounds, MedCol(MED_COLOR_PANEL), hovered? MedCol(MED_COLOR_FOCUS) : MedCol(MED_COLOR_BORDER));

    Rectangle plot = { bounds.x + 6, bounds.y + 6, bounds.width - 12, bounds.height - 12 };

    // ECG paper: 5 small squares per large square
    if (GuiMedGetStyle(MED_WAVE_GRID))
    {
        Color grid = MedCol(MED_COLOR_GRID);
        float cell = fmaxf(plot.height/12.0f, 4.0f);
        int n = 0;
        for (float x = plot.x; x <= plot.x + plot.width; x += cell, n++)
            DrawLineEx((Vector2){ x, plot.y }, (Vector2){ x, plot.y + plot.height }, (n % 5 == 0)? 1.5f : 1.0f, (n % 5 == 0)? grid : Fade(grid, 0.45f));
        n = 0;
        for (float y = plot.y + plot.height; y >= plot.y; y -= cell, n++)
            DrawLineEx((Vector2){ plot.x, y }, (Vector2){ plot.x + plot.width, y }, (n % 5 == 0)? 1.5f : 1.0f, (n % 5 == 0)? grid : Fade(grid, 0.45f));
    }

    if (!wave || wave->count < 2 || wave->capacity < 2)
    {
        MedTextAligned("NO SIGNAL", plot, ts, 1, 1, MedCol(MED_COLOR_TEXT_DIM));
    }
    else
    {
        float thick = (float)GuiMedGetStyle(MED_LINE_THICKNESS);
        float dx = plot.width/(float)(wave->capacity - 1);
        int gap = (int)ceilf((float)GuiMedGetStyle(MED_WAVE_GAP)/dx);
        // Samples written: before the first wrap only [0, count) are valid
        int valid = wave->count;

        for (int i = 1; i < wave->capacity; i++)
        {
            if (valid < wave->capacity && i >= valid) break;
            // Distance ahead of the sweep head: [0, gap) is the erase bar
            int ahead0 = (i - 1 - wave->head + wave->capacity) % wave->capacity;
            int ahead1 = (i - wave->head + wave->capacity) % wave->capacity;
            if (ahead0 < gap || ahead1 < gap) continue;

            Vector2 a = { plot.x + dx*(float)(i - 1), MedMapY(wave->samples[i - 1], plot, wave->minValue, wave->maxValue) };
            Vector2 b = { plot.x + dx*(float)i, MedMapY(wave->samples[i], plot, wave->minValue, wave->maxValue) };
            DrawLineEx(a, b, thick, color);
        }

        // Leading dot at the sweep position
        int last = (wave->head - 1 + wave->capacity) % wave->capacity;
        DrawCircleV((Vector2){ plot.x + dx*(float)last, MedMapY(wave->samples[last], plot, wave->minValue, wave->maxValue) }, thick*1.2f, color);
    }

    if (label)
    {
        Vector2 m = MedMeasure(label, ts);
        DrawRectangleRec((Rectangle){ bounds.x + 4, bounds.y + 4, m.x + 8, m.y + 4 }, Fade(MedCol(MED_COLOR_PANEL), 0.85f));
        MedText(label, bounds.x + 8, bounds.y + 6, ts, color);
    }

    return MedClicked(bounds)? 1 : 0;
}

int GuiVitalSign(Rectangle bounds, const char *label, const char *units, float value, int decimals, float lowLimit, float highLimit, Color color)
{
    float ts = (float)GuiMedGetStyle(MED_TEXT_SIZE);
    bool hovered = MedHovered(bounds);
    int state = GuiMedCheckLimits(value, lowLimit, highLimit);
    bool flashOn = (state != 0) && MedFlash(1.0f);

    Color fill = flashOn? MedCol(MED_COLOR_ALARM_MEDIUM) : MedCol(MED_COLOR_PANEL);
    Color valueColor = flashOn? MedCol(MED_COLOR_ALARM_TEXT) : (MedIsNaN(value)? MedCol(MED_COLOR_TEXT_DIM) : color);
    Color labelColor = flashOn? MedCol(MED_COLOR_ALARM_TEXT) : color;
    Color dimColor = flashOn? MedCol(MED_COLOR_ALARM_TEXT) : MedCol(MED_COLOR_TEXT_DIM);

    MedPanel(bounds, fill, hovered? MedCol(MED_COLOR_FOCUS) : ((state != 0)? MedCol(MED_COLOR_ALARM_MEDIUM) : MedCol(MED_COLOR_BORDER)));

    float x = bounds.x + 10, y = bounds.y + 8;
    if (label) { MedText(label, x, y, ts, labelColor); x += MedMeasure(label, ts).x + 8; }
    if (units) MedText(units, x, y + ts*0.15f, ts*0.8f, dimColor);

    // Limits column (high above low) on the right
    float ls = ts*0.8f;
    const char *hi = MedFormatLimit(highLimit);
    const char *lo = MedFormatLimit(lowLimit);
    float limW = fmaxf(MedMeasure(hi, ls).x, MedMeasure(lo, ls).x);
    Rectangle limRec = { bounds.x + bounds.width - limW - 10, bounds.y + 8, limW, ls };
    MedTextAligned(hi, limRec, ls, 2, 0, (state > 0)? labelColor : dimColor);
    limRec.y += ls + 2;
    MedTextAligned(lo, limRec, ls, 2, 0, (state < 0)? labelColor : dimColor);

    // Big value, right aligned below the header
    const char *text = MedFormatValue(value, decimals);
    Rectangle valRec = { bounds.x + 10, bounds.y + ts + 12, bounds.width - 20 - limW - 8, bounds.height - ts - 18 };
    float size = MedFitSize(text, valRec.width, valRec.height);
    valRec.width = bounds.width - 20;
    MedTextAligned(text, valRec, size, 2, 2, valueColor);

    return MedClicked(bounds)? 1 : 0;
}

int GuiBloodPressure(Rectangle bounds, const char *label, float systolic, float diastolic, float sysLowLimit, float sysHighLimit, Color color)
{
    float ts = (float)GuiMedGetStyle(MED_TEXT_SIZE);
    bool hovered = MedHovered(bounds);
    int state = GuiMedCheckLimits(systolic, sysLowLimit, sysHighLimit);
    bool flashOn = (state != 0) && MedFlash(1.0f);
    bool valid = !MedIsNaN(systolic) && !MedIsNaN(diastolic);

    Color fill = flashOn? MedCol(MED_COLOR_ALARM_MEDIUM) : MedCol(MED_COLOR_PANEL);
    Color fg = flashOn? MedCol(MED_COLOR_ALARM_TEXT) : color;
    Color dim = flashOn? MedCol(MED_COLOR_ALARM_TEXT) : MedCol(MED_COLOR_TEXT_DIM);

    MedPanel(bounds, fill, hovered? MedCol(MED_COLOR_FOCUS) : ((state != 0)? MedCol(MED_COLOR_ALARM_MEDIUM) : MedCol(MED_COLOR_BORDER)));

    float x = bounds.x + 10;
    if (label) { MedText(label, x, bounds.y + 8, ts, fg); x += MedMeasure(label, ts).x + 8; }
    MedText("mmHg", x, bounds.y + 8 + ts*0.15f, ts*0.8f, dim);

    float ls = ts*0.8f;
    const char *lim = TextFormat("Sys %s-%s", MedFormatLimit(sysLowLimit), MedFormatLimit(sysHighLimit));
    MedTextAligned(lim, (Rectangle){ bounds.x, bounds.y + 8, bounds.width - 10, ls }, ls, 2, 0, dim);

    char main[32];
    char mean[16];
    if (valid)
    {
        snprintf(main, sizeof(main), "%.0f/%.0f", systolic, diastolic);
        snprintf(mean, sizeof(mean), "(%.0f)", (systolic + 2.0f*diastolic)/3.0f);
    }
    else
    {
        snprintf(main, sizeof(main), "-?-/-?-");
        snprintf(mean, sizeof(mean), "(-?-)");
    }

    Rectangle area = { bounds.x + 10, bounds.y + ts + 12, bounds.width - 20, bounds.height - ts - 18 };
    float meanSize = area.height*0.42f;
    float meanW = MedMeasure(mean, meanSize).x;
    float size = MedFitSize(main, area.width - meanW - 12, area.height);
    meanSize = fminf(meanSize, size*0.6f);
    meanW = MedMeasure(mean, meanSize).x;

    MedTextAligned(mean, area, meanSize, 2, 2, fg);
    MedTextAligned(main, (Rectangle){ area.x, area.y, area.width - meanW - 10, area.height }, size, 2, 2, valid? fg : dim);

    return MedClicked(bounds)? 1 : 0;
}

int GuiTrendChart(Rectangle bounds, const char *label, const float *values, int count, float minValue, float maxValue, float lowLimit, float highLimit, Color color)
{
    bool hovered = MedHovered(bounds);
    Rectangle plot = MedChartFrame(bounds, label, count, minValue, maxValue, color, hovered);
    Color alarm = MedCol(MED_COLOR_ALARM_MEDIUM);

    // Out-of-limit bands
    if (!MedIsNaN(highLimit) && highLimit < maxValue)
    {
        float y = MedMapY(highLimit, plot, minValue, maxValue);
        DrawRectangleRec((Rectangle){ plot.x, plot.y, plot.width, y - plot.y }, Fade(alarm, 0.10f));
        MedDashedLineH(plot.x, plot.x + plot.width, y, 1, Fade(alarm, 0.7f));
    }
    if (!MedIsNaN(lowLimit) && lowLimit > minValue)
    {
        float y = MedMapY(lowLimit, plot, minValue, maxValue);
        DrawRectangleRec((Rectangle){ plot.x, y, plot.width, plot.y + plot.height - y }, Fade(alarm, 0.10f));
        MedDashedLineH(plot.x, plot.x + plot.width, y, 1, Fade(alarm, 0.7f));
    }

    if (values && count > 0)
    {
        float thick = (float)GuiMedGetStyle(MED_LINE_THICKNESS);
        for (int i = 1; i < count; i++)
        {
            if (MedIsNaN(values[i - 1]) || MedIsNaN(values[i])) continue;
            Vector2 a = { MedMapX(i - 1, count, plot), MedMapY(values[i - 1], plot, minValue, maxValue) };
            Vector2 b = { MedMapX(i, count, plot), MedMapY(values[i], plot, minValue, maxValue) };
            bool out = GuiMedCheckLimits(values[i], lowLimit, highLimit) != 0;
            DrawLineEx(a, b, thick, out? alarm : color);
        }
        // Isolated points (neighbours are gaps) still need to be visible
        for (int i = 0; i < count; i++)
        {
            if (MedIsNaN(values[i])) continue;
            bool prev = (i > 0) && !MedIsNaN(values[i - 1]);
            bool next = (i < count - 1) && !MedIsNaN(values[i + 1]);
            if (!prev && !next) DrawCircleV((Vector2){ MedMapX(i, count, plot), MedMapY(values[i], plot, minValue, maxValue) }, thick*1.5f, color);
        }
    }

    int index = MedHoverIndex(plot, count);
    if (index >= 0)
    {
        float x = MedMapX(index, count, plot);
        float v = values[index];
        if (!MedIsNaN(v)) DrawCircleV((Vector2){ x, MedMapY(v, plot, minValue, maxValue) }, 4, color);
        const char *ago = MedFormatAgo((float)(count - 1 - index)*(float)GuiMedGetStyle(MED_TREND_SAMPLE_SECONDS));
        char line1[32];
        snprintf(line1, sizeof(line1), "%s", MedIsNaN(v)? "no data" : TextFormat("%.1f", v));
        MedTooltip(x, plot, bounds, line1, ago, color);
    }

    return index;
}

int GuiBloodPressureTrend(Rectangle bounds, const char *label, const float *systolic, const float *diastolic, int count, float minValue, float maxValue, Color color)
{
    bool hovered = MedHovered(bounds);
    Rectangle plot = MedChartFrame(bounds, label, count, minValue, maxValue, color, hovered);
    float thick = (float)GuiMedGetStyle(MED_LINE_THICKNESS);
    float step = (count > 1)? plot.width/(float)(count - 1) : plot.width;
    float cap = MedClamp(step*0.35f, 3, 7);
    float lastX = 1e9f;

    // Newest first; skip measurements closer than a few pixels to the last one drawn
    for (int i = count - 1; i >= 0; i--)
    {
        if (!systolic || !diastolic || MedIsNaN(systolic[i]) || MedIsNaN(diastolic[i])) continue;
        float x = MedMapX(i, count, plot);
        if (lastX - x < 2*cap + 2) continue;
        lastX = x;
        float ys = MedMapY(systolic[i], plot, minValue, maxValue);
        float yd = MedMapY(diastolic[i], plot, minValue, maxValue);
        float ym = MedMapY((systolic[i] + 2.0f*diastolic[i])/3.0f, plot, minValue, maxValue);
        DrawLineEx((Vector2){ x, ys }, (Vector2){ x, yd }, thick*0.75f, Fade(color, 0.8f));
        DrawLineEx((Vector2){ x - cap, ys }, (Vector2){ x + cap, ys }, thick, color);
        DrawLineEx((Vector2){ x - cap, yd }, (Vector2){ x + cap, yd }, thick, color);
        DrawCircleV((Vector2){ x, ym }, thick*1.1f, MedCol(MED_COLOR_TEXT));
    }

    int index = MedHoverIndex(plot, count);
    if (index >= 0 && systolic && diastolic)
    {
        float x = MedMapX(index, count, plot);
        const char *ago = MedFormatAgo((float)(count - 1 - index)*(float)GuiMedGetStyle(MED_TREND_SAMPLE_SECONDS));
        char line1[40];
        if (MedIsNaN(systolic[index]) || MedIsNaN(diastolic[index])) snprintf(line1, sizeof(line1), "no data");
        else snprintf(line1, sizeof(line1), "%.0f/%.0f (%.0f)", systolic[index], diastolic[index], (systolic[index] + 2.0f*diastolic[index])/3.0f);
        MedTooltip(x, plot, bounds, line1, ago, color);
    }

    return index;
}

int GuiAlarmBanner(Rectangle bounds, const char *text, int priority)
{
    float ts = (float)GuiMedGetStyle(MED_TEXT_SIZE);
    bool active = (priority > MED_ALARM_NONE) && text && text[0];
    bool hovered = active && MedHovered(bounds);

    bool on = true;
    if (priority == MED_ALARM_HIGH) on = MedFlash(2.0f);
    else if (priority == MED_ALARM_MEDIUM) on = MedFlash(0.6f);

    Color fill = (active && on)? MedAlarmColor(priority) : MedCol(MED_COLOR_PANEL);
    Color border = active? MedAlarmColor(priority) : MedCol(MED_COLOR_BORDER);
    Color fg = (active && on)? MedCol(MED_COLOR_ALARM_TEXT) : (active? MedAlarmColor(priority) : MedCol(MED_COLOR_TEXT_DIM));

    MedPanel(bounds, fill, hovered? MedCol(MED_COLOR_FOCUS) : border);

    float icon = fminf(bounds.height - 12, ts*1.6f);
    MedDrawBell((Rectangle){ bounds.x + 10, bounds.y + (bounds.height - icon)/2, icon, icon }, fg);

    const char *prefix = (priority == MED_ALARM_HIGH)? "***" : ((priority == MED_ALARM_MEDIUM)? "**" : "*");
    const char *msg = active? TextFormat("%s %s", prefix, text) : ((text && text[0])? text : "No active alarms");
    Rectangle textRec = { bounds.x + icon + 22, bounds.y, bounds.width - icon - 32, bounds.height };
    float size = MedFitSize(msg, textRec.width, fminf(bounds.height*0.55f, ts*1.4f));
    MedTextAligned(msg, textRec, size, 0, 1, fg);

    return (active && MedClicked(bounds))? 1 : 0;
}

int GuiBarGauge(Rectangle bounds, const char *label, float value, float minValue, float maxValue, float lowLimit, float highLimit, Color color)
{
    float ts = (float)GuiMedGetStyle(MED_TEXT_SIZE);
    bool hovered = MedHovered(bounds);
    bool vertical = bounds.height > bounds.width;
    int state = GuiMedCheckLimits(value, lowLimit, highLimit);
    Color alarm = MedCol(MED_COLOR_ALARM_MEDIUM);
    Color dim = MedCol(MED_COLOR_TEXT_DIM);
    Color fillColor = (state != 0)? alarm : color;

    MedPanel(bounds, MedCol(MED_COLOR_PANEL), hovered? MedCol(MED_COLOR_FOCUS) : MedCol(MED_COLOR_BORDER));

    const char *valText = MedFormatValue(value, (fabsf(maxValue - minValue) < 20)? 1 : 0);
    float range = (maxValue != minValue)? maxValue - minValue : 1;
    float t = MedIsNaN(value)? 0 : MedClamp((value - minValue)/range, 0, 1);

    if (vertical)
    {
        if (label) MedTextAligned(label, (Rectangle){ bounds.x, bounds.y + 6, bounds.width, ts }, MedFitSize(label, bounds.width - 8, ts), 1, 0, color);
        float vs = MedFitSize(valText, bounds.width - 8, ts*1.5f);
        MedTextAligned(valText, (Rectangle){ bounds.x, bounds.y + bounds.height - vs - 6, bounds.width, vs }, vs, 1, 0, (state != 0)? alarm : MedCol(MED_COLOR_TEXT));

        float tw = fminf(bounds.width*0.4f, 28);
        Rectangle track = { bounds.x + (bounds.width - tw)/2, bounds.y + ts + 14, tw, bounds.height - ts - vs - 26 };
        DrawRectangleRec(track, MedCol(MED_COLOR_GRID));
        float fh = track.height*t;
        DrawRectangleRec((Rectangle){ track.x, track.y + track.height - fh, track.width, fh }, fillColor);

        for (int i = 0; i <= 4; i++)
        {
            float y = track.y + track.height*(float)i/4.0f;
            DrawLineEx((Vector2){ track.x - 5, y }, (Vector2){ track.x, y }, 1, dim);
        }
        float lims[2] = { lowLimit, highLimit };
        for (int i = 0; i < 2; i++)
        {
            if (MedIsNaN(lims[i])) continue;
            float y = track.y + track.height*(1.0f - MedClamp((lims[i] - minValue)/range, 0, 1));
            float x = track.x + track.width + 2;
            DrawTriangle((Vector2){ x, y }, (Vector2){ x + 7, y + 5 }, (Vector2){ x + 7, y - 5 }, alarm);
            DrawLineEx((Vector2){ track.x, y }, (Vector2){ track.x + track.width, y }, 1, alarm);
        }
    }
    else
    {
        float vs = fminf(bounds.height*0.5f, ts*1.4f);
        float vw = MedMeasure(valText, vs).x;
        if (label) MedText(label, bounds.x + 10, bounds.y + 6, ts, color);
        MedTextAligned(valText, (Rectangle){ bounds.x, bounds.y + 4, bounds.width - 10, vs }, vs, 2, 0, (state != 0)? alarm : MedCol(MED_COLOR_TEXT));

        float th = fminf(bounds.height*0.3f, 16);
        Rectangle track = { bounds.x + 10, bounds.y + bounds.height - th - 12, bounds.width - 20, th };
        if (track.y < bounds.y + vs + 8) track.y = bounds.y + vs + 8;
        (void)vw;
        DrawRectangleRec(track, MedCol(MED_COLOR_GRID));
        DrawRectangleRec((Rectangle){ track.x, track.y, track.width*t, track.height }, fillColor);

        float lims[2] = { lowLimit, highLimit };
        for (int i = 0; i < 2; i++)
        {
            if (MedIsNaN(lims[i])) continue;
            float x = track.x + track.width*MedClamp((lims[i] - minValue)/range, 0, 1);
            float y = track.y - 2;
            DrawTriangle((Vector2){ x, y }, (Vector2){ x - 5, y - 7 }, (Vector2){ x + 5, y - 7 }, alarm);
            DrawLineEx((Vector2){ x, track.y }, (Vector2){ x, track.y + track.height }, 1, alarm);
        }
    }

    return MedClicked(bounds)? 1 : 0;
}

int GuiAlarmLimits(Rectangle bounds, const char *label, float *lowLimit, float *highLimit, float minValue, float maxValue, float step, float current, Color color)
{
    float ts = (float)GuiMedGetStyle(MED_TEXT_SIZE);
    float small = ts*0.8f;
    int result = 0;
    bool hovered = MedHovered(bounds);
    float range = (maxValue != minValue)? maxValue - minValue : 1;
    if (step <= 0) step = range/100.0f;

    MedPanel(bounds, MedCol(MED_COLOR_PANEL), (hovered || (medDragThumb >= 0 && MedRecEquals(medDragRec, bounds)))? MedCol(MED_COLOR_FOCUS) : MedCol(MED_COLOR_BORDER));
    if (label) MedText(label, bounds.x + 10, bounds.y + 6, ts, color);

    float knob = fminf(ts*0.75f, 11);
    Rectangle track = { bounds.x + 14 + knob, bounds.y + ts + 16 + small, bounds.width - 28 - 2*knob, 6 };
    if (track.y + 6 + knob + small > bounds.y + bounds.height) track.y = bounds.y + bounds.height - knob - small - 8;

    // Interaction: grab the nearest thumb, drag until release
    if (MedInteractive() && lowLimit && highLimit)
    {
        Vector2 mouse = GetMousePosition();
        float xLo = track.x + track.width*MedClamp((*lowLimit - minValue)/range, 0, 1);
        float xHi = track.x + track.width*MedClamp((*highLimit - minValue)/range, 0, 1);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, bounds))
        {
            int thumb;
            if (mouse.x <= xLo) thumb = 0;
            else if (mouse.x >= xHi) thumb = 1;
            else thumb = (fabsf(mouse.x - xLo) <= fabsf(mouse.x - xHi))? 0 : 1;
            medDragRec = bounds;
            medDragThumb = thumb;
        }

        if (medDragThumb >= 0 && MedRecEquals(medDragRec, bounds))
        {
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
            {
                float v = minValue + range*MedClamp((mouse.x - track.x)/track.width, 0, 1);
                v = minValue + roundf((v - minValue)/step)*step;
                if (medDragThumb == 0)
                {
                    v = MedClamp(v, minValue, *highLimit - step);
                    if (v != *lowLimit) { *lowLimit = v; result = 1; }
                }
                else
                {
                    v = MedClamp(v, *lowLimit + step, maxValue);
                    if (v != *highLimit) { *highLimit = v; result = 1; }
                }
            }
            else medDragThumb = -1;
        }
    }

    float lo = lowLimit? *lowLimit : minValue;
    float hi = highLimit? *highLimit : maxValue;
    float xLo = track.x + track.width*MedClamp((lo - minValue)/range, 0, 1);
    float xHi = track.x + track.width*MedClamp((hi - minValue)/range, 0, 1);
    Color alarm = MedCol(MED_COLOR_ALARM_MEDIUM);

    // Track: alarm zones outside, parameter color inside
    DrawRectangleRec((Rectangle){ track.x, track.y, xLo - track.x, track.height }, Fade(alarm, 0.55f));
    DrawRectangleRec((Rectangle){ xHi, track.y, track.x + track.width - xHi, track.height }, Fade(alarm, 0.55f));
    DrawRectangleRec((Rectangle){ xLo, track.y, xHi - xLo, track.height }, Fade(color, 0.6f));

    // Scale end labels
    MedText(MedFormatLimit(minValue), track.x - knob, track.y + track.height + knob + 2, small, MedCol(MED_COLOR_TEXT_DIM));
    MedTextAligned(MedFormatLimit(maxValue), (Rectangle){ track.x, track.y + track.height + knob + 2, track.width + knob, small }, small, 2, 0, MedCol(MED_COLOR_TEXT_DIM));

    // Current value marker
    if (!MedIsNaN(current))
    {
        float xc = track.x + track.width*MedClamp((current - minValue)/range, 0, 1);
        bool out = GuiMedCheckLimits(current, lo, hi) != 0;
        Color mc = out? alarm : MedCol(MED_COLOR_TEXT);
        DrawTriangle((Vector2){ xc, track.y - 3 }, (Vector2){ xc - 6, track.y - 12 }, (Vector2){ xc + 6, track.y - 12 }, mc);
    }

    // Thumbs with value labels
    float xs[2] = { xLo, xHi };
    float vs[2] = { lo, hi };
    for (int i = 0; i < 2; i++)
    {
        bool active = (medDragThumb == i) && MedRecEquals(medDragRec, bounds);
        Vector2 c = { xs[i], track.y + track.height/2 };
        DrawCircleV(c, knob, active? MedCol(MED_COLOR_TEXT) : color);
        DrawCircleV(c, knob*0.45f, MedCol(MED_COLOR_PANEL));
    }
    char loText[16], hiBuf[16];
    snprintf(loText, sizeof(loText), "%s", MedFormatLimit(vs[0]));
    snprintf(hiBuf, sizeof(hiBuf), "%s", MedFormatLimit(vs[1]));
    float wLo = MedMeasure(loText, small).x, wHi = MedMeasure(hiBuf, small).x;
    float lx = xLo - wLo/2, hx = xHi - wHi/2;
    if (hx < lx + wLo + 6) { float mid = (xLo + xHi)/2; lx = mid - wLo - 3; hx = mid + 3; }
    float headerRight = bounds.x + 10 + (label? MedMeasure(label, ts).x + 10 : 0);
    float ly = track.y - knob - small - 2;
    if (ly < bounds.y + 6 && lx < headerRight) ly = bounds.y + 6;
    MedText(loText, lx, ly, small, MedCol(MED_COLOR_TEXT));
    MedText(hiBuf, hx, ly, small, MedCol(MED_COLOR_TEXT));

    return result;
}

int GuiPainScale(Rectangle bounds, int *value)
{
    float ts = (float)GuiMedGetStyle(MED_TEXT_SIZE);
    int result = 0;
    int current = value? *value : -1;
    static const char *words[] = { "No pain", "Mild", "Mild", "Mild", "Moderate", "Moderate", "Moderate", "Severe", "Severe", "Severe", "Worst possible" };

    MedPanel(bounds, MedCol(MED_COLOR_PANEL), MedCol(MED_COLOR_BORDER));

    Rectangle inner = { bounds.x + 10, bounds.y + 8, bounds.width - 20, bounds.height - 16 };
    float cellW = inner.width/11.0f;
    float numH = fminf(cellW*0.9f, ts*2.2f);
    float wordH = ts;
    float faceSpace = inner.height - numH - wordH - 12;
    float faceR = fminf(fminf(cellW*0.95f, faceSpace*0.5f), inner.width/12.0f);
    bool drawFaces = faceR >= 8;

    Color green = { 46, 190, 110, 255 }, yellow = { 240, 190, 20, 255 }, red = { 225, 60, 50, 255 };
    Rectangle row = { inner.x, inner.y + (drawFaces? faceR*2 + 8 : 0), inner.width, numH };

    for (int i = 0; i <= 10; i++)
    {
        float t = (float)i/10.0f;
        Color c = (t < 0.5f)? ColorLerp(green, yellow, t*2) : ColorLerp(yellow, red, (t - 0.5f)*2);
        Rectangle cell = { row.x + cellW*(float)i + 2, row.y, cellW - 4, row.height };
        bool hover = MedHovered(cell);
        bool selected = (i == current);

        DrawRectangleRec(cell, (selected || hover)? c : Fade(c, 0.28f));
        if (selected) DrawRectangleLinesEx((Rectangle){ cell.x - 2, cell.y - 2, cell.width + 4, cell.height + 4 }, 2, MedCol(MED_COLOR_TEXT));
        const char *n = TextFormat("%d", i);
        MedTextAligned(n, cell, MedFitSize(n, cell.width - 4, cell.height*0.6f), 1, 1, selected? MedCol(MED_COLOR_ALARM_TEXT) : MedCol(MED_COLOR_TEXT));

        if (hover && IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && value && *value != i) { *value = i; result = 1; }

        // Face above every even value: mouth curvature goes from smile to frown
        if (drawFaces && (i % 2 == 0))
        {
            Vector2 fc = { MedClamp(cell.x + cell.width/2, inner.x + faceR, inner.x + inner.width - faceR), inner.y + faceR };
            Color fcol = selected? c : Fade(c, 0.75f);
            DrawCircleV(fc, faceR, fcol);
            Color ink = { 30, 30, 30, 255 };
            float eye = fmaxf(faceR*0.1f, 1.5f);
            DrawCircleV((Vector2){ fc.x - faceR*0.35f, fc.y - faceR*0.2f }, eye, ink);
            DrawCircleV((Vector2){ fc.x + faceR*0.35f, fc.y - faceR*0.2f }, eye, ink);
            float curve = 1.0f - t*2.0f;          // 1 smile .. -1 frown
            float mw = faceR*0.45f, mh = faceR*0.28f, my = fc.y + faceR*0.38f - curve*mh*0.5f;
            Vector2 prev = { fc.x - mw, my };
            for (int s = 1; s <= 8; s++)
            {
                float u = -1.0f + 2.0f*(float)s/8.0f;
                Vector2 p = { fc.x + u*mw, my + curve*mh*(1.0f - u*u) };
                DrawLineEx(prev, p, fmaxf(faceR*0.09f, 1.5f), ink);
                prev = p;
            }
            if (i == 10) DrawCircleV((Vector2){ fc.x + faceR*0.4f, fc.y + faceR*0.12f }, eye*1.2f, (Color){ 80, 170, 255, 255 });
        }
    }

    const char *caption = (current >= 0 && current <= 10)? TextFormat("%d - %s", current, words[current]) : "Tap to rate pain 0-10";
    MedTextAligned(caption, (Rectangle){ inner.x, row.y + row.height + 6, inner.width, wordH }, wordH, 1, 0, MedCol(MED_COLOR_TEXT_DIM));

    return result;
}

int GuiInfusion(Rectangle bounds, const char *drug, float rateMlPerHour, float infusedMl, float volumeMl, bool running)
{
    float ts = (float)GuiMedGetStyle(MED_TEXT_SIZE);
    bool hovered = MedHovered(bounds);
    bool complete = (volumeMl > 0) && (infusedMl >= volumeMl);
    Color accent = complete? MedCol(MED_COLOR_ALARM_LOW) : (running? MedCol(MED_COLOR_ECG) : MedCol(MED_COLOR_ALARM_MEDIUM));
    Color dim = MedCol(MED_COLOR_TEXT_DIM);

    MedPanel(bounds, MedCol(MED_COLOR_PANEL), hovered? MedCol(MED_COLOR_FOCUS) : MedCol(MED_COLOR_BORDER));

    // Drip chamber icon on the left
    float iw = fminf(bounds.height*0.35f, 30);
    Rectangle chamber = { bounds.x + 10, bounds.y + 10, iw, bounds.height - 20 };
    float chH = fminf(chamber.height, iw*2.2f);
    chamber.y += (chamber.height - chH)/2;
    chamber.height = chH;
    DrawRectangleLinesEx(chamber, 2, dim);
    float fluidH = chamber.height*0.3f;
    DrawRectangleRec((Rectangle){ chamber.x + 2, chamber.y + chamber.height - fluidH, chamber.width - 4, fluidH - 2 }, Fade(accent, 0.5f));
    if (running && !complete && rateMlPerHour > 0)
    {
        float period = MedClamp(3600.0f/(rateMlPerHour*20.0f), 0.2f, 3.0f);   // 20 drops per mL
        float phase = fmodf((float)GetTime(), period)/period;
        float fall = chamber.height - fluidH - 8;
        float dy = chamber.y + 6 + fall*phase*phase;
        DrawCircleV((Vector2){ chamber.x + chamber.width/2, dy }, iw*0.13f, accent);
    }

    float x = chamber.x + chamber.width + 12;
    float w = bounds.x + bounds.width - x - 10;

    MedText(drug? drug : "Infusion", x, bounds.y + 8, ts, MedCol(MED_COLOR_TEXT));
    const char *status = complete? "COMPLETE" : (running? "RUNNING" : "PAUSED");
    bool statusOn = running || complete || MedFlash(0.6f);
    if (statusOn) MedTextAligned(status, (Rectangle){ x, bounds.y + 8, w, ts }, ts*0.8f, 2, 0, accent);

    // Rate
    const char *rate = TextFormat("%.1f", rateMlPerHour);
    float rs = fminf(bounds.height*0.34f, ts*2.4f);
    MedText(rate, x, bounds.y + ts + 12, rs, MedCol(MED_COLOR_TEXT));
    MedText("mL/h", x + MedMeasure(rate, rs).x + 6, bounds.y + ts + 12 + rs - ts*0.8f - 2, ts*0.8f, dim);

    // Time remaining
    if (!complete && rateMlPerHour > 0 && volumeMl > 0)
    {
        float hours = (volumeMl - infusedMl)/rateMlPerHour;
        int mins = (int)(hours*60.0f + 0.5f);
        MedTextAligned(TextFormat("%dh %02dm left", mins/60, mins%60), (Rectangle){ x, bounds.y + ts + 12, w, rs }, ts*0.8f, 2, 2, dim);
    }

    // Progress
    float t = (volumeMl > 0)? MedClamp(infusedMl/volumeMl, 0, 1) : 0;
    float barY = bounds.y + bounds.height - ts*0.8f - 18;
    Rectangle bar = { x, barY, w, 6 };
    DrawRectangleRec(bar, MedCol(MED_COLOR_GRID));
    DrawRectangleRec((Rectangle){ bar.x, bar.y, bar.width*t, bar.height }, accent);
    MedText(TextFormat("%.0f / %.0f mL", infusedMl, volumeMl), x, barY + 10, ts*0.8f, dim);

    return MedClicked(bounds)? 1 : 0;
}

int GuiPatientBanner(Rectangle bounds, const char *name, const char *details, const char *allergies)
{
    float ts = (float)GuiMedGetStyle(MED_TEXT_SIZE);
    bool hovered = MedHovered(bounds);
    bool hasAllergy = allergies && allergies[0];

    MedPanel(bounds, MedCol(MED_COLOR_PANEL), hovered? MedCol(MED_COLOR_FOCUS) : MedCol(MED_COLOR_BORDER));

    const char *pill = hasAllergy? TextFormat("ALLERGY: %s", allergies) : "NKDA";
    char pillBuf[128];
    snprintf(pillBuf, sizeof(pillBuf), "%s", pill);
    float ps = ts*0.9f;
    float pw = MedMeasure(pillBuf, ps).x + 20;
    float ph = fminf(bounds.height - 10, ps + 10);
    Rectangle pr = { bounds.x + bounds.width - pw - 8, bounds.y + (bounds.height - ph)/2, pw, ph };
    if (hasAllergy)
    {
        DrawRectangleRounded(pr, 0.5f, 6, MedCol(MED_COLOR_ALARM_HIGH));
        MedTextAligned(pillBuf, pr, ps, 1, 1, (Color){ 255, 255, 255, 255 });
    }
    else
    {
        DrawRectangleRoundedLinesEx(pr, 0.5f, 6, 1, MedCol(MED_COLOR_TEXT_DIM));
        MedTextAligned(pillBuf, pr, ps, 1, 1, MedCol(MED_COLOR_TEXT_DIM));
    }

    float ns = fminf(bounds.height*0.5f, ts*1.4f);
    float x = bounds.x + 12;
    if (name)
    {
        MedTextAligned(name, (Rectangle){ x, bounds.y, pr.x - x, bounds.height }, ns, 0, 1, MedCol(MED_COLOR_TEXT));
        x += MedMeasure(name, ns).x + 16;
    }
    if (details && x < pr.x - 20)
        MedTextAligned(details, (Rectangle){ x, bounds.y, pr.x - x - 10, bounds.height }, MedFitSize(details, pr.x - x - 10, ts), 0, 1, MedCol(MED_COLOR_TEXT_DIM));

    return MedClicked(bounds)? 1 : 0;
}

#endif // RAYMED_IMPLEMENTATION
