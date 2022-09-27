/*******************************************************************************************
 *
 *   raylib - classic game: space invaders
 *
 *   Sample game developed by Ian Eito, Albert Martos and Ramon Santamaria
 *
 *   This game has been created using raylib v1.3 (www.raylib.com)
 *   raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
 *
 *   Copyright (c) 2015 Ramon Santamaria (@raysan5)
 *
 ********************************************************************************************/

#include "raylib.h"
#include <stdio.h>
#include <string.h>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

//----------------------------------------------------------------------------------
// Some Defines
//----------------------------------------------------------------------------------
#define NUM_SHOOTS 50
#define NUM_MAX_ENEMIES 900
#define NUM_MAX_ENEMIES_SHOT 50
#define ROW 15
#define FIRST_WAVE 60
#define SECOND_WAVE 20
#define THIRD_WAVE 50
#define PATH "saveData.txt"
#define HEAL_CHANCE 3  // 3% dos inimigos curarao
#define PROG_SPEED 0.15 // de 0  a 1

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef enum
{
    FIRST = 0,
    SECOND,
    THIRD
} EnemyWave;

typedef struct Player
{
    Rectangle rec;
    Vector2 speed;
    Color color;
    int hp;
} Player;

typedef struct Enemy
{
    Rectangle rec;
    Vector2 speed;
    bool active;
    Color color;
    bool dead;
    bool heal;
} Enemy;

typedef struct Shoot
{
    Rectangle rec;
    Vector2 speed;
    bool active;
    Color color;
} Shoot;

typedef struct SaveData
{
    char name[10];
    int score;
} SaveData;

//------------------------------------------------------------------------------------
// Global Variables Declaration
//------------------------------------------------------------------------------------
static const int screenWidth = 600;
static const int screenHeight = 600;

static bool gameOver = false;
static bool waitingForRestart = false;
static bool pause = false;
static int score = 0;
static bool victory = false;

static Player player = {0};
static Enemy enemy[NUM_MAX_ENEMIES] = {0};
static Shoot shoot[NUM_SHOOTS] = {0};
static EnemyWave wave = {0};

static int shootRate = 0;
static float alpha = 0.0f;

static int activeEnemies = 0;
static int enemiesKill = 0;
static bool smooth = false;

float enemyBehaviourTimerCounter = 0;
int enemyMoveToSideCounter = 0;
bool isEnemyMovingLeft = false;
bool goingDown = false;

float progression;
SaveData datas[10] = {0};
//------------------------------------------------------------------------------------
// Module Functions Declaration (local)
//------------------------------------------------------------------------------------
// static void InitGame(void);        // Initialize game
static void UpdateGame(void);      // Update game (one frame)
static void DrawGame(void);        // Draw game (one frame)
static void UnloadGame(void);      // Unload game
static void UpdateDrawFrame(void); // Update and Draw (one frame)
static void RezetGame(void);       // When player got hitted but still have hp
static void StartGame(void);       // When all player's hp are gone || First time player plays

void PlayerMovement();
void PlayerCollisionWithEnemies();
void InitPlayer(bool);
void LoseHp();
void GainHP();

void EnemyBehaviour();
void InitEnemies();
void InitRowOfEnemies();

void ShotInit();
void ShotBehaviour();
void InitShots();

void LoadGame();
void Save(SaveData);
void SaveGame();
bool CheckScore(int);
//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    LoadGame();
    // Initialization (Note windowTitle is unused on Android)
    //---------------------------------------------------------
    InitWindow(screenWidth, screenHeight, "Planet Invaders");

    StartGame();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        // Update and Draw
        //----------------------------------------------------------------------------------
        UpdateDrawFrame();
        //----------------------------------------------------------------------------------
    }
#endif
    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadGame(); // Unload loaded data (textures, sounds, models...)

    CloseWindow(); // Close window and OpenGL context

    SaveGame();
    //--------------------------------------------------------------------------------------

    return 0;
}

//------------------------------------------------------------------------------------
// Module Functions Definitions (local)
//------------------------------------------------------------------------------------

// Initialize game variables
void StartGame(void)
{
    // Initialize game variables
    shootRate = 0;
    pause = false;
    gameOver = false;
    waitingForRestart = false;
    victory = false;
    smooth = false;
    wave = FIRST;
    activeEnemies = FIRST_WAVE;
    enemiesKill = 0;
    score = 0;
    alpha = 0;
    isEnemyMovingLeft = false;
    goingDown = false;
    progression = 1;
    // Initialize player
    InitPlayer(true);

    // Initialize enemies
    InitEnemies();

    // Initialize shoots
    InitShots();
}

void RezetGame(void)
{
    // Initialize game variables
    shootRate = 0;
    pause = false;
    gameOver = false;
    waitingForRestart = false;
    smooth = false;
    // wave = FIRST;
    activeEnemies = FIRST_WAVE;
    // enemiesKill = 0;
    // score = 0;
    alpha = 0;
    isEnemyMovingLeft = false;
    goingDown = false;
    // Initialize player
    InitPlayer(false);

    // Initialize enemies
    InitEnemies();

    // Initialize shoots
    InitShots();
}

// Update game (one frame)
void UpdateGame(void)
{
    if (!gameOver && !waitingForRestart)
    {
        if (IsKeyPressed('P'))
            pause = !pause;

        if (!pause)
        {
            // Player
            PlayerMovement();
            PlayerCollisionWithEnemies();

            // Enemy
            EnemyBehaviour();

            // Player Shoot
            ShotInit();
            ShotBehaviour();
        }
    }
    else
    {
        if (IsKeyPressed(KEY_ENTER))
        {
            if (gameOver)
            {
                StartGame();
                gameOver = false;
            }
            if (waitingForRestart)
            {
                waitingForRestart = false;
                RezetGame();
            }
        }
    }
}

// Draw game (one frame)
void DrawGame(void)
{
    BeginDrawing();

    ClearBackground(RAYWHITE);

    if (!gameOver && !waitingForRestart)
    {
        DrawRectangleRec(player.rec, player.color);

        if (wave == FIRST)
            DrawText("FIRST WAVE", screenWidth / 2 - MeasureText("FIRST WAVE", 40) / 2, screenHeight / 2 - 40, 40, Fade(BLACK, alpha));
        else if (wave == SECOND)
            DrawText("SECOND WAVE", screenWidth / 2 - MeasureText("SECOND WAVE", 40) / 2, screenHeight / 2 - 40, 40, Fade(BLACK, alpha));
        else if (wave == THIRD)
            DrawText("THIRD WAVE", screenWidth / 2 - MeasureText("THIRD WAVE", 40) / 2, screenHeight / 2 - 40, 40, Fade(BLACK, alpha));

        for (int i = 0; i < activeEnemies; i++)
        {
            if (enemy[i].active)
                DrawRectangleRec(enemy[i].rec, enemy[i].color);
        }

        for (int i = 0; i < NUM_SHOOTS; i++)
        {
            if (shoot[i].active)
                DrawRectangleRec(shoot[i].rec, shoot[i].color);
        }

        DrawText(TextFormat("%04i", score), 20, 20, 40, GRAY);

        if (victory)
            DrawText("YOU WIN", screenWidth / 2 - MeasureText("YOU WIN", 40) / 2, screenHeight / 2 - 40, 40, BLACK);

        if (pause)
            DrawText("GAME PAUSED", screenWidth / 2 - MeasureText("GAME PAUSED", 40) / 2, screenHeight / 2 - 40, 40, GRAY);

        for (int i = 0; i < player.hp; i++)
        {
            DrawRectangle(screenWidth - 40 * (i + 1), 20, 20, 20, GREEN);
        }
    }
    else
    {
        DrawText("PRESS [ENTER] TO PLAY AGAIN", GetScreenWidth() / 2 - MeasureText("PRESS [ENTER] TO PLAY AGAIN", 20) / 2, GetScreenHeight() / 2 - 50, 20, GRAY);
    }

    EndDrawing();
}

// Unload game variables
void UnloadGame(void)
{
    // TODO: Unload all dynamic loaded data (textures, sounds, models...)
}

// Update and Draw (one frame)
void UpdateDrawFrame(void)
{
    UpdateGame();
    DrawGame();
}

//=====================================
//=============PLAYER==================
//=====================================

void PlayerMovement()
{
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown('D'))
        player.rec.x += player.speed.x;
    if (IsKeyDown(KEY_LEFT) || IsKeyDown('A'))
        player.rec.x -= player.speed.x;
    if (IsKeyDown(KEY_UP) || IsKeyDown('W'))
        player.rec.y -= player.speed.y;
    if (IsKeyDown(KEY_DOWN) || IsKeyDown('S'))
        player.rec.y += player.speed.y;

    if (player.rec.x <= 0)
        player.rec.x = 0;
    if (player.rec.x + player.rec.width >= screenWidth)
        player.rec.x = screenWidth - player.rec.width;
    if (player.rec.y <= 0)
        player.rec.y = 0;
    if (player.rec.y + player.rec.height >= screenHeight)
        player.rec.y = screenHeight - player.rec.height;
}

void PlayerCollisionWithEnemies()
{
    for (int i = 0; i < activeEnemies; i++)
    {
        if (enemy[i].rec.y <= 5 || CheckCollisionRecs(player.rec, enemy[i].rec))
            LoseHp();
    }
}

void InitPlayer(bool startGame)
{
    player.rec.x = 20;
    player.rec.y = screenHeight - 50 - player.rec.height;
    player.rec.width = 20;
    player.rec.height = 20;
    player.speed.x = 5;
    player.speed.y = 0;
    player.color = BLACK;
    if (startGame)
        player.hp = 3;
}

void LoseHp()
{
    if (!waitingForRestart && !gameOver)
    {
        player.hp--;
        if (player.hp <= 0)
        {
            gameOver = true;
            if (CheckScore(score))
            {
                SaveData toSave = {"IRRA", score};
                Save(toSave);
            }
        }
        else
            waitingForRestart = true;
    }
}

void GainHP()
{
    player.hp++;
    if (player.hp > 3)
        player.hp = 3;
}

//=====================================
//==============ENEMY==================
//=====================================
void EnemyBehaviour()
{
    enemyBehaviourTimerCounter += GetFrameTime();
    if (enemyBehaviourTimerCounter >= progression)
    {
        enemyMoveToSideCounter++;
        if (enemyMoveToSideCounter >= 30)
        {
            enemyMoveToSideCounter = 0;
            isEnemyMovingLeft = !isEnemyMovingLeft;
            goingDown = true;
        }

        for (int i = 0; i < activeEnemies; i++)
        {
            if (enemy[i].active)
            {
                if (goingDown)
                {
                    enemy[i].rec.y += enemy[i].speed.y;
                }
                else
                {
                    enemy[i].rec.x += enemy[i].speed.x * (isEnemyMovingLeft ? -1 : 1);
                }
            }
        }

        if (goingDown)
        {
            progression = progression * (1 - PROG_SPEED);
            if (isEnemyMovingLeft == false)
                InitRowOfEnemies();
            goingDown = false;
        }

        enemyBehaviourTimerCounter = 0;
    }
}

void InitEnemies()
{
    for (int i = 0; i < NUM_MAX_ENEMIES; i++)
    {
        enemy[NUM_MAX_ENEMIES - 1 - i].rec.width = 10;
        enemy[NUM_MAX_ENEMIES - 1 - i].rec.height = 10;
        enemy[NUM_MAX_ENEMIES - 1 - i].rec.x = ((i % ROW) * 35 + enemy[i].rec.width / 2);
        if (i <= FIRST_WAVE)
            enemy[NUM_MAX_ENEMIES - 1 - i].rec.y = (((i / ROW) + 1) * 15) + 75;
        else
            enemy[NUM_MAX_ENEMIES - 1 - i].rec.y = (45 + 75);
        enemy[NUM_MAX_ENEMIES - 1 - i].speed.x = 3;
        enemy[NUM_MAX_ENEMIES - 1 - i].speed.y = 8 * 4;
        enemy[NUM_MAX_ENEMIES - 1 - i].active = true;
        enemy[NUM_MAX_ENEMIES - 1 - i].dead = false;
        enemy[NUM_MAX_ENEMIES - 1 - i].heal = GetRandomValue(0, 100) <= HEAL_CHANCE;
        enemy[NUM_MAX_ENEMIES - 1 - i].color = enemy[NUM_MAX_ENEMIES - 1 - i].heal ? GREEN : GRAY;
    }
}

void InitRowOfEnemies()
{
    for (int i = 0; i < NUM_MAX_ENEMIES; i++)
    {
        if (!enemy[NUM_MAX_ENEMIES - 1 - i].dead)
        {
            enemy[NUM_MAX_ENEMIES - 1 - i].active = true;
        }
    }
    activeEnemies += ROW;
}

//=====================================
//===============SHOT==================
//=====================================

void ShotInit()
{
    if (IsKeyDown(KEY_SPACE))
    {
        shootRate += 5;

        for (int i = 0; i < NUM_SHOOTS; i++)
        {
            if (!shoot[i].active && shootRate % 20 == 0)
            {
                shoot[i].rec.x = player.rec.x;
                shoot[i].rec.y = player.rec.y + player.rec.height / 4;
                shoot[i].active = true;
                break;
            }
        }
    }
}

void ShotBehaviour()
{
    for (int i = 0; i < NUM_SHOOTS; i++)
    {
        if (shoot[i].active)
        {
            // Movement
            shoot[i].rec.x += shoot[i].speed.x;
            shoot[i].rec.y += shoot[i].speed.y;

            // Collision with enemy
            for (int j = 0; j < activeEnemies; j++)
            {
                if (enemy[j].active)
                {
                    if (CheckCollisionRecs(shoot[i].rec, enemy[j].rec))
                    {
                        shoot[i].active = false;
                        // enemy[j].rec.x = GetRandomValue(screenWidth, screenWidth + 1000);
                        // enemy[j].rec.y = GetRandomValue(0, screenHeight - enemy[j].rec.height);
                        enemy[j].active = false;
                        enemy[j].dead = true;
                        shootRate = 0;
                        enemiesKill++;
                        score += 100;
                        if (enemy[i].heal)
                            GainHP();
                    }

                    if (shoot[i].rec.x + shoot[i].rec.width >= screenWidth || shoot[i].rec.y + shoot[i].rec.height <= 0)
                    {
                        shoot[i].active = false;
                        shootRate = 0;
                    }
                }
            }
        }
    }
}

void InitShots()
{
    for (int i = 0; i < NUM_SHOOTS; i++)
    {
        shoot[i].rec.x = player.rec.x;
        shoot[i].rec.y = player.rec.y + player.rec.height / 2;
        shoot[i].rec.width = 5;
        shoot[i].rec.height = 10;
        shoot[i].speed.x = 0;
        shoot[i].speed.y = -7;
        shoot[i].active = false;
        shoot[i].color = MAROON;
    }
}

//=====================================
//=============LOAD/SAVE===============TODO!!!
//=====================================
#include <stdio.h>
// Loads from disc (once when start program)
void LoadGame()
{
    // open files
    FILE *f;
    f = fopen(PATH, "r");

    if (f != NULL)
    {
        for (int i = 0; i < 10; i++)
        {
            fscanf(f, "%s %d", datas[i].name, &datas[i].score);
        }
    }
    fclose(f);
}

// Saves at disc (once when close program)
void SaveGame()
{
    FILE *f;
    f = fopen(PATH, "w");
    for (int i = 0; i < 10; i++)
    {
        // fprintf(f, "%s %d", datas[i].name, &datas[i].score); //COMENTED BCS AVAST
        printf("%s %d", datas[i].name, datas[i].score);
    }
    fclose(f);
}

// Data that must be saved ()
void Save(SaveData newData)
{
    // Holds new position
    int i = 0;
    for (i = 0; newData.score < datas[i].score || i < 10; i++)
    {
    }

    // Updates order
    for (int j = 9; j > i; j++)
    {
        datas[j].score = datas[j - 1].score;
        strcpy(datas[j].name, datas[j - 1].name);
    }

    // Places new data at correct position
    datas[i].score = newData.score;
    strcpy(datas[i].name, newData.name);
}

bool CheckScore(int score)
{
    for (int i = 0; i < 10; i++)
    {
        if (score > datas[i].score)
            return true;
    }
    return false;
}
