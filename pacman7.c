#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define PACMAN_SIZE 20
#define GHOST_SIZE 20
#define MAZE_WIDTH 40
#define MAZE_HEIGHT 21
#define FOOD_COUNT 10
#define GHOST_MOVE_INTERVAL 300
#define CHASING_GHOST_MOVE_INTERVAL 500
#define CHERRY_RESPAWN_INTERVAL 35000
#define STRAWBERRY_RESPAWN_INTERVAL 45000
#define APPLE_RESPAWN_INTERVAL 20000
#define MUSHROOM_RESPAWN_INTERVAL 17000
#define PACMAN_SPEED_BOOST_DURATION 30000
#define MAX_LIVES 3
#define MAX_SCORES 25
#define NAME_LENGTH 100

TTF_Font *font = NULL;


char maze[MAZE_HEIGHT][MAZE_WIDTH + 1] = {
    "########################################",
    "#..................##..................#",
    "#...####...#####...##...#####...####...#",
    "#......................................#",
    "#...####...##...########...##...####...#",
    "#..........##......##......##..........#",
    "######.....##### ..##.. #####.....######",
    "#..........##..............##..........#",
    "#######....##...########...##....#######",
    "#...............########...............#",
    "#######....##...########...##....#######",
    "#..........##..............##..........#",
    "######.....##...########...##.....######",
    "#..................##..................#",
    "#...####...#####...##...#####...####...#",
    "#.....##........................##.....#",
    "###...##...##...########...##...##...###",
    "#..........##......##......##..........#",
    "#....##########....##....##########....#",
    "#......................................#",
    "########################################"
};

typedef struct {
    char name[NAME_LENGTH];
    int score;
    time_t date;
    Uint32 duration;
} ScoreEntry;

ScoreEntry scores[MAX_SCORES];
int scoreCount = 0;


bool food[MAZE_HEIGHT][MAZE_WIDTH];

TTF_Font* loadFont(const char* fontPath, int size) {
    TTF_Font* font = TTF_OpenFont(fontPath, size);
    return font;
}

typedef enum {
    DIRECTION_RIGHT,
    DIRECTION_LEFT,
    DIRECTION_UP,
    DIRECTION_DOWN
} Direction;

typedef struct {
    int x, y;
    bool mouthOpen;
    Uint32 lastToggleTime;
    int score;
    int lives;
    bool canEatGhost;
    Direction direction;
} PacMan;

typedef struct {
    int x, y;
    SDL_Texture *texture;
    SDL_Texture *originalTexture;
    Uint32 lastMoveTime;
    bool isChasing;
    Uint32 moveInterval;
} Ghost;

typedef struct {
    int x, y;
    bool isVisible;
    Uint32 lastSpawnTime;
    SDL_Texture *texture;
} Cherry;

typedef struct {
    int x, y;
    bool isVisible;
    Uint32 lastSpawnTime;
    SDL_Texture *texture;
} Strawberry;

typedef struct {
    int x, y;
    bool isVisible;
    Uint32 spawnTime;
    Uint32 lastSpawnTime;
    SDL_Texture *texture;
} Apple;

typedef struct {
    int x, y;
    bool isVisible;
    Uint32 spawnTime;
    Uint32 lastSpawnTime;
    SDL_Texture *texture;
} Mushroom;

void spawnMushroom(Mushroom *mushroom) {
    int x, y;
    do {
        x = rand() % MAZE_WIDTH;
        y = rand() % MAZE_HEIGHT;
    } while (maze[y][x] == '#' || food[y][x]);
    mushroom->x = x;
    mushroom->y = y;
    mushroom->isVisible = true;
    mushroom->spawnTime = SDL_GetTicks() + 5000;
}

void drawMushroom(SDL_Renderer *renderer, Mushroom *mushroom) {
    if (mushroom->isVisible) {
        SDL_Rect mushroomRect = { mushroom->x * (SCREEN_WIDTH / MAZE_WIDTH), mushroom->y * (SCREEN_HEIGHT / MAZE_HEIGHT), PACMAN_SIZE, PACMAN_SIZE };
        SDL_RenderCopy(renderer, mushroom->texture, NULL, &mushroomRect);
    }
}

bool checkMushroomCollision(PacMan *pacman, Mushroom *mushroom) {
    return mushroom->isVisible && pacman->x == mushroom->x && pacman->y == mushroom->y;
}

void handleMushroompower(PacMan *pacman, Mushroom *mushroom){
    if(checkMushroomCollision(pacman, mushroom)){
        pacman->lives--;
        mushroom->isVisible = false; 
        mushroom->lastSpawnTime = SDL_GetTicks(); 
    }
}

void spawnApple(Apple *apple) {
    int x, y;
    do {
        x = rand() % MAZE_WIDTH;
        y = rand() % MAZE_HEIGHT;
    } while (maze[y][x] == '#' || food[y][x]);
    apple->x = x;
    apple->y = y;
    apple->isVisible = true;
    apple->spawnTime = SDL_GetTicks() + 5000;
}

void drawApple(SDL_Renderer *renderer, Apple *apple) {
    if (apple->isVisible) {
        SDL_Rect appleRect = { apple->x * (SCREEN_WIDTH / MAZE_WIDTH), apple->y * (SCREEN_HEIGHT / MAZE_HEIGHT), PACMAN_SIZE, PACMAN_SIZE };
        SDL_RenderCopy(renderer, apple->texture, NULL, &appleRect);
    }
}

bool checkAppleCollision(PacMan *pacman, Apple *apple) {
    return apple->isVisible && pacman->x == apple->x && pacman->y == apple->y;
}

void handleApplepower(PacMan *pacman, Apple *apple){
    if(checkAppleCollision(pacman, apple)){
        if(pacman->lives < MAX_LIVES){
            pacman->lives++;
        }
        apple->isVisible = false; 
        apple->lastSpawnTime = SDL_GetTicks(); 
    }
}

void spawnCherry(Cherry *cherry) {
    int x, y;
    do {
        x = rand() % MAZE_WIDTH;
        y = rand() % MAZE_HEIGHT;
    } while (maze[y][x] == '#' || food[y][x]);
    cherry->x = x;
    cherry->y = y;
    cherry->isVisible = true;
    cherry->lastSpawnTime = SDL_GetTicks();
}

bool checkCherryCollision(PacMan *pacman, Cherry *cherry) {
    return cherry->isVisible && pacman->x == cherry->x && pacman->y == cherry->y;
}

void drawCherry(SDL_Renderer *renderer, Cherry *cherry) {
    if (cherry->isVisible) {
        SDL_Rect cherryRect = { cherry->x * (SCREEN_WIDTH / MAZE_WIDTH), cherry->y * (SCREEN_HEIGHT / MAZE_HEIGHT), PACMAN_SIZE, PACMAN_SIZE };
        SDL_RenderCopy(renderer, cherry->texture, NULL, &cherryRect);
    }
}

void spawnStrawberry(Strawberry *strawberry) {
    int x, y;
    do {
        x = rand() % MAZE_WIDTH;
        y = rand() % MAZE_HEIGHT;
    } while (maze[y][x] == '#' || food[y][x]);
    strawberry->x = x;
    strawberry->y = y;
    strawberry->isVisible = true;
    strawberry->lastSpawnTime = SDL_GetTicks();
}

bool checkStrawberryCollision(PacMan *pacman, Strawberry *strawberry) {
    return strawberry->isVisible && pacman->x == strawberry->x && pacman->y == strawberry->y;
}

void drawStrawberry(SDL_Renderer *renderer, Strawberry *strawberry) {
    if (strawberry->isVisible) {
        SDL_Rect strawberryRect = { strawberry->x * (SCREEN_WIDTH / MAZE_WIDTH), strawberry->y * (SCREEN_HEIGHT / MAZE_HEIGHT), PACMAN_SIZE, PACMAN_SIZE };
        SDL_RenderCopy(renderer, strawberry->texture, NULL, &strawberryRect);
    }
}

void respawnGhost(Ghost *ghost) {
    int x, y;
    do {
        x = rand() % MAZE_WIDTH;
        y = rand() % MAZE_HEIGHT;
    } while (maze[y][x] == '#');
    ghost->x = x;
    ghost->y = y;
}

bool canEatGhosts = false;

void handleCherryEffect(PacMan *pacman, Ghost *ghosts[], int ghostCount) {
    if (canEatGhosts) {
        for (int i = 0; i < ghostCount; i++) {
            if (pacman->x == ghosts[i]->x && pacman->y == ghosts[i]->y) {
                respawnGhost(ghosts[i]); 
                canEatGhosts = false; 
            }
        }
    }
}

void eatCherry(PacMan *pacman) {
    pacman->canEatGhost = true;
    canEatGhosts = true; 
}


void moveChasingGhost(Ghost *ghost, PacMan *pacman) {
    Uint32 currentTime = SDL_GetTicks();
    if (currentTime - ghost->lastMoveTime < ghost->moveInterval) return;
    ghost->lastMoveTime = currentTime;
    
    int dx = pacman->x - ghost->x;
    int dy = pacman->y - ghost->y;

    int newX = ghost->x;
    int newY = ghost->y;
    
    if (abs(dx) > abs(dy)) {
        newX += (dx > 0) ? 1 : -1;
        if (maze[newY][newX] == '#') {
            newX = ghost->x; 
            newY += (dy > 0) ? 1 : -1;
        }
    } else {
        newY += (dy > 0) ? 1 : -1;
        if (maze[newY][newX] == '#') {
            newY = ghost->y;  
            newX += (dx > 0) ? 1 : -1;
        }
    }

    if (maze[newY][newX] != '#') {
        ghost->x = newX;
        ghost->y = newY;
    }
}

void clearFood() {
    for (int i = 0; i < MAZE_HEIGHT; i++) {
        for (int j = 0; j < MAZE_WIDTH; j++) {
            food[i][j] = false;
        }
    }
}

void generateRandomFood() {
    clearFood();
    int placed = 0;
    while (placed < FOOD_COUNT) {
        int x = rand() % MAZE_WIDTH;
        int y = rand() % MAZE_HEIGHT;
        if (maze[y][x] == '.' && !food[y][x]) {
            food[y][x] = true;
            placed++;
        }
    }
}

bool isFoodRemaining() {
    for (int i = 0; i < MAZE_HEIGHT; i++) {
        for (int j = 0; j < MAZE_WIDTH; j++) {
            if (food[i][j]) {
                return true;
            }
        }
    }
    return false;
}

SDL_Texture* loadTexture(const char* path, SDL_Renderer* renderer) {
    SDL_Surface* tempSurface = IMG_Load(path);
    if (!tempSurface) {
        printf("Failed to load image %s: %s\n", path, IMG_GetError());
        return NULL;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, tempSurface);
    SDL_FreeSurface(tempSurface);
    return texture;
}

SDL_Texture* renderTextToTexture(SDL_Renderer* renderer, TTF_Font* font, const char* text, SDL_Color color) {
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, text, color);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    return textTexture;
}

int showMenu(SDL_Renderer* renderer, TTF_Font* font) {
    const char* options[] = {"Start Game", "Scores and Records","Exit"};
    int n_options = sizeof(options) / sizeof(options[0]);
    int highlight = 0;
    SDL_Event event;
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color green = {0, 255, 0, 255};

    while (1) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        
        SDL_Texture* titleTexture = renderTextToTexture(renderer, font, "Welcome to Pac-Man Game!", white);
        SDL_Rect titleRect = {SCREEN_WIDTH / 2 - 150, 50, 300, 50};
        SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
        SDL_DestroyTexture(titleTexture);

        
        for (int i = 0; i < n_options; i++) {
            SDL_Texture* optionTexture = renderTextToTexture(renderer, font, options[i], (i == highlight) ? green : white);
            int textWidth, textHeight;
            SDL_QueryTexture(optionTexture, NULL, NULL, &textWidth, &textHeight);
            SDL_Rect optionRect = {SCREEN_WIDTH / 2 - textWidth / 2, 150 + i * 50, textWidth, textHeight};
            SDL_RenderCopy(renderer, optionTexture, NULL, &optionRect);
            SDL_DestroyTexture(optionTexture);
        }

        SDL_RenderPresent(renderer);

        
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                return 2; 
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        highlight = (highlight - 1 + n_options) % n_options;
                        break;
                    case SDLK_DOWN:
                        highlight = (highlight + 1) % n_options;
                        break;
                    case SDLK_RETURN:
                        return highlight; 
                }
            }
        }
    }
}

void getPlayerName(SDL_Renderer* renderer, TTF_Font* font, char* playerName) {
    SDL_Event event;
    SDL_Color white = {255, 255, 255, 255};
    int nameLength = 0;
    playerName[0] = '\0';
    bool done = false;

    while (!done) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

    
        SDL_Texture* promptTexture = renderTextToTexture(renderer, font, "Enter your name:", white);
        SDL_Rect promptRect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 50, 200, 30};
        SDL_RenderCopy(renderer, promptTexture, NULL, &promptRect);
        SDL_DestroyTexture(promptTexture);

        
        SDL_Texture* nameTexture = renderTextToTexture(renderer, font, playerName, white);
        SDL_Rect nameRect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 , 200, 30};
        SDL_RenderCopy(renderer, nameTexture, NULL, &nameRect);
        SDL_DestroyTexture(nameTexture);

        SDL_RenderPresent(renderer);

        
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                exit(0);
            } else if (event.type == SDL_TEXTINPUT) {
                if (nameLength < NAME_LENGTH - 1) {
                    playerName[nameLength++] = event.text.text[0];
                    playerName[nameLength] = '\0';
                }
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_RETURN && nameLength > 0) {
                    done = true;
                    return; 
                } else if (event.key.keysym.sym == SDLK_BACKSPACE && nameLength > 0) {
                    playerName[--nameLength] = '\0';
                }
            }
        }
    }
}

void saveScoresToFile(const char* filename) {
    FILE* file = fopen(filename, "w");

    for (int i = 0; i < scoreCount; i++) {
        fprintf(file, "%s %d %ld\n", scores[i].name, scores[i].score, scores[i].date);
    }

    fclose(file);
}

void loadScoresFromFile(const char* filename) {
    FILE* file = fopen(filename, "r");
    scoreCount = 0;
    while (fscanf(file, "%s %d %ld", scores[scoreCount].name, &scores[scoreCount].score, &scores[scoreCount].date) == 3) {
        scoreCount++;
        if (scoreCount >= MAX_SCORES) break;
    }

    fclose(file);
}

void saveScore(const char* playerName, int score) {
    if (scoreCount < MAX_SCORES) {
        strcpy(scores[scoreCount].name, playerName);
        scores[scoreCount].score = score;
        scores[scoreCount].date = time(NULL);
        scoreCount++;
    } else {
        int minIndex = 0;
        for (int i = 1; i < MAX_SCORES; i++) {
            if (scores[i].score < scores[minIndex].score) {
                minIndex = i;
            }
        }
        if (score > scores[minIndex].score) {
            strcpy(scores[minIndex].name, playerName);
            scores[minIndex].score = score;
            scores[minIndex].date = time(NULL);
        }
    }

    for (int i = 0; i < scoreCount - 1; i++) {
        for (int j = i + 1; j < scoreCount; j++) {
            if (scores[i].score < scores[j].score) {
                ScoreEntry temp = scores[i];
                scores[i] = scores[j];
                scores[j] = temp;
            }
        }
    }

    saveScoresToFile("score.txt");
}

void showScores(SDL_Renderer* renderer, TTF_Font* font) {
    SDL_Event event;
    SDL_Color white = {255, 255, 255, 255};
    ScoreEntry filteredScores[MAX_SCORES];
    int filteredScoreCount = 0;

    for (int i = 0; i < scoreCount; i++) {
        bool playerExists = false;
        for (int j = 0; j < filteredScoreCount; j++) {
            if (strcmp(scores[i].name, filteredScores[j].name) == 0) {
                playerExists = true;
                if (scores[i].score > filteredScores[j].score) {
                    filteredScores[j] = scores[i]; 
                }
                break;
            }
        }
        if (!playerExists) {
            filteredScores[filteredScoreCount++] = scores[i]; 
        }
    }

    while (1) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        
        SDL_Texture* titleTexture = renderTextToTexture(renderer, font, "Top Scores", white);
        SDL_Rect titleRect = {SCREEN_WIDTH / 2 - 75, 50, 150, 30};
        SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
        SDL_DestroyTexture(titleTexture);

        
        for (int i = 0; i < filteredScoreCount; i++) {
            char scoreText[100];
            char dateText[50];
            struct tm* timeinfo = localtime(&filteredScores[i].date);
            strftime(dateText, sizeof(dateText), "%Y-%m-%d %H:%M:%S", timeinfo);
            snprintf(scoreText, sizeof(scoreText), "%s - %d (%s)", filteredScores[i].name, filteredScores[i].score, dateText);
            SDL_Texture* scoreTexture = renderTextToTexture(renderer, font, scoreText, white);
            SDL_Rect scoreRect = {SCREEN_WIDTH / 2 - 100, 100 + i * 30, 200, 30};
            SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
            SDL_DestroyTexture(scoreTexture);
        }

        SDL_RenderPresent(renderer);

        
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                return;
            }
        }
    }
}

bool showGameOver(SDL_Renderer* renderer, TTF_Font* font, const char* playerName, int score) {
    SDL_Event event;
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color green = {0, 255, 0, 255};
    bool returnToMenu = false;

    while (!returnToMenu) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_Texture* gameOverTexture = renderTextToTexture(renderer, font, "Game Over!", white);
        SDL_Rect gameOverRect = {SCREEN_WIDTH / 2 - 75, 50, 150, 30};
        SDL_RenderCopy(renderer, gameOverTexture, NULL, &gameOverRect);
        SDL_DestroyTexture(gameOverTexture);

        char scoreText[100];
        snprintf(scoreText, sizeof(scoreText), "%s - Score: %d", playerName, score);
        SDL_Texture* scoreTexture = renderTextToTexture(renderer, font, scoreText, white);
        SDL_Rect scoreRect = {SCREEN_WIDTH / 2 - 100, 100, 200, 30};
        SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
        SDL_DestroyTexture(scoreTexture);

        SDL_Texture* returnTexture = renderTextToTexture(renderer, font, "Return to Menu", green);
        SDL_Rect returnRect = {SCREEN_WIDTH / 2 - 100, 150, 200, 30};
        SDL_RenderCopy(renderer, returnTexture, NULL, &returnRect);
        SDL_DestroyTexture(returnTexture);

        SDL_RenderPresent(renderer);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                exit(0);
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_RETURN) {
                    returnToMenu = true;
                    showMenu(renderer, font);
                }
            }
        }
    }
    return true;
}

void displayUI(SDL_Renderer* renderer, TTF_Font* font, int score, int lives, const char* playerName, Uint32 duration) {
    SDL_Color textColor = {255, 255, 255, 255};  
    Uint32  gamestart  = SDL_GetTicks();

    
    SDL_Texture* playerTexture = renderTextToTexture(renderer, font, playerName, textColor);
    SDL_Rect playerRect = {10, 455, 0, 0};  
    SDL_QueryTexture(playerTexture, NULL, NULL, &playerRect.w, &playerRect.h);
    SDL_RenderCopy(renderer, playerTexture, NULL, &playerRect);
    SDL_DestroyTexture(playerTexture);


    char scoreText[50];
    snprintf(scoreText, sizeof(scoreText), "Score: %d", score);
    SDL_Texture* scoreTexture = renderTextToTexture(renderer, font, scoreText, textColor);
    SDL_Rect scoreRect = {290, 455, 0, 0};  
    SDL_QueryTexture(scoreTexture, NULL, NULL, &scoreRect.w, &scoreRect.h);
    SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
    SDL_DestroyTexture(scoreTexture);

    
    char livesText[50];
    snprintf(livesText, sizeof(livesText), "Lives: %d", lives);
    SDL_Texture* livesTexture = renderTextToTexture(renderer, font, livesText, textColor);
    SDL_Rect livesRect = {550, 455, 0, 0};  
    SDL_QueryTexture(livesTexture, NULL, NULL, &livesRect.w, &livesRect.h);
    SDL_RenderCopy(renderer, livesTexture, NULL, &livesRect);
    SDL_DestroyTexture(livesTexture);

    char durationTime[50];
    snprintf(durationTime, sizeof(durationTime), "duration: %d", duration);
    SDL_Texture* durationTimeTexture = renderTextToTexture(renderer, font, durationTime, textColor);
    SDL_Rect durationTimeRect = {390, 455, 0, 0};  
    SDL_QueryTexture(durationTimeTexture, NULL, NULL, &durationTimeRect.w, &durationTimeRect.h);
    SDL_RenderCopy(renderer, durationTimeTexture, NULL, &durationTimeRect);
    SDL_DestroyTexture(durationTimeTexture);
}

void drawMaze(SDL_Renderer *renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
    for (int i = 0; i < MAZE_HEIGHT; i++) {
        for (int j = 0; j < MAZE_WIDTH; j++) {
            if (maze[i][j] == '#') {
                SDL_Rect wall = { j * (SCREEN_WIDTH / MAZE_WIDTH), i * (SCREEN_HEIGHT / MAZE_HEIGHT), (SCREEN_WIDTH / MAZE_WIDTH), (SCREEN_HEIGHT / MAZE_HEIGHT) };
                SDL_RenderFillRect(renderer, &wall);
            }
        }
    }
}

void drawFood(SDL_Renderer *renderer) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int i = 0; i < MAZE_HEIGHT; i++) {
        for (int j = 0; j < MAZE_WIDTH; j++) {
            if (food[i][j]) {
                SDL_Rect dot = { j * (SCREEN_WIDTH / MAZE_WIDTH) + (SCREEN_WIDTH / MAZE_WIDTH) / 4, i * (SCREEN_HEIGHT / MAZE_HEIGHT) + (SCREEN_HEIGHT / MAZE_HEIGHT) / 4, 8, 8 };
                SDL_RenderFillRect(renderer, &dot);
            }
        }
    }
}

void drawPacMan(SDL_Renderer *renderer, PacMan *pacman, SDL_Texture *openTex, SDL_Texture *closedTex) {
    SDL_Rect pacmanRect = { pacman->x * (SCREEN_WIDTH / MAZE_WIDTH), pacman->y * (SCREEN_HEIGHT / MAZE_HEIGHT), PACMAN_SIZE, PACMAN_SIZE };
    SDL_Texture* currentTexture = pacman->mouthOpen ? openTex : closedTex;

    double angle = 0;
    switch (pacman->direction) {
        case DIRECTION_RIGHT:
            angle = 0;
            break;
        case DIRECTION_LEFT:
            angle = 180;
            break;
        case DIRECTION_UP:
            angle = 270;
            break;
        case DIRECTION_DOWN:
            angle = 90;
            break;
    }

    SDL_RenderCopyEx(renderer, currentTexture, NULL, &pacmanRect, angle, NULL, SDL_FLIP_NONE);
}

void drawGhost(SDL_Renderer *renderer, Ghost *ghost) {
    SDL_Rect ghostRect = { ghost->x * (SCREEN_WIDTH / MAZE_WIDTH), ghost->y * (SCREEN_HEIGHT / MAZE_HEIGHT), GHOST_SIZE, GHOST_SIZE };
    SDL_RenderCopy(renderer, ghost->texture, NULL, &ghostRect);
}


void moveGhost(Ghost *ghost) {
    Uint32 currentTime = SDL_GetTicks();
    if (currentTime - ghost->lastMoveTime < GHOST_MOVE_INTERVAL) return;
    ghost->lastMoveTime = currentTime;


    int directions[4][2] = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} };  

    
    for (int i = 0; i < 4; i++) {
        int j = rand() % 4;
        int tempX = directions[i][0];
        int tempY = directions[i][1];
        directions[i][0] = directions[j][0];
        directions[i][1] = directions[j][1];
        directions[j][0] = tempX;
        directions[j][1] = tempY;
    }

    
    for (int i = 0; i < 4; i++) {
        int newX = ghost->x + directions[i][0];
        int newY = ghost->y + directions[i][1];

        
        if (maze[newY][newX] != '#') {
            ghost->x = newX;
            ghost->y = newY;
            break;  
        }
    }
}

void updateGhostsAppearanceAndSpeed(Ghost* ghosts[], int ghostCount, SDL_Texture* newTexture) {
    for (int i = 0; i < ghostCount; i++) {
        ghosts[i]->texture = newTexture;
        if (ghosts[i]->moveInterval > 100) {
            ghosts[i]->moveInterval -= 50;
        }
    }
}

void revertGhostsAppearanceAndSpeed(Ghost* ghosts[], int ghostCount) {
    for (int i = 0; i < ghostCount; i++) {
        ghosts[i]->texture = ghosts[i]->originalTexture;
        ghosts[i]->moveInterval = GHOST_MOVE_INTERVAL;
    }
}

bool checkCollision(PacMan *pacman, Ghost *ghost) {
    return pacman->x == ghost->x && pacman->y == ghost->y;
}

bool checkGhostCollision(Ghost *chasingGhost1, Ghost *chasingGhost2) {
    return chasingGhost1->x == chasingGhost2->x && chasingGhost1->y == chasingGhost2->y;
}

void handlePacManGhostCollision(PacMan *pacman, Ghost *ghosts[], int ghostCount) {
    for (int i = 0; i < ghostCount; i++) {
        if (checkCollision(pacman, ghosts[i])) {
            if(canEatGhosts){
                respawnGhost(ghosts[i]);
                canEatGhosts = false;
            }
            else{
                pacman->lives--;
                pacman->x = 1;
                pacman->y = 1;
            }
        }
    }
}

void preventGhostCollision(Ghost* ghosts[], int ghostCount) {
    for (int i = 0; i < ghostCount; i++) {
        for (int j = i + 1; j < ghostCount; j++) {
            if (ghosts[i]->isChasing && ghosts[j]->isChasing && checkGhostCollision(ghosts[i], ghosts[j])) {
                ghosts[j]->x += (rand() % 3) - 1;
                ghosts[j]->y += (rand() % 3) - 1;
            }
        }
    }
}

bool canMove(int x, int y) {
    return maze[y][x] != '#';
}



int main(int argc, char *argv[]) {
   
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();
    srand(time(NULL));
    loadScoresFromFile("score.txt");


    SDL_Window *window = SDL_CreateWindow("Pac-Man Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Texture *pacmanOpenTex = loadTexture("3.png", renderer);
    SDL_Texture *pacmanClosedTex = loadTexture("4.png", renderer);
    SDL_Texture *ghostTexNormal1 = loadTexture("1.png", renderer);
    SDL_Texture *ghostTexNormal2 = loadTexture("2.png", renderer);
    SDL_Texture *ghostTexNormal3 = IMG_LoadTexture(renderer, "6.png");
    SDL_Texture *ghostTexNormal4 = IMG_LoadTexture(renderer, "7.png");
    SDL_Texture *ghostTexNormal5 = IMG_LoadTexture(renderer, "7.png");
    SDL_Texture *ghostTexChange = loadTexture("5.png", renderer);
    SDL_Texture *cherryTex = IMG_LoadTexture(renderer, "14.png");
    SDL_Texture *strawberryTex = IMG_LoadTexture(renderer, "8.png");
    SDL_Texture *appleTex = IMG_LoadTexture(renderer, "9.png");
    SDL_Texture *mushroomTex = IMG_LoadTexture(renderer, "10.png");
    
    TTF_Font* font = loadFont("arial.ttf", 22);
    
    int score = 0;
    int lives = 3;
    char playerName[NAME_LENGTH] = "Player";

    int quit = 0;
    SDL_Event event;

    PacMan pacman = {1, 1, true, SDL_GetTicks(), 0, MAX_LIVES, DIRECTION_RIGHT};
    Ghost ghost1 = {10, 10, ghostTexNormal1, ghostTexNormal1, SDL_GetTicks(), false, GHOST_MOVE_INTERVAL};
    Ghost ghost2 = {30, 15, ghostTexNormal2, ghostTexNormal2, SDL_GetTicks(), false, GHOST_MOVE_INTERVAL};
    Ghost ghost3 = {35, 5, ghostTexNormal5, ghostTexNormal5, SDL_GetTicks(), false, GHOST_MOVE_INTERVAL};
    Ghost chasingGhost1 = {5, 5, ghostTexNormal3, ghostTexNormal3, SDL_GetTicks(), true, CHASING_GHOST_MOVE_INTERVAL};
    Ghost chasingGhost2 = {35, 17, ghostTexNormal4, ghostTexNormal4, SDL_GetTicks(), true, CHASING_GHOST_MOVE_INTERVAL};
    Cherry cherry = {0, 0, false, 0, cherryTex};
    Strawberry strawberry = {0, 0, false, 0, strawberryTex};
    Apple apple = {0, 0, false, 0, 0, appleTex};
    Mushroom mushroom = {0, 0, false, 0, 0, mushroomTex};
    spawnCherry(&cherry);
    spawnStrawberry(&strawberry);
    generateRandomFood();
    Ghost* allGhosts[] = {&ghost1, &ghost2, &ghost3, &chasingGhost1, &chasingGhost2};
    int ghostCount = 5;
    Uint32 lastAppearanceChangeTime = SDL_GetTicks();
    handlePacManGhostCollision(&pacman, allGhosts , ghostCount);
    handleCherryEffect(&pacman, allGhosts, ghostCount);
    handleApplepower(&pacman, &apple);
    handleMushroompower(&pacman, &mushroom);
    Uint32 revertTime = 0;
    Uint32 speedBoostStartTime = 0;
    bool isPacmanSpeedBoosted = false;
    bool running = true;
    bool isChanged = false;

    

    while (running) {
        int menuChoice = showMenu(renderer, font);
        saveScoresToFile("score.txt");

        switch (menuChoice) {
            case 0: {
                playerName[0] = '\0';
                getPlayerName(renderer, font, playerName);
                pacman = (PacMan){1, 1, true, SDL_GetTicks(), 0, MAX_LIVES, false, DIRECTION_RIGHT};
                bool gameOver = false; 
                clearFood();
                generateRandomFood();
                spawnCherry(&cherry);
                spawnStrawberry(&strawberry);
                spawnApple(&apple);
                spawnMushroom(&mushroom);
                ghost1 = (Ghost){10, 10, ghostTexNormal1, ghostTexNormal1, SDL_GetTicks(), false, GHOST_MOVE_INTERVAL};
                ghost2 = (Ghost){30, 15, ghostTexNormal2, ghostTexNormal2, SDL_GetTicks(), false, GHOST_MOVE_INTERVAL};
                ghost3 = (Ghost){35, 5, ghostTexNormal5, ghostTexNormal5, SDL_GetTicks(), false, GHOST_MOVE_INTERVAL};
                chasingGhost1 = (Ghost){5, 5, ghostTexNormal3, ghostTexNormal3, SDL_GetTicks(), true, CHASING_GHOST_MOVE_INTERVAL};
                chasingGhost2 = (Ghost){35, 17, ghostTexNormal4, ghostTexNormal4, SDL_GetTicks(), true, CHASING_GHOST_MOVE_INTERVAL};

                
                while (!gameOver) {
                     Uint32 currentTime = SDL_GetTicks();
                     Uint32 durationtime = SDL_GetTicks() / 1000;
                     if (currentTime - pacman.lastToggleTime >= 500) {
                         pacman.mouthOpen = !pacman.mouthOpen;
                         pacman.lastToggleTime = currentTime;
                     }

                     if(!isChanged && currentTime - lastAppearanceChangeTime >= 30000) {
                         updateGhostsAppearanceAndSpeed(allGhosts, ghostCount, ghostTexChange);
                         revertTime = currentTime;
                         isChanged = true;
                     }

                     if (isChanged && currentTime - revertTime >= 5000) {
                         revertGhostsAppearanceAndSpeed(allGhosts, ghostCount);
                         lastAppearanceChangeTime = currentTime;
                         isChanged = false;
                     }

                     if (currentTime - cherry.lastSpawnTime >= CHERRY_RESPAWN_INTERVAL && !cherry.isVisible) {
                         spawnCherry(&cherry);
                     }

                     if (currentTime - apple.lastSpawnTime >= APPLE_RESPAWN_INTERVAL && !apple.isVisible) {
                         spawnApple(&apple);
                     }

                     handleApplepower(&pacman, &apple);

                     if(checkAppleCollision(&pacman, &apple)){
                         apple.isVisible = false;
                     }

                     if(checkMushroomCollision(&pacman, &mushroom)){
                         handleMushroompower(&pacman, &mushroom);
                         mushroom.isVisible = false;
                         mushroom.lastSpawnTime = SDL_GetTicks();
                     }


                     if (currentTime - mushroom.lastSpawnTime >= MUSHROOM_RESPAWN_INTERVAL && !mushroom.isVisible) {
                         spawnMushroom(&mushroom);
                     }


                     if (checkCherryCollision(&pacman, &cherry)) {
                         eatCherry(&pacman);
                         cherry.isVisible = false;
                         pacman.canEatGhost = true;
                     }

                     handleCherryEffect(&pacman, allGhosts, ghostCount);

                     if (currentTime - strawberry.lastSpawnTime >= STRAWBERRY_RESPAWN_INTERVAL && !strawberry.isVisible) {
                         spawnStrawberry(&strawberry);
                     }

                     Uint32 pacmanMoveDelay = isPacmanSpeedBoosted ? 100 : 200;

                     if (checkStrawberryCollision(&pacman, &strawberry)) {
                         strawberry.isVisible = false;
                         isPacmanSpeedBoosted = true;
                         speedBoostStartTime = currentTime;
                     }

                     if (isPacmanSpeedBoosted && currentTime - speedBoostStartTime >= PACMAN_SPEED_BOOST_DURATION) {
                         isPacmanSpeedBoosted = false;
                     }


                     if (currentTime - pacman.lastToggleTime >= pacmanMoveDelay) {
                         pacman.mouthOpen = !pacman.mouthOpen;
                         pacman.lastToggleTime = currentTime;
                     }


                     while (SDL_PollEvent(&event)) {
                         if (event.type == SDL_QUIT) {
                             running = false;
                             gameOver = true;
                         } else if (event.type == SDL_KEYDOWN) {
                             int newX = pacman.x;
                             int newY = pacman.y;
                             switch (event.key.keysym.sym) {
                                 case SDLK_UP:
                                     newY--;
                                     pacman.direction = DIRECTION_UP;
                                     break;
                                 case SDLK_DOWN:
                                     newY++;
                                     pacman.direction = DIRECTION_DOWN;
                                     break;
                                 case SDLK_LEFT:
                                     newX--;
                                     pacman.direction = DIRECTION_LEFT;
                                     break;
                                 case SDLK_RIGHT:
                                     newX++;
                                     pacman.direction = DIRECTION_RIGHT;
                                      break;
                            }
                         if (canMove(newX, newY)) {
                             pacman.x = newX;
                             pacman.y = newY;
                             if (food[newY][newX]) {
                                 food[newY][newX] = false;
                                 pacman.score += 5;
                                 if (!isFoodRemaining()) {
                                     generateRandomFood();
                                 }
                             }
                         }
                         }
                     }


                     moveGhost(&ghost1);
                     moveGhost(&ghost2);
                     moveGhost(&ghost3);
                     moveChasingGhost(&chasingGhost1, &pacman);
                     moveChasingGhost(&chasingGhost2, &pacman);
                     preventGhostCollision(allGhosts, ghostCount);
                     handlePacManGhostCollision(&pacman, allGhosts, ghostCount);
        


    
        
                     SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                     SDL_RenderClear(renderer);
                     for (int i = 0; i < ghostCount; i++) {
                         SDL_Rect ghostRect = { allGhosts[i]->x * (SCREEN_WIDTH / MAZE_WIDTH), allGhosts[i]->y * (SCREEN_HEIGHT / MAZE_HEIGHT), GHOST_SIZE, GHOST_SIZE };
                         SDL_RenderCopy(renderer, allGhosts[i]->texture, NULL, &ghostRect);
                     }
                     drawMaze(renderer);
                     drawFood(renderer);
                     drawGhost(renderer, &ghost1);
                     drawGhost(renderer, &ghost2);
                     drawGhost(renderer, &ghost3);
                     drawGhost(renderer, &chasingGhost1);
                     drawGhost(renderer, &chasingGhost2);
                     drawPacMan(renderer, &pacman, pacmanOpenTex, pacmanClosedTex);
                     drawCherry(renderer, &cherry);
                     drawStrawberry(renderer, &strawberry); 
                     drawApple(renderer, &apple); 
                     drawMushroom(renderer, &mushroom); 
                     displayUI(renderer, font, pacman.score, pacman.lives, playerName, durationtime);
                     SDL_RenderPresent(renderer);
                     SDL_Delay(16);

                     if (pacman.lives <= 0) {
                        gameOver = true;
                        saveScore(playerName, pacman.score);
                        bool returnToMenu = showGameOver(renderer, font, playerName, pacman.score);
                        if (returnToMenu) {
                          break; 
                        }
                    }
                }
                
            }
            case 1: 
                showScores(renderer, font);
                break;
            case 2: 
                running = false;
                break;
        }

        
    }

    SDL_DestroyTexture(pacmanOpenTex);
    SDL_DestroyTexture(pacmanClosedTex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_DestroyTexture(strawberryTex);
    SDL_DestroyTexture(cherryTex);
    SDL_DestroyTexture(appleTex);
    SDL_DestroyTexture(mushroomTex);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
