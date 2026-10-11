// Gridline - 3DS/2DS homebrew port (citro2d)
// Controls: A = move forward, D-pad / Circle Pad left-right = steer,
//           START on title = play, X on title = tracks screen, B = back, SELECT = exit

#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>

#define TOP_W 400
#define TOP_H 240
#define BOT_W 320
#define BOT_H 240

// 1 = straight, 2 = interchange, 3 = slight corner, 4/5 = corner animations
static const int trackArray[] = {
    1,2,3,      // Hangar Straight
    4,          // Stowe
    1,2,3,      // Vale straight
    5,          // Club
    4,          // Abbey
    4,          // Farm
    1,2,3,      // Wellington Straight
    4,          // Brooklands
    1,2,3,      // Straight
    5,          // Luffield
    4,          // Woodcote
    5,          // Copse
    1,2,3,      // Straight
    5,          // Maggotts
    4,          // Becketts
    4,          // Chapel
    1,2,3,      // Hangar Straight
    4,          // Stowe
    5,          // Vale
    4,          // Club
    5,          // Abbey
    4,          // Farm
    1,2,3       // Finish
};
#define TRACK_LEN ((int)(sizeof(trackArray) / sizeof(trackArray[0])))

enum {
    IMG_TRACK1, IMG_TRACK2, IMG_TRACK3,
    IMG_TRACK4_1, IMG_TRACK4_2, IMG_TRACK4_3, IMG_TRACK4_4, IMG_TRACK4_5,
    IMG_TRACK5_1, IMG_TRACK5_2, IMG_TRACK5_3, IMG_TRACK5_5,
    IMG_CAR, IMG_UI1, IMG_UI3,
    IMG_COUNT
};

static const char *imgFiles[IMG_COUNT] = {
    "romfs:/gfx/track1.t3x",   "romfs:/gfx/track2.t3x",   "romfs:/gfx/track3.t3x",
    "romfs:/gfx/track4_1.t3x", "romfs:/gfx/track4_2.t3x", "romfs:/gfx/track4_3.t3x",
    "romfs:/gfx/track4_4.t3x", "romfs:/gfx/track4_5.t3x",
    "romfs:/gfx/track5_1.t3x", "romfs:/gfx/track5_2.t3x", "romfs:/gfx/track5_3.t3x",
    "romfs:/gfx/track5_5.t3x",
    "romfs:/gfx/fororo.t3x",   "romfs:/gfx/ui1.t3x",      "romfs:/gfx/ui3.t3x"
};

static C2D_SpriteSheet sheets[IMG_COUNT];

typedef enum { STATE_TITLE, STATE_TRACKS, STATE_PLAY } GameState;

// Draw image i stretched to the rectangle x,y,w,h (same as p5's image(img,x,y,w,h))
static void drawImg(int i, float x, float y, float w, float h, float depth)
{
    C2D_Image img = C2D_SpriteSheetGetImage(sheets[i], 0);
    C2D_DrawImageAt(img, x, y, depth, NULL,
                    w / img.subtex->width, h / img.subtex->height);
}

static bool loadAll(void)
{
    for (int i = 0; i < IMG_COUNT; i++) {
        sheets[i] = C2D_SpriteSheetLoad(imgFiles[i]);
        if (!sheets[i]) return false;
    }
    return true;
}

int main(void)
{
    romfsInit();
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    C3D_RenderTarget *top    = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget *bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    if (!loadAll()) {
        // Missing assets: show a plain console message instead of crashing
        consoleInit(GFX_TOP, NULL);
        printf("Could not load graphics.\nCheck the romfs/gfx folder.\n\nPress START to exit.");
        while (aptMainLoop()) {
            hidScanInput();
            if (hidKeysDown() & KEY_START) break;
            gfxFlushBuffers();
            gfxSwapBuffers();
            gspWaitForVBlank();
        }
        gfxExit();
        return 1;
    }

    const u32 skyColor   = C2D_Color32(0x59, 0xb9, 0xde, 0xff);
    const u32 barBack    = C2D_Color32(0x20, 0x20, 0x20, 0xff);
    const u32 barFill    = C2D_Color32(0xe0, 0x30, 0x30, 0xff);

    GameState state = STATE_TITLE;
    int   trackPosition = 0;
    int   cornerTime = 0;
    float carWidth = TOP_W / 3.5f;
    float carHeight = TOP_H / 3.0f;
    float carx = TOP_W / 2.8f;

    while (aptMainLoop()) {
        hidScanInput();
        u32 down = hidKeysDown();
        u32 held = hidKeysHeld();

        if (down & KEY_SELECT) break;

        // ---------- input / update ----------
        if (state == STATE_TITLE) {
            if (down & (KEY_START | KEY_A | KEY_TOUCH)) {
                state = STATE_PLAY;
                trackPosition = 0;
                cornerTime = 0;
                carx = TOP_W / 2.8f;
            } else if (down & KEY_X) {
                state = STATE_TRACKS;
            }
        } else if (state == STATE_TRACKS) {
            if (down & KEY_B) state = STATE_TITLE;
        } else { // STATE_PLAY
            if (down & KEY_B) state = STATE_TITLE;

            // steer
            circlePosition cp;
            hidCircleRead(&cp);
            if ((held & KEY_DRIGHT) || cp.dx >  40) carx += 2.0f;
            if ((held & KEY_DLEFT)  || cp.dx < -40) carx -= 2.0f;
            if (carx < 0) carx = 0;
            if (carx > TOP_W - carWidth) carx = TOP_W - carWidth;

            int piece = trackArray[trackPosition];

            // corner animation timer (original: +10 per frame)
            if (piece == 4 || piece == 5) cornerTime += 10;

            // move forward (blocked while a corner animation is playing)
            if (down & KEY_A) {
                bool inCorner = (piece == 4 || piece == 5) && cornerTime < 120;
                if (!inCorner) {
                    trackPosition++;
                    cornerTime = 0;
                    if (trackPosition >= TRACK_LEN) trackPosition = 0;
                }
            }
        }

        // ---------- draw ----------
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        C2D_TargetClear(top, skyColor);
        C2D_SceneBegin(top);

        if (state == STATE_TITLE) {
            drawImg(IMG_UI1, 0, 0, TOP_W, TOP_H, 0.0f);
        } else if (state == STATE_TRACKS) {
            drawImg(IMG_UI3, 0, 0, TOP_W, TOP_H, 0.0f);
        } else {
            int bg = IMG_TRACK1;
            switch (trackArray[trackPosition]) {
                case 1: bg = IMG_TRACK1; break;
                case 2: bg = IMG_TRACK2; break;
                case 3: bg = IMG_TRACK3; break;
                case 4:
                    if      (cornerTime < 30)  bg = IMG_TRACK4_1;
                    else if (cornerTime < 60)  bg = IMG_TRACK4_2;
                    else if (cornerTime < 90)  bg = IMG_TRACK4_3;
                    else if (cornerTime < 120) bg = IMG_TRACK4_4;
                    else                       bg = IMG_TRACK4_5;
                    break;
                case 5:
                    if      (cornerTime < 30)  bg = IMG_TRACK5_1;
                    else if (cornerTime < 60)  bg = IMG_TRACK5_2;
                    else if (cornerTime < 90)  bg = IMG_TRACK5_3;
                    else                       bg = IMG_TRACK5_5;
                    break;
            }
            drawImg(bg, 0, 0, TOP_W, TOP_H, 0.0f);
            drawImg(IMG_CAR, carx, TOP_H / 2.0f, carWidth, carHeight, 0.5f);
        }

        // bottom screen: simple lap progress bar while playing
        C2D_TargetClear(bottom, C2D_Color32(0x10, 0x10, 0x10, 0xff));
        C2D_SceneBegin(bottom);
        if (state == STATE_PLAY) {
            float barX = 20, barY = 110, barW = BOT_W - 40, barH = 20;
            C2D_DrawRectSolid(barX, barY, 0.0f, barW, barH, barBack);
            C2D_DrawRectSolid(barX, barY, 0.1f,
                              barW * (float)(trackPosition + 1) / TRACK_LEN, barH, barFill);
        }

        C3D_FrameEnd(0);
    }

    for (int i = 0; i < IMG_COUNT; i++)
        if (sheets[i]) C2D_SpriteSheetFree(sheets[i]);

    C2D_Fini();
    C3D_Fini();
    gfxExit();
    romfsExit();
    return 0;
}
