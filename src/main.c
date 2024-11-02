#include "raylib.h"

#if defined(PLATFORM_WEB)
    #define CUSTOM_MODAL_DIALOGS
    #include <emscripten/emscripten.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

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
    bool hovered, revealed, solved, wrong;
} Card;

typedef struct State {
    Card *grid;
    Rectangle *piece_combos;
    Texture2D texture;
    int revealed_count;
    int revealed_ids[3];
    int attempts;
    Vector2 grid_offset;
    bool reset_cards;
    int grid_width;
    int grid_height;
    int card_count;
    float card_spacing;
    float card_size;
} State;

// Includes padding; card texture will have a blank border

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
static const int screen_width = 800;
static const int screen_height = 450;

#define MAX_COLORS 8
static Color colors[MAX_COLORS];

static State state = {0};

static RenderTexture2D target = { 0 };  // Render texture to render our game

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

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
static void update(void); // Update and Draw one frame

static void shuffle(Card *array, int n) {
    if (n > 1) {
        for (size_t i = n - 1; i > 0; i--) {
            size_t j = (size_t)((double)rand() / ((double)RAND_MAX + 1) * (i + 1));
            Card t = array[j];
            array[j] = array[i];
            array[i] = t;
        }
    }
}

static void draw_card(Card *card, Rectangle dst) {
    Vector2 origin = (Vector2){state.card_size/2.0f, state.card_size/2.0f};
    dst.x += origin.x;
    dst.y += origin.y;
    dst.x += state.grid_offset.x;
    dst.y += state.grid_offset.y;
    float r = (float)card->rotation * 90.0f;

    Rectangle hover_rect = (Rectangle){
        dst.x - origin.x - 2.0f,
            dst.y - origin.y - 2.0f,
            dst.width + 4.0f,
            dst.height + 5.0f,
    };

    if (card->solved) {
        DrawRectangleRec(hover_rect, colors[3]);
    } else if (card->wrong) {
        DrawRectangleRec(hover_rect, colors[2]);
    } else if (card->hovered) {
        DrawRectangleRec(hover_rect, colors[1]);
    }

    if (!card->revealed && !card->solved) {
        DrawTexturePro(state.texture, CARD_0, dst, origin, r, WHITE);
    } else {
        DrawTexturePro(state.texture, CARD_1, dst, origin, r, WHITE);
        DrawTexturePro(state.texture, card->texcoords, dst, origin, r, WHITE);
    }

    /*DrawText(TextFormat("%d", card->combo_id), dst.x - origin.x, dst.y - origin.y, 20, BLACK);*/
    /*DrawText(TextFormat("%d", card->rotation), dst.x - origin.x, dst.y - origin.y + 25, 20, BLACK);*/
    /*DrawText(TextFormat("%d", card->solved ? 1 : 0), dst.x - origin.x, dst.y - origin.y + 50, 20, BLACK);*/

}

static void init_grid(int grid_width, int grid_height) {

    state.grid_width = grid_width;
    state.grid_height = grid_height;
    state.card_size = (float)((screen_height - 10) /  grid_height / 32 * 32);
    state.card_spacing = state.card_size + state.card_size / 8.0f;
    state.grid_offset.y = (float)(screen_height - state.grid_height * state.card_spacing) / 2.0f;
    state.grid_offset.x = (float)(screen_width - state.grid_width * state.card_spacing - state.grid_offset.y); // TODO revisit for rectangular boards
    state.card_count = state.grid_width * state.grid_height;

    if (state.grid != NULL) {
        free(state.grid);
    }
    state.grid = malloc(sizeof(Card) * state.card_count);
    memset(state.grid, 0, sizeof(Card) * state.card_count);

    if (state.piece_combos != NULL) {
        free(state.piece_combos);
    }

    state.piece_combos = malloc(sizeof(Rectangle) * 8 * 3);
    state.piece_combos[0] = PIECE_LG_00;
    state.piece_combos[1] = PIECE_MD_10;
    state.piece_combos[2] = PIECE_SM_11;
    state.piece_combos[3] = PIECE_LG_00;
    state.piece_combos[4] = PIECE_MD_11;
    state.piece_combos[5] = PIECE_SM_10;
    state.piece_combos[6] = PIECE_LG_01;
    state.piece_combos[7] = PIECE_MD_00;
    state.piece_combos[8] = PIECE_SM_11;
    state.piece_combos[9] = PIECE_LG_01;
    state.piece_combos[10] = PIECE_MD_01;
    state.piece_combos[11] = PIECE_SM_01;
    state.piece_combos[12] = PIECE_LG_10;
    state.piece_combos[13] = PIECE_MD_10;
    state.piece_combos[14] = PIECE_SM_10;
    state.piece_combos[15] = PIECE_LG_10;
    state.piece_combos[16] = PIECE_MD_11;
    state.piece_combos[17] = PIECE_SM_00;
    state.piece_combos[18] = PIECE_LG_11;
    state.piece_combos[19] = PIECE_MD_00;
    state.piece_combos[20] = PIECE_SM_10;
    state.piece_combos[21] = PIECE_LG_11;
    state.piece_combos[22] = PIECE_MD_01;
    state.piece_combos[23] = PIECE_SM_00;
    // TODO more combos for bigger boards. (What's the max board size?)

    // TODO we're not using all of these, enforce color pallette
    colors[0] = WHITE;
    colors[1] = BLACK;
    colors[2] = MAROON;
    colors[3] = DARKGREEN;
    colors[4] = GOLD;
    colors[5] = DARKBLUE;
    colors[6] = DARKPURPLE;
    colors[7] = MAGENTA;

    for (int y = 0; y < state.grid_height; y++) {
        for (int x = 0; x < state.grid_width; x++) {
            int i = y * state.grid_width + x;
            Card *card = &state.grid[i];

            int p = y * state.grid_width + x;
            int piece = p % 3;
            int rotation = (p / 3) % 4;
            int combo = (p / 12) % 3;
            int combo_id = rotation * 8 + combo;
            int ii = combo * 3 + piece;
            card->texcoords = state.piece_combos[ii];
            card->rotation = rotation;
            card->combo_id = combo_id;
        }
    }

    shuffle(state.grid, state.card_count);
}

static void reset_cards() {
    for (int i = 0; i < state.card_count; i++) {
        state.grid[i].hovered = false;
        state.grid[i].revealed = false;
        state.grid[i].wrong = false;
    }
    state.revealed_count = 0;
}

static bool check_solve() {

    int guesses[3];
    int guess_count = 0;

    for (int i = 0; i < state.card_count; i++) {
        if (state.grid[i].revealed) {
            guesses[guess_count] = i;
            guess_count++;
            if (guess_count > 3) {
                LOG("ERROR: More than 3 guesses!");
                return;
            }
        }
    }

    bool all_match = true;
    for (int i = 0; i < 3; i++) {
        if (state.grid[guesses[i]].combo_id != state.grid[guesses[0]].combo_id) {
            all_match = false;
        }
    }

    if (all_match) {
        for (int i = 0; i < 3; i++) {
            state.grid[guesses[i]].solved = true;
        }
    } else {
        for (int i = 0; i < 3; i++) {
            state.grid[guesses[i]].wrong = true;
        }
    }

    return all_match;
}

static void draw_grid() {

    Vector2 mouse = GetMousePosition();

    bool in_revealed = false;
    if (state.revealed_count >= 3) {
        in_revealed = true;
        bool solved = check_solve();
        if (solved || IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            reset_cards();
        }
    }

    int padding = 2;
    for (int y = 0; y < state.grid_width; y++) {
        for (int x = 0; x < state.grid_height; x++) {

            int i = y * state.grid_width + x;
            Card *card = &state.grid[i];
            float margin = (state.card_spacing - state.card_size) / 2.0f;
            Rectangle rect = (Rectangle){x * state.card_spacing + state.grid_offset.x, y * state.card_spacing + state.grid_offset.y, state.card_spacing, state.card_spacing};
            Rectangle tex_rect = (Rectangle){x * state.card_spacing + margin, y * state.card_spacing + margin, state.card_size, state.card_size};

            if (CheckCollisionPointRec(mouse, rect)) {
                card->hovered = true;
                if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && !card->revealed && !in_revealed) {
                    card->revealed = true;
                    state.revealed_ids[state.revealed_count] = i;
                    if (state.revealed_count == 0) {
                        state.attempts++;
                    }
                    state.revealed_count++;
                }
            } else {
                card->hovered = false;
            }

            draw_card(card, tex_rect);
        }
    }

    DrawText(TextFormat("Attempts: %d", state.attempts), 0, 0, 20, BLACK);
}

int main(void) {
#if !defined(_DEBUG)
    /*SetTraceLogLevel(LOG_NONE); // Disable raylib trace log messages*/
#endif

    srand(time(NULL));

    InitWindow(screen_width, screen_height, "Puzzle Matcher");
    
    // TODO: Load resources / Initialize variables at this point
    state.texture = LoadTexture("resources/puzzle.png");
    init_grid(3, 3);

    
    
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

    if (IsKeyPressed(KEY_UP) && state.grid_height < 12) {
        init_grid(state.grid_width, state.grid_height + 1);
    }
    if (IsKeyPressed(KEY_DOWN) && state.grid_height > 1) {
        init_grid(state.grid_width, state.grid_height - 1);
    }
    if (IsKeyPressed(KEY_LEFT) && state.grid_width > 1) {
        init_grid(state.grid_width - 1, state.grid_height);
    }
    if (IsKeyPressed(KEY_RIGHT) && state.grid_width < 12) {
        init_grid(state.grid_width + 1, state.grid_height);
    }

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
