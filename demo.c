/*******************************************************************************************
*
*   raymed demo - simulated bedside monitor using raymed widgets alongside raygui controls
*
*   Keys: 1/2/3 switch pages, D desaturation event, L toggle light theme, F11 fullscreen
*   Run with --shot <page> <file.png> [dlg] to render a few seconds and save a screenshot
*   (flags: d = desaturation event, l = light theme, g = ECG paper grid).
*
********************************************************************************************/

#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define RAYMED_IMPLEMENTATION
#include "raymed.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define WAVE_RATE       125         // Samples per second for ECG / ART / pleth
#define WAVE_SECONDS    6
#define RESP_RATE       25
#define RESP_SECONDS    12
#define TREND_COUNT     240         // 4 h of 1-minute samples
#define TREND_TICK      1.0f        // Real seconds per simulated trend minute

typedef struct Patient {
    float hr, spo2, rr, temp, etco2, artSys, artDia;
    float hrTarget, spo2Target, rrTarget;
    float nibpSys, nibpDia;          // Last cuff measurement
    float beatPhase, breathPhase;
    float desatTimer;
} Patient;

typedef struct Limits {
    float hrLo, hrHi, spo2Lo, spo2Hi, rrLo, rrHi, sysLo, sysHi, tempLo, tempHi;
} Limits;

typedef struct Pump {
    const char *drug;
    float rate, infused, volume;
    bool running;
} Pump;

static float Frand(void) { return (float)GetRandomValue(-1000, 1000)/1000.0f; }
static float Gauss(float x, float c, float w) { float d = (x - c)/w; return expf(-0.5f*d*d); }

static float EcgShape(float p)
{
    return 0.15f*Gauss(p, 0.16f, 0.025f) - 0.12f*Gauss(p, 0.235f, 0.010f) + 1.10f*Gauss(p, 0.25f, 0.012f)
         - 0.25f*Gauss(p, 0.27f, 0.012f) + 0.30f*Gauss(p, 0.45f, 0.045f);
}

static float PulseShape(float p, float delay)
{
    float q = fmodf(p - delay + 1.0f, 1.0f);
    return Gauss(q, 0.12f, 0.06f) + 0.32f*Gauss(q, 0.36f, 0.07f) + 0.12f*Gauss(q, 0.60f, 0.15f);
}

static void PushTrend(float *arr, float v)
{
    memmove(arr, arr + 1, sizeof(float)*(TREND_COUNT - 1));
    arr[TREND_COUNT - 1] = v;
}

static void ApplyRayguiTheme(bool light)
{
    // Mirror the raymed palette onto raygui so both sets of controls match
    Color panel = GuiMedColor(MED_COLOR_PANEL), border = GuiMedColor(MED_COLOR_BORDER);
    Color focus = GuiMedColor(MED_COLOR_FOCUS), text = GuiMedColor(MED_COLOR_TEXT);
    Color accent = GuiMedColor(MED_COLOR_SPO2), bg = GuiMedColor(MED_COLOR_BACKGROUND);

    GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, ColorToInt(border));
    GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, ColorToInt(panel));
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(GuiMedColor(MED_COLOR_TEXT_DIM)));
    GuiSetStyle(DEFAULT, BORDER_COLOR_FOCUSED, ColorToInt(focus));
    GuiSetStyle(DEFAULT, BASE_COLOR_FOCUSED, ColorToInt(light? (Color){ 230, 236, 244, 255 } : (Color){ 24, 32, 44, 255 }));
    GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED, ColorToInt(text));
    GuiSetStyle(DEFAULT, BORDER_COLOR_PRESSED, ColorToInt(accent));
    GuiSetStyle(DEFAULT, BASE_COLOR_PRESSED, ColorToInt(Fade(accent, 0.25f)));
    GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED, ColorToInt(text));
    GuiSetStyle(DEFAULT, LINE_COLOR, ColorToInt(border));
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, ColorToInt(bg));
    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
    GuiSetStyle(DEFAULT, TEXT_SPACING, 1);
}

int main(int argc, char **argv)
{
    int shotPage = -1;
    const char *shotFile = NULL;
    const char *shotFlags = "";     // d = desaturation event, l = light theme, g = ECG grid
    if (argc >= 4 && strcmp(argv[1], "--shot") == 0) { shotPage = atoi(argv[2]); shotFile = argv[3]; if (argc >= 5) shotFlags = argv[4]; }

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(1360, 860, "raymed - medical widgets for raylib");
    SetTargetFPS(60);
    SetRandomSeed(7);

    // Bold sans for crisp numerics; the first one found wins, else raylib's default font
    static const char *fontPaths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "C:/Windows/Fonts/segoeuib.ttf",
        "C:/Windows/Fonts/arialbd.ttf",
        "/System/Library/Fonts/Supplemental/Arial Bold.ttf",
    };
    Font font = { 0 };
    for (int i = 0; i < (int)(sizeof(fontPaths)/sizeof(fontPaths[0])) && font.texture.id == 0; i++)
        if (FileExists(fontPaths[i])) font = LoadFontEx(fontPaths[i], 96, NULL, 0);
    if (font.texture.id > 0)
    {
        GenTextureMipmaps(&font.texture);
        SetTextureFilter(font.texture, TEXTURE_FILTER_TRILINEAR);
        GuiMedSetFont(font);
        GuiSetFont(font);
    }

    bool lightTheme = false, appliedLight = false, waveGrid = false, flashing = true;
    GuiMedLoadStyleDark();
    ApplyRayguiTheme(false);

    // Waveform buffers (caller-owned storage)
    static float ecgData[WAVE_RATE*WAVE_SECONDS], artData[WAVE_RATE*WAVE_SECONDS], plethData[WAVE_RATE*WAVE_SECONDS], respData[RESP_RATE*RESP_SECONDS];
    GuiWaveBuffer ecg, art, pleth, resp;
    GuiWaveBufferInit(&ecg, ecgData, WAVE_RATE*WAVE_SECONDS, -0.6f, 1.4f);
    GuiWaveBufferInit(&art, artData, WAVE_RATE*WAVE_SECONDS, 40.0f, 180.0f);
    GuiWaveBufferInit(&pleth, plethData, WAVE_RATE*WAVE_SECONDS, -0.1f, 1.3f);
    GuiWaveBufferInit(&resp, respData, RESP_RATE*RESP_SECONDS, -1.3f, 1.3f);

    Patient pt = { 78, 97, 16, 37.1f, 36, 124, 72, 78, 97, 16, 122, 74, 0, 0, 0 };
    Limits lim = { 50, 120, 90, 100, 8, 28, 90, 160, 35.5f, 38.5f };
    Pump pumps[3] = {
        { "Noradrenaline 4mg/50mL", 6.5f, 31.0f, 50.0f, true },
        { "Propofol 1%", 12.0f, 88.0f, 100.0f, true },
        { "Normal Saline 0.9%", 83.0f, 978.0f, 1000.0f, true },
    };

    // Trend history with a desaturation episode ~2.5 h ago and a probe-off gap
    static float tHr[TREND_COUNT], tSpo2[TREND_COUNT], tRr[TREND_COUNT], tSys[TREND_COUNT], tDia[TREND_COUNT];
    for (int i = 0; i < TREND_COUNT; i++)
    {
        float m = (float)i;
        float ep = Gauss(m, 90, 9);
        tHr[i] = 82 - m*0.02f + 30*ep + 3*sinf(m*0.13f) + 1.5f*Frand();
        tSpo2[i] = 97 - 11*ep + 0.8f*Frand();
        tRr[i] = 15 + 9*ep + 1.2f*sinf(m*0.2f) + Frand();
        if (i >= 150 && i < 162) tSpo2[i] = NAN;
        bool cuff = (i % 5 == 0);
        tSys[i] = cuff? 132 - m*0.05f + 18*ep + 4*Frand() : NAN;
        tDia[i] = cuff? 78 - m*0.02f + 8*ep + 3*Frand() : NAN;
    }

    int page = (shotPage >= 0)? shotPage : 0;
    int pain = 3;
    float trendTimer = 0, nibpTimer = 0, ackTimer = 0, simTime = 0;
    float ecgAcc = 0, respAcc = 0;
    int frame = 0;
    if (strchr(shotFlags, 'd')) pt.desatTimer = 25;
    if (strchr(shotFlags, 'l')) lightTheme = true;
    if (strchr(shotFlags, 'g')) waveGrid = true;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        if (dt > 0.1f) dt = 0.1f;
        simTime += dt;

        // Input
        if (IsKeyPressed(KEY_ONE)) page = 0;
        if (IsKeyPressed(KEY_TWO)) page = 1;
        if (IsKeyPressed(KEY_THREE)) page = 2;
        if (IsKeyPressed(KEY_D)) pt.desatTimer = 25;
        if (IsKeyPressed(KEY_F11)) ToggleFullscreen();
        bool themeWas = lightTheme;
        if (IsKeyPressed(KEY_L)) lightTheme = !lightTheme;

        // Physiology: drift toward targets, desaturation event overrides
        float spo2Goal = pt.spo2Target, hrGoal = pt.hrTarget, rrGoal = pt.rrTarget;
        if (pt.desatTimer > 0) { pt.desatTimer -= dt; spo2Goal = 83; hrGoal = pt.hrTarget + 38; rrGoal = pt.rrTarget + 12; }
        pt.hr += (hrGoal - pt.hr)*dt*0.35f + Frand()*dt*3;
        pt.spo2 += (spo2Goal - pt.spo2)*dt*0.25f + Frand()*dt*0.8f;
        if (pt.spo2 > 100) pt.spo2 = 100;
        pt.rr += (rrGoal - pt.rr)*dt*0.3f + Frand()*dt*0.8f;
        pt.temp += (37.1f - pt.temp)*dt*0.05f + Frand()*dt*0.02f;
        pt.artSys = 118 + (pt.hr - 78)*0.35f + 3*sinf(simTime*0.4f);
        pt.artDia = 72 + (pt.hr - 78)*0.15f;
        pt.etco2 = 36 + (pt.rr - 16)*-0.6f + Frand()*0.3f;

        // Waveform generation
        ecgAcc += dt*WAVE_RATE;
        while (ecgAcc >= 1)
        {
            ecgAcc -= 1;
            pt.beatPhase = fmodf(pt.beatPhase + pt.hr/60.0f/WAVE_RATE, 1.0f);
            float wander = 0.04f*sinf(simTime*1.7f);
            GuiWaveBufferPush(&ecg, EcgShape(pt.beatPhase) + wander + 0.015f*Frand());
            GuiWaveBufferPush(&art, pt.artDia + (pt.artSys - pt.artDia)*PulseShape(pt.beatPhase, 0.12f));
            GuiWaveBufferPush(&pleth, PulseShape(pt.beatPhase, 0.28f)*(0.6f + 0.4f*(pt.spo2 - 80)/20.0f));
        }
        respAcc += dt*RESP_RATE;
        while (respAcc >= 1)
        {
            respAcc -= 1;
            pt.breathPhase = fmodf(pt.breathPhase + pt.rr/60.0f/RESP_RATE, 1.0f);
            GuiWaveBufferPush(&resp, sinf(pt.breathPhase*2*PI) + 0.03f*Frand());
        }

        // Trends and cuff (accelerated: 1 real second = 1 simulated minute)
        trendTimer += dt;
        if (trendTimer >= TREND_TICK)
        {
            trendTimer -= TREND_TICK;
            nibpTimer += 1;
            bool cuff = nibpTimer >= 5;
            if (cuff) { nibpTimer = 0; pt.nibpSys = pt.artSys + 4*Frand(); pt.nibpDia = pt.artDia + 3*Frand(); }
            PushTrend(tHr, pt.hr); PushTrend(tSpo2, pt.spo2); PushTrend(tRr, pt.rr);
            PushTrend(tSys, cuff? pt.nibpSys : NAN); PushTrend(tDia, cuff? pt.nibpDia : NAN);
        }

        for (int i = 0; i < 3; i++)
        {
            Pump *p = &pumps[i];
            if (p->running && p->infused < p->volume) p->infused = fminf(p->volume, p->infused + p->rate*dt/60.0f);  // accelerated 60x
        }

        // Alarm evaluation: pick the highest priority active condition
        int alarmPriority = MED_ALARM_NONE, alarmCount = 0;
        char alarmText[96] = "";
        #define RAISE(pri, ...) do { alarmCount++; if ((pri) > alarmPriority) { alarmPriority = (pri); snprintf(alarmText, sizeof(alarmText), __VA_ARGS__); } } while (0)
        if (pt.spo2 < 85) RAISE(MED_ALARM_HIGH, "Desat  SpO2 %.0f", pt.spo2);
        else if (GuiMedCheckLimits(pt.spo2, lim.spo2Lo, lim.spo2Hi)) RAISE(MED_ALARM_MEDIUM, "SpO2 %.0f %s", pt.spo2, pt.spo2 < lim.spo2Lo? "< low limit" : "> high limit");
        if (pt.hr > 150) RAISE(MED_ALARM_HIGH, "Extreme tachy  HR %.0f", pt.hr);
        else if (GuiMedCheckLimits(pt.hr, lim.hrLo, lim.hrHi)) RAISE(MED_ALARM_MEDIUM, "HR %.0f %s", pt.hr, pt.hr < lim.hrLo? "< low limit" : "> high limit");
        if (GuiMedCheckLimits(pt.rr, lim.rrLo, lim.rrHi)) RAISE(MED_ALARM_MEDIUM, "RR %.0f out of limits", pt.rr);
        if (GuiMedCheckLimits(pt.nibpSys, lim.sysLo, lim.sysHi)) RAISE(MED_ALARM_MEDIUM, "NIBP sys %.0f out of limits", pt.nibpSys);
        for (int i = 0; i < 3; i++)
        {
            if (pumps[i].infused >= pumps[i].volume) RAISE(MED_ALARM_LOW, "Pump %d infusion complete", i + 1);
            else if (!pumps[i].running) RAISE(MED_ALARM_LOW, "Pump %d paused", i + 1);
        }
        if (alarmCount > 1) { size_t n = strlen(alarmText); snprintf(alarmText + n, sizeof(alarmText) - n, "   (+%d more)", alarmCount - 1); }
        if (ackTimer > 0)
        {
            ackTimer -= dt;
            if (alarmPriority < MED_ALARM_HIGH) snprintf(alarmText, sizeof(alarmText), "Alarm audio paused %d:%02d", (int)ackTimer/60, (int)ackTimer%60), alarmPriority = MED_ALARM_NONE;
        }

        // Draw
        BeginDrawing();
        ClearBackground(GuiMedColor(MED_COLOR_BACKGROUND));

        float W = (float)GetScreenWidth(), H = (float)GetScreenHeight();
        const float pad = 8, gap = 8;

        GuiPatientBanner((Rectangle){ pad, pad, W - 2*pad - 3*130 - gap, 44 }, "DOE, Jordan", "MRN 0042-1187 | 54 y | 82 kg | Bed ICU-07", "Penicillin");
        GuiToggleGroup((Rectangle){ W - pad - 3*130, pad, 130 - 2, 44 }, "MONITOR;TRENDS;SETUP", &page);

        if (GuiAlarmBanner((Rectangle){ pad, pad + 44 + gap, W - 2*pad, 44 }, alarmText, alarmPriority)) ackTimer = 60;

        float top = pad + 2*(44 + gap);
        Rectangle content = { pad, top, W - 2*pad, H - top - pad - 14 };

        if (page == 0)
        {
            float stripH = 118;
            float rightW = fmaxf(content.width*0.34f, 320);
            Rectangle waves = { content.x, content.y, content.width - rightW - gap, content.height - stripH - gap };
            float wh = (waves.height - 3*gap)/4;

            if (GuiWaveform((Rectangle){ waves.x, waves.y, waves.width, wh }, "II   x1", &ecg, GuiMedColor(MED_COLOR_ECG))) waveGrid = !waveGrid;
            GuiWaveform((Rectangle){ waves.x, waves.y + (wh + gap), waves.width, wh }, "ART", &art, GuiMedColor(MED_COLOR_BP));
            GuiWaveform((Rectangle){ waves.x, waves.y + 2*(wh + gap), waves.width, wh }, "Pleth", &pleth, GuiMedColor(MED_COLOR_SPO2));
            GuiWaveform((Rectangle){ waves.x, waves.y + 3*(wh + gap), waves.width, wh }, "Resp", &resp, GuiMedColor(MED_COLOR_RESP));

            float rx = waves.x + waves.width + gap;
            float th = (waves.height - 3*gap)/4;
            int clicked = 0;
            clicked |= GuiVitalSign((Rectangle){ rx, waves.y, rightW, th }, "HR", "bpm", roundf(pt.hr), 0, lim.hrLo, lim.hrHi, GuiMedColor(MED_COLOR_ECG));
            clicked |= GuiVitalSign((Rectangle){ rx, waves.y + (th + gap), rightW, th }, "SpO2", "%", roundf(pt.spo2), 0, lim.spo2Lo, lim.spo2Hi, GuiMedColor(MED_COLOR_SPO2));
            clicked |= GuiBloodPressure((Rectangle){ rx, waves.y + 2*(th + gap), rightW, th }, "NIBP", pt.nibpSys, pt.nibpDia, lim.sysLo, lim.sysHi, GuiMedColor(MED_COLOR_BP));

            float gaugeW = 70;
            float cw = (rightW - gaugeW - 2*gap)/2;
            float ry = waves.y + 3*(th + gap);
            clicked |= GuiVitalSign((Rectangle){ rx, ry, cw, th }, "RR", "rpm", roundf(pt.rr), 0, lim.rrLo, lim.rrHi, GuiMedColor(MED_COLOR_RESP));
            clicked |= GuiVitalSign((Rectangle){ rx + cw + gap, ry, cw, th }, "Temp", "C", pt.temp, 1, lim.tempLo, lim.tempHi, GuiMedColor(MED_COLOR_TEMP));
            GuiBarGauge((Rectangle){ rx + 2*(cw + gap), ry, gaugeW, th }, "EtCO2", pt.etco2, 0, 60, 30, 45, GuiMedColor(MED_COLOR_CO2));
            if (clicked) page = 2;

            float pw = (content.width - 2*gap)/3;
            float py = content.y + content.height - stripH;
            for (int i = 0; i < 3; i++)
                if (GuiInfusion((Rectangle){ content.x + i*(pw + gap), py, pw, stripH }, pumps[i].drug, pumps[i].rate, pumps[i].infused, pumps[i].volume, pumps[i].running))
                    pumps[i].running = !pumps[i].running;
        }
        else if (page == 1)
        {
            float cw = (content.width - gap)/2, ch = (content.height - gap)/2;
            GuiTrendChart((Rectangle){ content.x, content.y, cw, ch }, "HR  bpm", tHr, TREND_COUNT, 40, 160, lim.hrLo, lim.hrHi, GuiMedColor(MED_COLOR_ECG));
            GuiTrendChart((Rectangle){ content.x + cw + gap, content.y, cw, ch }, "SpO2  %", tSpo2, TREND_COUNT, 70, 100, lim.spo2Lo, NAN, GuiMedColor(MED_COLOR_SPO2));
            GuiBloodPressureTrend((Rectangle){ content.x, content.y + ch + gap, cw, ch }, "NIBP  mmHg", tSys, tDia, TREND_COUNT, 40, 180, GuiMedColor(MED_COLOR_BP));
            GuiTrendChart((Rectangle){ content.x + cw + gap, content.y + ch + gap, cw, ch }, "RR  rpm", tRr, TREND_COUNT, 0, 40, lim.rrLo, lim.rrHi, GuiMedColor(MED_COLOR_RESP));
        }
        else
        {
            float lw = content.width*0.5f;
            float lh = 92;
            float y = content.y;
            GuiAlarmLimits((Rectangle){ content.x, y, lw, lh }, "HR  bpm", &lim.hrLo, &lim.hrHi, 20, 200, 5, pt.hr, GuiMedColor(MED_COLOR_ECG)); y += lh + gap;
            GuiAlarmLimits((Rectangle){ content.x, y, lw, lh }, "SpO2  %", &lim.spo2Lo, &lim.spo2Hi, 50, 100, 1, pt.spo2, GuiMedColor(MED_COLOR_SPO2)); y += lh + gap;
            GuiAlarmLimits((Rectangle){ content.x, y, lw, lh }, "RR  rpm", &lim.rrLo, &lim.rrHi, 0, 60, 1, pt.rr, GuiMedColor(MED_COLOR_RESP)); y += lh + gap;
            GuiAlarmLimits((Rectangle){ content.x, y, lw, lh }, "NIBP sys  mmHg", &lim.sysLo, &lim.sysHi, 40, 240, 5, pt.nibpSys, GuiMedColor(MED_COLOR_BP)); y += lh + gap;
            GuiAlarmLimits((Rectangle){ content.x, y, lw, lh }, "Temp  C", &lim.tempLo, &lim.tempHi, 33, 42, 0.1f, pt.temp, GuiMedColor(MED_COLOR_TEMP));

            float rx = content.x + lw + gap, rw = content.width - lw - gap;
            GuiPainScale((Rectangle){ rx, content.y, rw, 170 }, &pain);

            // Plain raygui controls, sharing the same theme
            Rectangle box = { rx, content.y + 170 + gap + 10, rw, 300 };
            GuiGroupBox(box, "Simulation (raygui)");
            float sx = box.x + 130, sw = box.width - 200, sy = box.y + 20;
            GuiSlider((Rectangle){ sx, sy, sw, 24 }, "HR target", TextFormat("%.0f", pt.hrTarget), &pt.hrTarget, 30, 180); sy += 36;
            GuiSlider((Rectangle){ sx, sy, sw, 24 }, "SpO2 target", TextFormat("%.0f", pt.spo2Target), &pt.spo2Target, 70, 100); sy += 36;
            GuiSlider((Rectangle){ sx, sy, sw, 24 }, "RR target", TextFormat("%.0f", pt.rrTarget), &pt.rrTarget, 4, 40); sy += 44;
            GuiCheckBox((Rectangle){ box.x + 16, sy, 22, 22 }, "Light theme", &lightTheme);
            GuiCheckBox((Rectangle){ box.x + 180, sy, 22, 22 }, "ECG paper grid", &waveGrid);
            GuiCheckBox((Rectangle){ box.x + 380, sy, 22, 22 }, "Alarm flashing", &flashing); sy += 44;
            if (GuiButton((Rectangle){ box.x + 16, sy, 220, 36 }, "Desaturation event")) pt.desatTimer = 25;
            if (GuiButton((Rectangle){ box.x + 248, sy, 220, 36 }, "Refill pumps")) for (int i = 0; i < 3; i++) { pumps[i].infused = 0; pumps[i].running = true; }
        }

        DrawTextEx(GuiMedGetFont(), "Simulated data - not a medical device", (Vector2){ pad, H - 3 - 12 }, 12, 1, Fade(GuiMedColor(MED_COLOR_TEXT_DIM), 0.6f));
        EndDrawing();

        if (lightTheme != themeWas || lightTheme != appliedLight)
        {
            if (lightTheme) GuiMedLoadStyleLight(); else GuiMedLoadStyleDark();
            ApplyRayguiTheme(lightTheme);
            appliedLight = lightTheme;
        }
        GuiMedSetStyle(MED_WAVE_GRID, waveGrid);
        GuiMedSetStyle(MED_FLASH_ENABLED, flashing);

        frame++;
        if (shotFile && frame == 60*(strchr(shotFlags, 'd')? 14 : 8)) { TakeScreenshot(shotFile); break; }
    }

    if (font.texture.id > 0) UnloadFont(font);
    CloseWindow();
    return 0;
}
