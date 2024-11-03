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

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

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
    bool showing_new_buttons;
    bool has_won;
    float scale_factor;
} State;

// Includes padding; card texture will have a blank border

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
static const int screen_width = 800;
static const int screen_height = 450;
static const float menu_width = 350.0f;

#define COLOR_BG ((Color){0x9b, 0x99, 0xb6, 0xff})
#define COLOR_DARK ((Color){0x1b, 0x20, 0x1f, 0xff})
#define COLOR_LIGHT ((Color){0x1f, 0xc1, 0x65, 0xff})
#define COLOR_RED ((Color){0xc9, 0x29, 0x6e, 0xff})
#define COLOR_CARD_DARK ((Color){0xd9, 0xa0, 0x66, 0xff})
#define COLOR_CARD_LIGHT ((Color){0x32, 0x3c, 0x39, 0xff})
#define COLOR_PUZ_DARK ((Color){0x5b, 0x6e, 0xe1, 0xff})
#define COLOR_PUZ_LIGHT ((Color){0x63, 0x9b, 0xff, 0xff})

static State state = {0};
static Texture2D texture;
static Font font;

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

#define BORDER_CORNER ((Rectangle){64, 96, 12, 12})
#define BORDER_X ((Rectangle){76, 96, 4, 12})
#define BORDER_Y ((Rectangle){64, 108, 12, 4})

#define TITLE ((Rectangle){ 0, 0, 79, 39 })
#define ATTEMPTS ((Rectangle){ 0, 40, 84, 24 })
#define NEW ((Rectangle){ 0, 110, 37, 18 })
#define LETTER_X ((Rectangle){ 48, 112, 11, 10 })
#define NUM_0 ((Rectangle){ 0,  64, 11, 16 })
#define NUM_1 ((Rectangle){ 11, 64, 7,  16 })
#define NUM_2 ((Rectangle){ 18, 64, 11, 16 })
#define NUM_3 ((Rectangle){ 29, 64, 11, 16 })
#define NUM_4 ((Rectangle){ 40, 64, 11, 16 })
#define NUM_5 ((Rectangle){ 51, 64, 11, 16 })
#define NUM_6 ((Rectangle){ 62, 64, 11, 16 })
#define NUM_7 ((Rectangle){ 73, 64, 11, 16 })
#define NUM_8 ((Rectangle){ 84, 64, 11, 16 })
#define NUM_9 ((Rectangle){ 95, 64, 11, 16 })

#define TEXT_HEIGHT 23
#define NUM_HEIGHT 16

typedef enum Alignment {
    ALIGN_START,
    ALIGN_MID,
    ALIGN_END,
} Alignment;

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

    float border = state.card_size / 32.0f;
    Rectangle hover_rect = (Rectangle){
        dst.x - origin.x - border,
            dst.y - origin.y - border,
            dst.width + border * 2.0f,
            dst.height + border * 2.0f,
    };

    if (card->solved) {
        DrawRectangleRec(hover_rect, COLOR_LIGHT);
    } else if (card->wrong) {
        DrawRectangleRec(hover_rect, COLOR_RED);
    } else if (card->hovered) {
        DrawRectangleRec(hover_rect, COLOR_DARK);
    }

    if (!card->revealed && !card->solved) {
        DrawTexturePro(texture, CARD_0, dst, origin, 0, WHITE);
    } else {
        DrawTexturePro(texture, CARD_1, dst, origin, r, WHITE);
        DrawTexturePro(texture, card->texcoords, dst, origin, r, WHITE);
    }

    /*DrawTextEx(font, TextFormat("%d", card->combo_id), (Vector2){dst.x - origin.x, dst.y - origin.y}, 16, 0, COLOR_DARK);*/
    /*DrawTextEx(font, TextFormat("%d", card->rotation), (Vector2){dst.x - origin.x, dst.y - origin.y + 25}, 16, 0, COLOR_DARK);*/
    /*DrawTextEx(font, TextFormat("%d", card->solved ? 1 : 0), (Vector2){dst.x - origin.x, dst.y - origin.y + 50}, 16, 0, COLOR_DARK);*/

}

static bool check_solve() {

    int guesses[3];
    int guess_count = 0;
    state.has_won = true;

    for (int i = 0; i < state.card_count; i++) {
        if (!state.grid[i].solved) {
            state.has_won = false;
        }
        if (state.grid[i].revealed) {
            guesses[guess_count] = i;
            guess_count++;
            if (guess_count > 3) {
                LOG("ERROR: More than 3 guesses!");
                return false;
            }
        }
    }

    bool all_match = false;
    if (guess_count == 3) {
        all_match = true;
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
    }

    return all_match;
}


static void init_grid(int grid_width, int grid_height) {

    memset(&state, 0, sizeof(State));

    state.grid_width = grid_width;
    state.grid_height = grid_height;

    state.card_size = min(
        (float)((screen_width - (int)menu_width - 10) / grid_width / 32 * 32),
        (float)((screen_height - 10) / grid_height / 32 * 32)
    );

    state.card_spacing = state.card_size + state.card_size / 8.0f;
    state.grid_offset.y = (float)(screen_height - state.grid_height * state.card_spacing) / 2.0f;
    state.grid_offset.x = (float)(screen_width - state.grid_width * state.card_spacing - 10);
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

    for (int y = 0; y < state.grid_height; y++) {
        for (int x = 0; x < state.grid_width; x++) {
            int i = y * state.grid_width + x;
            Card *card = &state.grid[i];

            int piece = i % 3;
            int rotation = (i / 3) % 4;
            int combo = (i / 12) % 3;
            int combo_id = rotation * 8 + combo;
            int ii = combo * 3 + piece;
            card->texcoords = state.piece_combos[ii];
            card->rotation = rotation;
            card->combo_id = combo_id;
        }
    }

    shuffle(state.grid, state.card_count);

    /*for (int i = 0; i < state.card_count; i++) {*/
    /*    state.grid[i].solved = true;*/
    /*}*/
    /*check_solve();*/
}

static void reset_cards() {
    for (int i = 0; i < state.card_count; i++) {
        state.grid[i].hovered = false;
        state.grid[i].revealed = false;
        state.grid[i].wrong = false;
    }
    state.revealed_count = 0;
}

static void ui_label(const char *text, Vector2 pos, float size, Alignment align_x, Alignment align_y) {
    Vector2 text_size = MeasureTextEx(font, text, size, 1);
    Vector2 origin = {0, 0};
    switch (align_x) {
        case ALIGN_START: break;
        case ALIGN_MID: origin.x = text_size.x / 2.0f; break;
        case ALIGN_END: origin.x = text_size.x; break;
        default: break;
    }
    switch (align_y) {
        case ALIGN_START: break;
        case ALIGN_MID: origin.y = text_size.y / 2.0f; break;
        case ALIGN_END: origin.y = text_size.y; break;
        default: break;
    }

    /*DrawRectanglePro((Rectangle){pos.x, pos.y, text_size.x, text_size.y}, origin, 0, BLUE);*/

    DrawTextPro(font, text, pos, origin, 0, size, 1, COLOR_DARK);
}

static bool ui_button(char *text, Vector2 pos, float size, Alignment align_x, Alignment align_y) {

    Vector2 text_size = MeasureTextEx(font, text, size, 1);
    float border = 4.0f;
    float margin = 8.0f;
    Rectangle outer_rect = {pos.x, pos.y, text_size.x + margin * 2.0f, text_size.y + margin * 2.0f};
    Rectangle inner_rect = {outer_rect.x + border, outer_rect.y + border, outer_rect.width - border * 2.0f, outer_rect.height - border * 2.0f};

    Vector2 text_pos = {pos.x + margin, pos.y + margin};
    Vector2 origin = {0, 0};
    switch (align_x) {
        case ALIGN_START: break;
        case ALIGN_MID: origin.x = outer_rect.width / 2.0f; break;
        case ALIGN_END: origin.x = outer_rect.width; break;
        default: break;
    }
    switch (align_y) {
        case ALIGN_START: break;
        case ALIGN_MID: origin.y = outer_rect.height / 2.0f; break;
        case ALIGN_END: origin.y = outer_rect.height; break;
        default: break;
    }

    Vector2 mouse = GetMousePosition();
    mouse.x /= state.scale_factor;
    mouse.y /= state.scale_factor;

    Rectangle interaction_rect = {outer_rect.x - origin.x, outer_rect.y - origin.y, outer_rect.width, outer_rect.height};

    bool hovered = CheckCollisionPointRec(mouse, interaction_rect);
    bool active = hovered && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    bool clicked = hovered && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);

    Color fill_color = COLOR_BG;
    Color text_color = COLOR_DARK;
    if (active) {
        fill_color = COLOR_DARK;
        text_color = COLOR_LIGHT;
    } else if (hovered) {
        fill_color = COLOR_LIGHT;
    }

    DrawRectanglePro(outer_rect, origin, 0, text_color);
    DrawRectanglePro(inner_rect, origin, 0, fill_color);
    DrawTextPro(font, text, text_pos, origin, 0, size, 1, text_color);

    return clicked;
}

static void draw_ui() {

    Vector2 origin = {12, 12};
    DrawTexturePro(texture, BORDER_CORNER, (Rectangle){24, 24, 24, 24}, origin, 0, WHITE);
    DrawTexturePro(texture, BORDER_CORNER, (Rectangle){menu_width - 24, 24, 24, 24}, origin, 0, WHITE);
    DrawTexturePro(texture, BORDER_CORNER, (Rectangle){24, screen_height - 24, 24, 24}, origin, 0, WHITE);
    DrawTexturePro(texture, BORDER_CORNER, (Rectangle){menu_width - 24, screen_height - 24, 24, 24}, origin, 0, WHITE);

    origin = (Vector2){12, (screen_height - 36*2) / 2};

    DrawTexturePro(texture, BORDER_Y, (Rectangle){24, screen_height / 2, 24, screen_height - 36*2}, origin , 0, WHITE);
    DrawTexturePro(texture, BORDER_Y, (Rectangle){menu_width - 24, screen_height / 2, 24, screen_height - 36*2}, origin , 0, WHITE);

    origin = (Vector2){(menu_width - 36*2) / 2, 12};
    DrawTexturePro(texture, BORDER_X, (Rectangle){menu_width / 2, 24, menu_width - 36*2, 24}, origin , 0, WHITE);
    DrawTexturePro(texture, BORDER_X, (Rectangle){menu_width / 2, screen_height - 24, menu_width - 36*2, 24}, origin , 0, WHITE);

    Vector2 title_pos = {menu_width / 2, 48};
    ui_label("Puzzle", title_pos, 48, ALIGN_MID, ALIGN_START);
    title_pos = (Vector2){menu_width / 2, 96};
    ui_label("Matcher", title_pos, 48, ALIGN_MID, ALIGN_START);

    Vector2 attempts_pos = {48, screen_height - 96 - 12};
    ui_label(TextFormat("Attempts: %d", state.attempts), attempts_pos, 36, ALIGN_START, ALIGN_END);

    Vector2 new_position = {48, screen_height - 48};
    if (!state.showing_new_buttons) {
        if (ui_button("New", new_position, 36, ALIGN_START, ALIGN_END)) {
            state.showing_new_buttons = true;
        }
    } else {
        if (ui_button("Cancel", new_position, 36, ALIGN_START, ALIGN_END)) {
            state.showing_new_buttons = false;
        }
        Vector2 pos = {184, screen_height - 48};
        if (ui_button("3x3", pos, 36.0f, ALIGN_START, ALIGN_END)) {
            init_grid(3, 3);
        }
        pos.x += 80;
        if (ui_button("4x3", pos, 36.0f, ALIGN_START, ALIGN_END)) {
            init_grid(4, 3);
        }
        pos.x += 80;
        if (ui_button("6x4", pos, 36.0f, ALIGN_START, ALIGN_END)) {
            init_grid(6, 4);
        }
        pos.x += 80;
        if (ui_button("6x6", pos, 36.0f, ALIGN_START, ALIGN_END)) {
            init_grid(6, 6);
        }
    }

    if (state.has_won) {
        Vector2 win_pos = {48, screen_height / 2};
        ui_label("You won! Press \"New\"\nto try again with a\nlarger board, or try\nto win in fewer\nattempts.", win_pos, 24, ALIGN_START, ALIGN_MID);
    }
}

static void draw_grid() {
    Vector2 mouse = GetMousePosition();
    mouse.x /= state.scale_factor;
    mouse.y /= state.scale_factor;

    bool in_revealed = false;
    if (state.revealed_count >= 3) {
        in_revealed = true;
        bool solved = check_solve();
        if (solved || IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            reset_cards();
        }
    }

    for (int y = 0; y < state.grid_height; y++) {
        for (int x = 0; x < state.grid_width; x++) {

            int i = y * state.grid_width + x;
            Card *card = &state.grid[i];
            // TODO borders are incorrect without margin, why?
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


}

int main(void) {
#if !defined(_DEBUG)
    /*SetTraceLogLevel(LOG_NONE); // Disable raylib trace log messages*/
#endif

    srand(time(NULL));

    InitWindow(screen_width, screen_height, "Puzzle Matcher");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetExitKey(KEY_Q);
    
    texture = LoadTexture("resources/puzzle.png");
    font = LoadFont("resources/november.ttf");
    init_grid(3, 3);

    // Render texture to draw full screen, enables screen scaling
    // NOTE: If screen is scaled, mouse input should be scaled proportionally
    target = LoadRenderTexture(screen_width, screen_height);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);


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

    // Draw
    // Render game screen to a texture, 
    // it could be useful for scaling or further shader postprocessing
    BeginTextureMode(target);

    ClearBackground(COLOR_BG);

    draw_grid();
    draw_ui();
        
    EndTextureMode();

    float scale_x = (float)GetScreenWidth() / screen_width;
    float scale_y = (float)GetScreenHeight() / screen_height;
    state.scale_factor = min(scale_x, scale_y);
    
    // Render to screen (main framebuffer)
    BeginDrawing();

    ClearBackground(COLOR_BG);
    DrawTexturePro(target.texture, (Rectangle){ 0, 0, (float)target.texture.width, -(float)target.texture.height }, (Rectangle){ 0, 0, (float)target.texture.width * state.scale_factor, (float)target.texture.height * state.scale_factor }, (Vector2){ 0, 0 }, 0.0f, WHITE);

    EndDrawing();
}
