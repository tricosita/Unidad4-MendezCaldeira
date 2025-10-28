

// Mendez Caldeira Triana - MAVI 2025

#include "raylib.h"
#include <vector>
#include <string>
#include <cstdio>   // std::snprintf
#include <cmath>

// ---------------- Configuración general ----------------
static constexpr int   W = 2080;
static constexpr int   H = 2080-500;
static constexpr int   TARGET_FPS = 60;

static constexpr int   RUN_FRAMES = 5;   // cantidad de PNGs (1..5)
static constexpr float RUN_FPS = 08.0f; // velocidad de la animación de “correr en el lugar”

// Física simple
static constexpr float GRAVITY = 2400.0f; // px/s^2 (gravedad fuerte pues ventana grande. numero mas chico, flota en el aire))
static constexpr float JUMP_VELOCITY = 1500.0f; // px/s (mayo numero mayor salto)

// Piso (línea de suelo)
static constexpr float GROUND_Y = H * 0.90f;    // altura del suelo 

// Escala visual del sprite
static constexpr float SPRITE_SCALE = 6.0f;     // aumenta o baja según el tamaño de tus PNGs


// --- Moon (boton) ---
static constexpr const char* MOON_PATH = "../../assets/texture/moon.png"; 
static constexpr float MOON_SCALE = 1.0f;   // ajusta tamaño
static constexpr float MOON_MARGIN_RIGHT = 80.0f; // distancia del borde derecho
bool isNight = false;  // controla si el fondo es negro o blanco


// ---------------- Animación con frames sueltos ----------------
struct FrameListAnim {
    std::vector<Texture2D> frames;
    int   frame = 0;
    float fps = RUN_FPS;
    float acc = 0.0f;      // acumulador de tiempo
    bool  loop = true;

    void Update(float dt, bool isPlaying = true) {
        if (!isPlaying || frames.empty()) return;
        const float spf = 1.0f / fps;
        acc += dt;
        while (acc >= spf) {
            acc -= spf;
            frame = (frame + 1) % (int)frames.size();
        }
    }

    const Texture2D& Current() const {
        return frames[frame];
    }

    void UnloadAll() {
        for (auto& t : frames) UnloadTexture(t);
        frames.clear();
    }
};

// ---------------- Player minimalista: corre en lugar + salto ----------------
struct Player {
    // usa (x, y) como el punto de los pies (centro de apoyo)
    Vector2 pos{ W * 0.5f, GROUND_Y +200.0f}; //(altura en relacion al suelo)
    float   velY = 0.0f;
    float velX = 0.0f;      // velocidad horizontal actual (px/s)
    float speed = 600.0f;    // velocidad base al moverse (px/s)
    int   facing = 1;         // 1 = derecha, -1 = izquierda (para flip)
    bool    onGround = true;
    FrameListAnim run;        // animación de “correr en el lugar”
    float   scale = SPRITE_SCALE;

    //sonido de salto
    Sound* sfxJump{ nullptr };   // no posee el recurso; lo administra main

    bool isRunning = false;   // controla si el personaje anima o se queda quieto

    void Update(float dt) {
        // Volver a modo estático con R 
        if (IsKeyPressed(KEY_R)) {
            isRunning = false;
            run.frame = 0;
            run.acc = 0.0f;
            velY = 0.0f;
            pos.y = GROUND_Y;
        }

        // Input horizontal 
        velX = 0.0f;
        if (IsKeyDown(KEY_RIGHT)) {
            velX = +speed;
            if (facing != 1) facing = 1;   // flip a derecha
            isRunning = true;              // al moverse, animar
        }
        if (IsKeyDown(KEY_LEFT)) {
            velX = -speed;
            if (facing != -1) facing = -1; // flip a izquierda
            isRunning = true;
        }

        // Integración horizontal + límites
        pos.x += velX * dt;
        const float margin = 50.0f;
        if (pos.x < margin)      pos.x = margin;
        if (pos.x > W - margin)  pos.x = W - margin;

        // Salto con SPACE (solo si está en el suelo)
        if (onGround && IsKeyPressed(KEY_SPACE)) {
            velY = -JUMP_VELOCITY;
            onGround = false;
            if (!isRunning) isRunning = true;   // arranca animación al primer salto
            if (sfxJump) PlaySound(*sfxJump);
        }

        // Animación solo si corre
        run.Update(dt, isRunning);

        // Física vertical y colisión con el suelo
        velY += GRAVITY * dt;
        pos.y += velY * dt;

        if (pos.y >= GROUND_Y) {
            pos.y = GROUND_Y;
            velY = 0.0f;
            onGround = true;
        }
    }
   

    void Draw() const {
        if (run.frames.empty()) return;

        const Texture2D& tex = run.Current();

        // Fuente: ancho con signo para flip
        Rectangle src{ 0, 0, (float)tex.width, (float)tex.height };
        if (facing < 0) src.width = -src.width;  // ← flip horizontal si mira a la izquierda

        // Destino: usa pivot centro-inferior (pies)
        Rectangle dst{ pos.x, pos.y, tex.width * scale, tex.height * scale };
        Vector2   origin{ dst.width * 0.5f, dst.height };

        DrawTexturePro(tex, src, dst, origin, 0.0f, WHITE);
    }
};

// ---------------- Utilidad: carga de N frames secuenciales ----------------
static bool LoadRunFrames(FrameListAnim& anim) {
    anim.frames.reserve(RUN_FRAMES);

        // const char* pattern = "../../assets/texture/run/%d.png";
    const char* pattern = "../../assets/texture/run/%d.png";

    for (int i = 1; i <= RUN_FRAMES; ++i) {
        char path[512];
        std::snprintf(path, sizeof(path), pattern, i);

        if (!FileExists(path)) {
            TraceLog(LOG_ERROR, "No existe el archivo: %s  (revisar ruta y Working Directory)", path);
            return false;
        }

        Texture2D t = LoadTexture(path);
        // Para pixel art: mantener nitidez al escalar
        SetTextureFilter(t, TEXTURE_FILTER_POINT);
        anim.frames.push_back(t);
    }
    anim.fps = RUN_FPS;
    return true;
}

//estrellas
std::vector<Vector2> starPositions = {
    {1000,  40},
    {1300, 300},
    {1360,  40},
    {900,  500},
    {700,  600},
    {720,   60},
    {700,  700},
    {300, 1000}
};




// ---------------- Programa principal ----------------
int main() {
    InitWindow(W, H, "Proyecto MAVI - Saltar");
    SetTargetFPS(TARGET_FPS);
    InitAudioDevice();


    // --- Cargar moon (boton) ---
    Texture2D moonTex = { 0 };
    Rectangle  moonRect = { 0 }; // rect de clic y dibujo
    if (!FileExists(MOON_PATH)) {
        TraceLog(LOG_WARNING, "No existe: %s", MOON_PATH);
    }
    else {
        moonTex = LoadTexture(MOON_PATH);
        SetTextureFilter(moonTex, TEXTURE_FILTER_BILINEAR); // o POINT si es pixel art
        // posicionar apoyada en el suelo, cerca del margen derecho
        float w = moonTex.width * MOON_SCALE;
        float h = moonTex.height * MOON_SCALE;
        float x = W - MOON_MARGIN_RIGHT - w;   // pegado a la derecha con margen
		float y = 50.0f;                     // desde arriba
        moonRect = { x, y, w, h };
    }

    // --- Configuración global de audio ---
    if (IsAudioDeviceReady()) {
        SetMasterVolume(1.0f);          // volumen global máximo
        TraceLog(LOG_INFO, "AudioDevice OK (volumen global 100%%)");
    }
    else {
        TraceLog(LOG_ERROR, "AudioDevice NO está listo");
    }


    // --- Diagnóstico de audio ---
    if (!IsAudioDeviceReady()) {
        TraceLog(LOG_ERROR, "AudioDevice NO listo");
    }
    else {
        TraceLog(LOG_INFO, "AudioDevice listo");
    }
    // Fuerza volumen global alto
    SetMasterVolume(1.0f);

    // Cargar assets
    Player hero;
    if (!LoadRunFrames(hero.run)) {
        DrawText("Error cargando frames. Ver consola.", 40, 40, 30, RED);
        // Esperar un poco para leer logs si ejecutás fuera del depurador
        // (en VS conviene correr con F5 y mirar Output)
        CloseWindow();
        return 1;
    }
    
    // Cargar sonido de salto
    const char* jumpPath = "../../assets/sound/jump.wav";
    if (!FileExists(jumpPath)) {
        TraceLog(LOG_WARNING, "No existe SFX: %s (se ejecuta sin sonido)", jumpPath);
    }
    Sound jump = { 0 };
    if (FileExists(jumpPath)) {
        jump = LoadSound(jumpPath);
        SetSoundVolume(jump, 1.0f);    // volumen individual máximo (0..1)
        TraceLog(LOG_INFO, "SFX cargado: %s | volumen 100%%", jumpPath);
    }


    // Hacer accesible el sonido desde el Player ?
    hero.sfxJump = FileExists(jumpPath) ? &jump : nullptr;

    // --- Diagnóstico inicial ---
    TraceLog(LOG_INFO, "Frames cargados: %i", (int)hero.run.frames.size());

    while (!WindowShouldClose()) {
        const float dt = GetFrameTime();

        
        // Update
        hero.Update(dt);

        // --- Controlar velocidad de animación ---
        if (IsKeyPressed(KEY_UP)) hero.run.fps += 2.0f;
        if (IsKeyPressed(KEY_DOWN))  hero.run.fps -= 2.0f;
        if (hero.run.fps < 1.0f)     hero.run.fps = 1.0f;

        // --- Interacción con moon (sin clase, modo simple) ---
        bool moonHover = false;
        bool moonClick = false;
        {
            Vector2 mp = GetMousePosition();
            if (moonTex.id != 0) {
                moonHover = CheckCollisionPointRec(mp, moonRect);
                if (moonHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    moonClick = true;
                    isNight = !isNight;  // alternar fondo día/noche
                    TraceLog(LOG_INFO, "Moon clickeada! Cambiando fondo");
                }
            }
        }

        // Draw
        BeginDrawing();
        ClearBackground(isNight ? BLACK : RAYWHITE);

        // dibujar piso de referencia
        DrawLine(0, (int)GROUND_Y, W, (int)GROUND_Y, DARKGRAY);
       
      // luna
        if (moonTex.id != 0) {
            DrawTextureEx(moonTex, { moonRect.x, moonRect.y }, 0.0f, MOON_SCALE, WHITE);
            // debug opcional:
            // DrawRectangleLines((int)moonRect.x, (int)moonRect.y, (int)moonRect.width, (int)moonRect.height, RED);
        }

                
        // --- HUD de instrucciones ---
        DrawText("MAVI 2025 - TP3 - Triana.", 40, 40, 36, DARKGRAY);
        DrawText("SPACE para saltar.Corre en el lugar.", 40, 340, 36, DARKGRAY);
        DrawText("Presiona R para volver al modo estatico (frame 1)", 40, 380, 36, DARKGRAY);
        DrawText("Con flechas arriba y abajo  controlan la velocidad de frames", 40, 420, 36, RED);
        DrawText("Click en la luna!", 1500, 40, 60, BLACK);



        //estrellas
        for (const Vector2& pos : starPositions) {
            DrawText("*", (int)pos.x, (int)pos.y, 60, RAYWHITE);
        }

        // --- Mostrar mensaje gigante al presionar M ---
        if (IsKeyDown(KEY_M)) {
            const char* msg = "MAVI 2025";
            int fontSize = 300; // tamaño gigante, 
            int textWidth = MeasureText(msg, fontSize);
            int textX = W / 2 - textWidth / 2; // centrar horizontalmente
            int textY = H / 2 - fontSize / 2 - 250;  // centrar verticalmente

            // Color cambia según fondo
            Color color = isNight ? RAYWHITE : BLACK;
            DrawText(msg, textX, textY, fontSize, color);
        }

        // --- HUD de diagnóstico visual ---
        DrawText(TextFormat("Frames cargados: %i", (int)hero.run.frames.size()), 40, 90, 36, DARKGRAY);
        DrawText(TextFormat("Anim FPS: %.1f | Frame actual: %i",
            hero.run.fps, hero.run.frame),
            40, 130, 36, DARKGRAY);

		// Diagnóstico de audio
        DrawText(IsAudioDeviceReady() ? "Audio: OK" : "Audio: FALLA",
            40, 210, 32, IsAudioDeviceReady() ? DARKGREEN : RED);

        if (hero.sfxJump) {
            DrawText("SFX jump cargado", 40, 245, 28, DARKGRAY);
        }
        else {
            DrawText("SFX jump NO asignado", 40, 245, 28, MAROON);
        }
       
		// Estado de animación
        DrawText(hero.isRunning ? "Estado: corriendo" : "Estado: estatico",
            40, 280, 32, hero.isRunning ? DARKGREEN : MAROON);

        hero.Draw();

        EndDrawing();
    }

    hero.run.UnloadAll();
    if (hero.sfxJump) UnloadSound(jump);
    if (moonTex.id != 0) UnloadTexture(moonTex);
    CloseAudioDevice();

    CloseWindow();
    return 0;
}
