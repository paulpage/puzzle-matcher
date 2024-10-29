#include "raylib.h"

#if defined(PLATFORM_WEB)
    #define CUSTOM_MODAL_DIALOGS
    #include <emscripten/emscripten.h>
#endif

#include <stdio.h>

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

typedef struct Shape {
    Vector2 pos;
    int r;
    int color_id;
} Shape;



#define MAX_SHAPES 1024
typedef struct State {
    Shape shapes[MAX_SHAPES];
    int shape_count;
    int color_id;
} State;

// TODO: Define your custom data types here

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
static const int screenWidth = 800;
static const int screenHeight = 450;

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

int main(void) {
#if !defined(_DEBUG)
    SetTraceLogLevel(LOG_NONE); // Disable raylib trace log messages
#endif

    InitWindow(screenWidth, screenHeight, "raylib gamejam template");
    
    // TODO: Load resources / Initialize variables at this point
    state.color_id = 2; // black and white are reserved
    
    // Render texture to draw full screen, enables screen scaling
    // NOTE: If screen is scaled, mouse input should be scaled proportionally
    target = LoadRenderTexture(screenWidth, screenHeight);
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

//--------------------------------------------------------------------------------------------
// Module functions definition
//--------------------------------------------------------------------------------------------
void update(void) {
    // Update
    Vector2 mouse = GetMousePosition();

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        state.color_id++;
        if (state.color_id >= MAX_COLORS) {
            state.color_id = 2;
        }
    }

    if (IsKeyPressed(KEY_R)) {
        state.shape_count = 0;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        state.shapes[state.shape_count] = (Shape){mouse, 20, state.color_id};
        state.shape_count++;
    }

    // Temporarily place the mouse position on the shape stack
    state.shapes[state.shape_count] = (Shape){mouse, 20, state.color_id};
    state.shape_count++;

    // Draw
    // Render game screen to a texture, 
    // it could be useful for scaling or further shader postprocessing
    BeginTextureMode(target);

    ClearBackground(RAYWHITE);
    /*DrawText("Welcome to raylib NEXT gamejam!", 150, 140, 30, colors[1]);*/
    /*DrawRectangleLinesEx((Rectangle){ 0, 0, screenWidth, screenHeight }, 16, colors[1]);*/

    /*DrawRectangle(20, 20, 100, 100, GREEN);*/

    for (int i = 0; i < state.shape_count; i++) {
        for (int j = i + 1; j < state.shape_count; j++) {
            Color color = colors[1];
            if (state.shapes[i].color_id == state.shapes[j].color_id) {
                color = colors[state.shapes[i].color_id];
            }
            DrawLineEx(state.shapes[i].pos, state.shapes[j].pos, 2, color);
        }
    }

    for (int i = 0; i < state.shape_count; i++) {
        Shape shape = state.shapes[i];
        DrawCircleV(shape.pos, shape.r, colors[shape.color_id]);
    }

    /*DrawCircle(mouse.x, mouse.y, 20, colors[state.active_color_idx]);*/
    DrawRing(mouse, 16.0f, 20.0f, 0.0f, 360.0f, 12, colors[state.color_id]);
        
    EndTextureMode();
    
    // Render to screen (main framebuffer)
    BeginDrawing();

    ClearBackground(colors[0]);
    DrawTexturePro(target.texture, (Rectangle){ 0, 0, (float)target.texture.width, -(float)target.texture.height }, (Rectangle){ 0, 0, (float)target.texture.width, (float)target.texture.height }, (Vector2){ 0, 0 }, 0.0f, WHITE);

    EndDrawing();

    // Pop the mouse position from the shape stack
    state.shape_count--;
}
