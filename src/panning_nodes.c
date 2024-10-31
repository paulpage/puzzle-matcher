#include "raylib.h"

#if defined(PLATFORM_WEB)
    #define CUSTOM_MODAL_DIALOGS
    #include <emscripten/emscripten.h>
#endif

#include <stdio.h>
#include <math.h>

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
    Vector2 mouse_press_pos;
    Vector2 mouse_press_offset;
    Vector2 offset;
} State;

// TODO: Define your custom data types here

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
static const int screen_width = 800;
static const int screen_height = 450;


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

    InitWindow(screen_width, screen_height, "raylib gamejam template");
    
    // TODO: Load resources / Initialize variables at this point
    state.color_id = 2; // black and white are reserved
    
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
    /*Vector2 mouse = vec2_diff(GetMousePosition(), state.offset);*/
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
        state.mouse_press_pos = mouse;
        state.mouse_press_offset = state.offset;
        /*state.shapes[state.shape_count] = (Shape){mouse, 20, state.color_id};*/
        /*state.shape_count++;*/
    }

    Vector2 mouse_delta = vec2_diff(mouse, state.mouse_press_pos);
    int mouse_distance = vec2_distance(mouse_delta);

    if (mouse_distance > 5) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            /*state.offset = vec2_add(state.offset, mouse_delta);*/
            state.offset = vec2_add(state.mouse_press_offset, mouse_delta);
            /*state.offset = vec2_add(mouse_delta, state.mouse_press_pos);*/
            /*state.mouse_press_pos = mouse;*/
        }
    } else {
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            state.shapes[state.shape_count] = (Shape){vec2_diff(mouse, state.offset), 20, state.color_id};
            state.shape_count++;
        }
    }

    /*// Temporarily place the mouse position on the shape stack*/
    /*state.shapes[state.shape_count] = (Shape){mouse, 20, state.color_id};*/
    /*state.shape_count++;*/

    // Draw
    // Render game screen to a texture, 
    // it could be useful for scaling or further shader postprocessing
    BeginTextureMode(target);

    ClearBackground(RAYWHITE);

    /*DrawRectangle(20, 20, 100, 100, GREEN);*/

    for (int i = 0; i < state.shape_count; i++) {
        for (int j = i + 1; j < state.shape_count; j++) {
            Color color = colors[1];
            if (state.shapes[i].color_id == state.shapes[j].color_id) {
                color = colors[state.shapes[i].color_id];
            }
            DrawLineEx(vec2_add(state.shapes[i].pos, state.offset), vec2_add(state.shapes[j].pos, state.offset), 2, color);
        }
    }

    for (int i = 0; i < state.shape_count; i++) {
        Shape shape = state.shapes[i];
        DrawCircleV(vec2_add(shape.pos, state.offset), shape.r, colors[shape.color_id]);
    }

    /*DrawCircle(mouse.x, mouse.y, 20, colors[state.active_color_idx]);*/
    DrawRing(mouse, 16.0f, 20.0f, 0.0f, 360.0f, 12, colors[state.color_id]);
        
    EndTextureMode();
    
    // Render to screen (main framebuffer)
    BeginDrawing();

    ClearBackground(colors[0]);
    DrawTexturePro(target.texture, (Rectangle){ 0, 0, (float)target.texture.width, -(float)target.texture.height }, (Rectangle){ 0, 0, (float)target.texture.width, (float)target.texture.height }, (Vector2){ 0, 0 }, 0.0f, WHITE);

    /*DrawText(TextFormat("mouse=(%f, %f)", mouse.x, mouse.y), 0, 0, 20, colors[1]);*/
    /*DrawText(TextFormat("offset=(%f, %f)", state.offset.x, state.offset.y), 0, 25, 20, colors[1]);*/
    /*DrawText(TextFormat("presspos=(%f, %f)", state.mouse_press_pos.x, state.mouse_press_pos.y), 0, 50, 20, colors[1]);*/

    EndDrawing();

    /*// Pop the mouse position from the shape stack*/
    /*state.shape_count--;*/
}
