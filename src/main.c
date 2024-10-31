#include "raylib.h"

#if defined(PLATFORM_WEB)
    #define CUSTOM_MODAL_DIALOGS
    #include <emscripten/emscripten.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
// Simple log system to avoid printf() calls if required
// NOTE: Avoiding those calls, also avoids const strings memory usage
#define SUPPORT_LOG_INFO
#if defined(SUPPORT_LOG_INFO)
    #define LOG(...) printf(__VA_ARGS__)
#else
    #define LOG(...)
#endif

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef enum { 
    SCREEN_LOGO = 0, 
    SCREEN_TITLE, 
    SCREEN_GAMEPLAY, 
    SCREEN_ENDING
} GameScreen;

typedef struct Card {
    Rectangle texcoords;
    int rotation;
    int combo_id;
    bool hovered, active, revealed, solved;
} Card;

#define GRID_WIDTH 6
#define GRID_HEIGHT 6
#define CARD_COUNT 36

static const float CARD_SPACING = 72.0f;
static const float CARD_SIZE = 64.0f;

// Includes padding; card texture will have a blank border

// Texture coordinates
#define SCL 32
#define PIECE_LG_00 ((Rectangle){ SCL*0, SCL*0, SCL, SCL})
#define PIECE_LG_01 ((Rectangle){ SCL*0, SCL*1, SCL, SCL})
#define PIECE_LG_10 ((Rectangle){ SCL*1, SCL*0, SCL, SCL})
#define PIECE_LG_11 ((Rectangle){ SCL*1, SCL*1, SCL, SCL})
#define PIECE_MD_00 ((Rectangle){ SCL*0, SCL*2, SCL, SCL})
#define PIECE_MD_01 ((Rectangle){ SCL*0, SCL*3, SCL, SCL})
#define PIECE_MD_10 ((Rectangle){ SCL*1, SCL*2, SCL, SCL})
#define PIECE_MD_11 ((Rectangle){ SCL*1, SCL*3, SCL, SCL})
#define PIECE_SM_00 ((Rectangle){ SCL*2, SCL*0, SCL, SCL})
#define PIECE_SM_01 ((Rectangle){ SCL*2, SCL*1, SCL, SCL})
#define PIECE_SM_10 ((Rectangle){ SCL*3, SCL*0, SCL, SCL})
#define PIECE_SM_11 ((Rectangle){ SCL*3, SCL*1, SCL, SCL})
#define CARD_0      ((Rectangle){ SCL*2, SCL*2, SCL, SCL})
#define CARD_1      ((Rectangle){ SCL*3, SCL*2, SCL, SCL})

static const Rectangle piece_combos[8][3] = {
    {PIECE_LG_00, PIECE_MD_10, PIECE_SM_11},
    {PIECE_LG_00, PIECE_MD_11, PIECE_SM_10},
    {PIECE_LG_01, PIECE_MD_00, PIECE_SM_11},
    {PIECE_LG_01, PIECE_MD_01, PIECE_SM_01},
    {PIECE_LG_10, PIECE_MD_10, PIECE_SM_10},
    {PIECE_LG_10, PIECE_MD_11, PIECE_SM_00},
    {PIECE_LG_11, PIECE_MD_00, PIECE_SM_10},
    {PIECE_LG_11, PIECE_MD_01, PIECE_SM_00},
};

typedef struct State {
    Card grid[GRID_WIDTH][GRID_HEIGHT];
    Texture2D texture;
    int revealed_count;
    int revealed_ids[3];
    int attempts;
} State;

// TODO: Define your custom data types here

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
static const int screen_width = 800;
static const int screen_height = 450;

/*static const Color cards[CARD_COUNT] = {*/
/**/
/*};*/

#define MAX_COLORS 8
static const Color colors[MAX_COLORS] = {
    WHITE,
    BLACK,
    MAROON,
    DARKGREEN,
    GOLD,
    DARKBLUE,
    DARKPURPLE,
    MAGENTA,
};

static State state = {0};

static RenderTexture2D target = { 0 };  // Render texture to render our game

// TODO: Define global variables here, recommended to make them static

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
static void update(void); // Update and Draw one frame

static void draw_card(Card *card, Rectangle dst) {
    Vector2 origin = (Vector2){CARD_SIZE/2.0f, CARD_SIZE/2.0f};
    dst.x += origin.x;
    dst.y += origin.y;
    float r = (float)card->rotation * 90.0f;
    if (!card->revealed && !card->solved) {
        DrawTexturePro(state.texture, CARD_0, dst, origin, r, WHITE);
    } else {
        DrawTexturePro(state.texture, CARD_1, dst, origin, r, WHITE);
        DrawTexturePro(state.texture, card->texcoords, dst, origin, r, WHITE);
    }
    /*DrawText(TextFormat("%d", card->combo_id), dst.x - origin.x, dst.y - origin.y, 20, BLACK);*/
    /*DrawText(TextFormat("%d", card->rotation), dst.x - origin.x, dst.y - origin.y + 25, 20, BLACK);*/
}

static void init_grid() {
    memset(state.grid, 0, sizeof(state.grid));
    for (int x = 0; x < GRID_WIDTH; x++) {
        for (int y = 0; y < GRID_HEIGHT; y++) {
            Card *card = &state.grid[x][y];

            int i = y * GRID_HEIGHT + x;
            int piece = i % 3;
            int rotation = (i / 3) % 4;
            /*int combo = (rotation / 4) % 8;*/
            int combo = (i / 12) % 3;
            int combo_id = rotation * 8 + combo;
            card->texcoords = piece_combos[combo][piece];
            /*card->revealed = true;*/
            card->rotation = rotation;
            card->combo_id = combo_id;
        }
    }
}

static void draw_grid() {

    Vector2 mouse = GetMousePosition();

    int padding = 2;
    for (int x = 0; x < GRID_WIDTH; x++) {
        for (int y = 0; y < GRID_HEIGHT; y++) {

            Card *card = &state.grid[x][y];
            float margin = (CARD_SPACING - CARD_SIZE) / 2.0f;
            Rectangle tex_rect = (Rectangle){x * CARD_SPACING + margin, y * CARD_SPACING + margin, CARD_SIZE, CARD_SIZE};
            Rectangle rect = (Rectangle){x * CARD_SPACING, y * CARD_SPACING, CARD_SPACING, CARD_SPACING};

            draw_card(card, tex_rect);

            /*float margin = CARD_SPACING;*/
            /**/
            /*Rectangle drawing_rect = (Rectangle){*/
            /*    x * CARD_SIZE + padding,*/
            /*    y * CARD_SIZE + padding,*/
            /*    CARD_SIZE - 2*padding,*/
            /*    CARD_SIZE - 2*padding,*/
            /*};*/
            /**/
            /**/
            if (CheckCollisionPointRec(mouse, rect)) {
                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                    card->active = true;
                } else {
                    card->hovered = true;
                }

                if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                    if (card->active) {
                        card->revealed = !(card->revealed);
                        card->active = false;
                        state.revealed_ids[state.revealed_count] = y * GRID_WIDTH + x;
                        state.revealed_count++;

                        if (state.revealed_count >= 3) {
                            int combo_id = -1;
                            bool solved = true;
                            for (int i = 0; i < 3; i++) {
                                int j = state.revealed_ids[i];
                                int xx = j % GRID_WIDTH;
                                int yy = j / GRID_WIDTH;
                                Card *c = &state.grid[xx][yy];
                                c->revealed = false;
                                if (combo_id == -1 || combo_id == c->combo_id) {
                                    combo_id = c->combo_id;
                                } else {
                                    solved = false;
                                }
                            }
                            for (int i = 0; i < 3; i++) {
                                int j = state.revealed_ids[i];
                                int xx = j % GRID_WIDTH;
                                int yy = j / GRID_WIDTH;
                                Card *c = &state.grid[xx][yy];
                                if (solved) {
                                    c->solved = true;
                                }
                            }
                            state.attempts++;
                            state.revealed_count = 0;
                        }

                    }
                }
            } else {
                card->hovered = false;
                card->active = false;
            }

            /*DrawRectangleRec(drawing_rect, color);*/
        }
    }

    DrawText(TextFormat("Attempts: %d", state.attempts), 0, 0, 20, BLACK);
}

int main(void) {
#if !defined(_DEBUG)
    /*SetTraceLogLevel(LOG_NONE); // Disable raylib trace log messages*/
#endif

    InitWindow(screen_width, screen_height, "raylib gamejam template");
    
    // TODO: Load resources / Initialize variables at this point
    state.texture = LoadTexture("resources/puzzle.png");
    init_grid();
    
    
    // Render texture to draw full screen, enables screen scaling
    // NOTE: If screen is scaled, mouse input should be scaled proportionally
    target = LoadRenderTexture(screen_width, screen_height);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

    SetExitKey(KEY_Q);

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(update, 60, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        update();
    }
#endif

    UnloadRenderTexture(target);
    // TODO: Unload all loaded resources at this point
    CloseWindow();
    return 0;
}

float vec2_distance(Vector2 v) {
    return sqrt(v.x * v.x + v.y * v.y);
}

Vector2 vec2_diff(Vector2 a, Vector2 b) {
    return (Vector2){a.x - b.x, a.y - b.y};
}

Vector2 vec2_add(Vector2 a, Vector2 b) {
    return (Vector2){a.x + b.x, a.y + b.y};
}

//--------------------------------------------------------------------------------------------
// Module functions definition
//--------------------------------------------------------------------------------------------
void update(void) {
    // Update
    Vector2 mouse = GetMousePosition();

    // Draw
    // Render game screen to a texture, 
    // it could be useful for scaling or further shader postprocessing
    BeginTextureMode(target);

    ClearBackground(GRAY);

    draw_grid();
    float texw = (float)state.texture.width;
    float texh = (float)state.texture.height;
    /*DrawTexturePro(*/
    /*        state.texture,*/
    /*        (Rectangle){0, 0, texw, texh},*/
    /*        (Rectangle){texw, texh, texw*2.0f, texh*2.0f},*/
    /*        (Vector2){texw, texh},*/
    /*        90.0f,*/
    /*        WHITE);*/

        
    EndTextureMode();
    
    // Render to screen (main framebuffer)
    BeginDrawing();

    ClearBackground(colors[0]);
    DrawTexturePro(target.texture, (Rectangle){ 0, 0, (float)target.texture.width, -(float)target.texture.height }, (Rectangle){ 0, 0, (float)target.texture.width, (float)target.texture.height }, (Vector2){ 0, 0 }, 0.0f, WHITE);

    /*DrawText(TextFormat("mouse=(%f, %f)", mouse.x, mouse.y), 0, 0, 20, colors[1]);*/
    /*DrawText(TextFormat("offset=(%f, %f)", state.offset.x, state.offset.y), 0, 25, 20, colors[1]);*/
    /*DrawText(TextFormat("presspos=(%f, %f)", state.mouse_press_pos.x, state.mouse_press_pos.y), 0, 50, 20, colors[1]);*/

    EndDrawing();
}
