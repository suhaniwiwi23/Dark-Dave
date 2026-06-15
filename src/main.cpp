#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>

// ============================================
// DARK DAVE — HAUNTED MANSION
// Level 1: The Foyer
// ============================================

const int SCREEN_WIDTH  = 800;
const int SCREEN_HEIGHT = 600;
const int TILE_SIZE     = 40;

// --- STRUCTS ---

struct Collectible {
    int row, col;
    SDL_Rect rect;
    char type;
};

struct Ghost {
    float x, y;
    float startY;
    float speed    = 1.2f;
    bool  movingRight = true;
    int   frame    = 0;
    int   frameTick = 0;
    int   width    = 32;
    int   height   = 32;

    void update() {
        x += movingRight ? speed : -speed;
        if (x > 680 || x < 50) movingRight = !movingRight;
        y = startY + sin(SDL_GetTicks() * 0.002) * 10.0f;

        frameTick++;
        if (frameTick >= 8) {
            frame = (frame + 1) % 9;
            frameTick = 0;
        }
    }

    SDL_Rect getRect() const {
        return { (int)x, (int)y, width, height };
    }

    // First row of ghost sheet = walk animation
    SDL_Rect getSrcRect() const {
        return { frame * 32, 0, 32, 32 };
    }
};

struct NPC {
    std::string name;
    std::vector<std::string> lines;
    SDL_Rect triggerZone;
    SDL_Rect drawRect;
    bool talked    = false;
    bool active    = false;
    int  lineIndex = 0;
};

// --- MAP LOADER ---

std::vector<std::string> loadMap(const std::string& path) {
    std::vector<std::string> map;
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open: " << path << "\n";
        return map;
    }
    std::string line;
    while (std::getline(file, line)) map.push_back(line);
    return map;
}

// --- TEXTURE LOADER ---

SDL_Texture* loadTexture(const std::string& path, SDL_Renderer* ren) {
    SDL_Surface* surf = IMG_Load(path.c_str());
    if (!surf) {
        std::cerr << "Failed to load: " << path << "\n";
        return nullptr;
    }
    // Convert to format that supports alpha
    SDL_Surface* converted = SDL_ConvertSurfaceFormat(
        surf, SDL_PIXELFORMAT_RGBA8888, 0);
    SDL_FreeSurface(surf);
    
    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, converted);
    SDL_FreeSurface(converted);
    
    if (!tex) {
        std::cerr << "Texture error: " << SDL_GetError() << "\n";
        return nullptr;
    }
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    return tex;
}

// --- RENDER TEXT ---

void renderText(SDL_Renderer* ren, TTF_Font* font,
                const std::string& text, int x, int y,
                SDL_Color color) {
    SDL_Surface* surf = TTF_RenderText_Solid(font, text.c_str(), color);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
    SDL_Rect dst = { x, y, surf->w, surf->h };
    SDL_RenderCopy(ren, tex, NULL, &dst);
    SDL_FreeSurface(surf);
    SDL_DestroyTexture(tex);
}

// --- DRAW DIALOGUE BOX ---

void drawDialogue(SDL_Renderer* ren, TTF_Font* font,
                  TTF_Font* smallFont,
                  const std::string& speaker,
                  const std::string& line) {
    // Dark box
    SDL_Rect box = { 20, 470, 760, 120 };
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren, 10, 5, 25, 220);
    SDL_RenderFillRect(ren, &box);

    // Purple border
    SDL_SetRenderDrawColor(ren, 130, 0, 200, 255);
    SDL_RenderDrawRect(ren, &box);

    // Inner border
    SDL_Rect inner = { 22, 472, 756, 116 };
    SDL_SetRenderDrawColor(ren, 80, 0, 120, 255);
    SDL_RenderDrawRect(ren, &inner);

    // Speaker name bar
    SDL_Rect nameBar = { 20, 450, 220, 28 };
    SDL_SetRenderDrawColor(ren, 60, 0, 100, 255);
    SDL_RenderFillRect(ren, &nameBar);
    SDL_SetRenderDrawColor(ren, 130, 0, 200, 255);
    SDL_RenderDrawRect(ren, &nameBar);

    // Speaker name
    SDL_Color nameColor = { 220, 150, 255, 255 };
    renderText(ren, font, speaker, 32, 453, nameColor);

    // Dialogue line
    SDL_Color lineColor = { 200, 200, 220, 255 };
    renderText(ren, smallFont, line, 35, 490, lineColor);

    // Press E prompt (blinking)
    if ((SDL_GetTicks() / 500) % 2 == 0) {
        SDL_Color promptColor = { 100, 80, 160, 255 };
        renderText(ren, smallFont, "[ E ] Continue", 630, 555, promptColor);
    }
}

// --- DRAW TILE ---

void drawTile(SDL_Renderer* ren,
              SDL_Texture* tilesTex,
              char tile, SDL_Rect dst) {

    // Source rects from dungeon_tiles.png
    // Wall top:  starts at 32,32  size ~80x90
    // Wall face: starts at 32,320
    // Floor:     starts at 286,32 size ~48x48
    // Torch:     starts at 194,130 size ~16x20

    switch (tile) {
        case 'R': {
            // Draw wall face (stone texture)
            SDL_Rect src = { 32, 320, 80, 80 };
            SDL_RenderCopy(ren, tilesTex, &src, &dst);
            break;
        }
        case 'P': {
            // Draw floor/cobblestone as platform
            SDL_Rect src = { 286, 32, 48, 48 };
            SDL_RenderCopy(ren, tilesTex, &src, &dst);
            break;
        }
        case 'F': {
            // Draw torch tile + purple overlay for ghost flame
            SDL_Rect src = { 194, 130, 16, 20 };
            SDL_RenderCopy(ren, tilesTex, &src, &dst);

            // Purple flame flicker on top
            int flicker = rand() % 8;
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(ren, 140, 0, 255, 120 + flicker * 10);
            SDL_RenderFillRect(ren, &dst);
            break;
        }
        case 'W': {
            // Tar pit — very dark with blue tint
            SDL_SetRenderDrawColor(ren, 15, 15, 30, 255);
            SDL_RenderFillRect(ren, &dst);

            // Bubble effect
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(ren, 40, 40, 80,
                (SDL_GetTicks() / 300) % 2 == 0 ? 180 : 60);
            SDL_Rect bubble1 = { dst.x + 5,  dst.y + 8,  8, 8 };
            SDL_Rect bubble2 = { dst.x + 22, dst.y + 14, 6, 6 };
            SDL_RenderFillRect(ren, &bubble1);
            SDL_RenderFillRect(ren, &bubble2);
            break;
        }
        case 'D': {
            // Cursed gem — bright purple diamond shape
            SDL_Rect gem = { dst.x + 10, dst.y + 8, 20, 24 };
            SDL_SetRenderDrawColor(ren, 180, 0, 255, 255);
            SDL_RenderFillRect(ren, &gem);

            // Shine
            SDL_SetRenderDrawColor(ren, 255, 200, 255, 255);
            SDL_Rect shine = { gem.x + 4, gem.y + 4, 6, 6 };
            SDL_RenderFillRect(ren, &shine);

            // Glow pulse
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            int pulse = (int)(sin(SDL_GetTicks() * 0.005) * 40 + 60);
            SDL_SetRenderDrawColor(ren, 180, 0, 255, pulse);
            SDL_Rect glow = { dst.x + 4, dst.y + 2, 32, 36 };
            SDL_RenderFillRect(ren, &glow);
            break;
        }
        case 'T': {
            // Haunted lantern — orange glow
            SDL_Rect lantern = { dst.x + 10, dst.y + 4, 20, 32 };
            SDL_SetRenderDrawColor(ren, 200, 120, 0, 255);
            SDL_RenderFillRect(ren, &lantern);

            // Glow center
            SDL_SetRenderDrawColor(ren, 255, 220, 100, 255);
            SDL_Rect glow = { lantern.x + 5, lantern.y + 8, 10, 12 };
            SDL_RenderFillRect(ren, &glow);

            // Pulse
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            int pulse = (int)(sin(SDL_GetTicks() * 0.004) * 50 + 80);
            SDL_SetRenderDrawColor(ren, 255, 140, 0, pulse);
            SDL_Rect halo = { dst.x, dst.y, TILE_SIZE, TILE_SIZE };
            SDL_RenderFillRect(ren, &halo);
            break;
        }
    }
}

// --- DRAW HUD ---

void drawHUD(SDL_Renderer* ren, TTF_Font* font,
             int score, int lives, int gems, int totalGems) {
    // HUD background bar
    SDL_Rect hudBg = { 0, 0, SCREEN_WIDTH, 40 };
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren, 10, 5, 25, 220);
    SDL_RenderFillRect(ren, &hudBg);

    // Bottom border line
    SDL_SetRenderDrawColor(ren, 100, 0, 180, 255);
    SDL_RenderDrawLine(ren, 0, 40, SCREEN_WIDTH, 40);

    // Score
    SDL_Color white = { 200, 180, 255, 255 };
    renderText(ren, font,
        "SCORE: " + std::to_string(score), 10, 8, white);

    // Gems collected
    SDL_Color gemCol = { 200, 100, 255, 255 };
    renderText(ren, font,
        "GEMS: " + std::to_string(gems) + "/" +
        std::to_string(totalGems), 280, 8, gemCol);

    // Level name
    SDL_Color lvlCol = { 150, 130, 200, 255 };
    renderText(ren, font, "THE FOYER", 430, 8, lvlCol);

    // Lives as red skulls (rects)
    for (int i = 0; i < lives; i++) {
        SDL_SetRenderDrawColor(ren, 180, 0, 0, 255);
        SDL_Rect skull = { SCREEN_WIDTH - 30 - i * 28, 8, 18, 22 };
        SDL_RenderFillRect(ren, &skull);
        SDL_SetRenderDrawColor(ren, 255, 50, 50, 255);
        SDL_Rect eye1 = { skull.x + 2, skull.y + 5, 4, 4 };
        SDL_Rect eye2 = { skull.x + 10, skull.y + 5, 4, 4 };
        SDL_RenderFillRect(ren, &eye1);
        SDL_RenderFillRect(ren, &eye2);
    }
}

// ============================================
// MAIN
// ============================================

int main(int argc, char* argv[]) {
    srand(time(nullptr));

    // Init SDL systems
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL Error: " << SDL_GetError() << "\n";
        return 1;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "IMG Error: " << IMG_GetError() << "\n";
        return 1;
    }
    if (TTF_Init() != 0) {
        std::cerr << "TTF Error: " << TTF_GetError() << "\n";
        return 1;
    }

    SDL_Window* win = SDL_CreateWindow(
        "Dark Dave — The Haunted Mansion",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

    // --- LOAD ASSETS ---
    SDL_Texture* texBg     = loadTexture("assets/background.png",   ren);
    SDL_Texture* texTiles  = loadTexture("assets/dungeon_tiles.png", ren);
    SDL_Texture* texGhost  = loadTexture("assets/ghostIce_all.png",  ren);
    // ADD these:
    SDL_Texture* texDaveStand = loadTexture("assets/dave/stand.png", ren);
    SDL_Texture* texDaveRun1  = loadTexture("assets/dave/run1.png",  ren);
    SDL_Texture* texDaveRun2  = loadTexture("assets/dave/run2.png",  ren);
    SDL_Texture* texDaveJump  = loadTexture("assets/dave/jump.png",  ren);
    SDL_Texture* texNPC  = loadTexture("assets/npc.png", ren);
    SDL_SetTextureBlendMode(texDaveStand, SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(texDaveRun1, SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(texDaveRun2, SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(texDaveJump, SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(texNPC, SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(texGhost, SDL_BLENDMODE_BLEND);

    TTF_Font* fontMain  = TTF_OpenFont("assets/font.ttf", 20);
    TTF_Font* fontSmall = TTF_OpenFont("assets/font.ttf", 16);

    if (!texBg || !texTiles || !texGhost ||
        !texDaveStand || !texDaveRun1 ||
        !texDaveRun2  || !texDaveJump ||
        !texNPC || !fontMain || !fontSmall) {
        std::cerr << "Failed to load textures or fonts!\n";
        std::cerr << "Copy DLLs check: SDL2_image.dll and SDL2_ttf.dll\n";
        std::cerr << "must be in DangerousDave/ folder!\n";
        return 1;
    }

    // --- LOAD MAP ---
    std::vector<std::string> levelMap = loadMap("level1.txt");
        if (levelMap.empty()) {
        std::cerr << "Map failed!\n";
        return 1;
    }

    // --- PLAYER ---
    float daveX = 80, daveY = 100;
    float yVelocity   = 0;
    const float GRAV  = 0.5f;
    const float JUMP  = -12.0f;
    const int   SPD   = 4;
    const int   DW    = 40;
    const int   DH    = 55;
    bool isJumping    = false;
    bool daveMoving      = false;       // ← ADD
    bool daveFacingRight = true;        // ← ADD
    int  daveFrame       = 0;           // ← ADD
    int  daveFrameTick   = 0;   
    int  score        = 0;
    int  lives        = 3;
    int  gemsCollected = 0;
    int  totalGems    = 0;

    for (auto& row : levelMap)
        for (char c : row)
            if (c == 'D') totalGems++;

    // --- GHOSTS ---
    std::vector<Ghost> ghosts = {
        { 300, 280, 280 },
        { 520, 180, 180 }
    };

    // --- NPC ---
    NPC butler;
    butler.name       = "Eddy the Butler";
    butler.drawRect   = { 180, 490, 28, 46 };
    butler.triggerZone = { 130, 460, 110, 80 };
    butler.lines      = {
       "Welcome to Blackwood Mansion, young one.",
        "I am Edmund, keeper of these cursed halls.",
        "My master disappeared 100 years ago tonight.",
        "Collect the gems and yellow trophy to unlock the next floor.",
        "But beware... you are not alone in here."
    };

    // --- DEATH MESSAGES ---
    std::vector<std::string> deathMsgs = {
        "The mansion claims another soul...",
        "The shadows swallowed you whole.",
        "The tar remembers everyone it takes.",
        "The ghost was faster than it looked.",
        "You heard something behind you. Too late."
    };

    // --- STATE ---
    bool running       = true;
    bool dialogueActive = false;
    bool levelComplete = false;
    SDL_Event e;

    // ============================================
    // GAME LOOP
    // ============================================
    while (running) {

        // --- EVENTS ---
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE: running = false; break;
                    case SDLK_e:
                        if (dialogueActive) {
                            butler.lineIndex++;
                            if (butler.lineIndex >=
                                (int)butler.lines.size()) {
                                dialogueActive   = false;
                                butler.active    = false;
                                butler.talked    = true;
                                butler.lineIndex = 0;
                            }
                        }
                        break;
                }
            }
        }

        // --- INPUT ---
        
        // --- INPUT ---
    if (!dialogueActive) {
        const Uint8* keys = SDL_GetKeyboardState(NULL);
    
        bool leftHeld  = keys[SDL_SCANCODE_LEFT]  || keys[SDL_SCANCODE_A];
        bool rightHeld = keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D];
        bool jumpHeld  = keys[SDL_SCANCODE_UP]    || keys[SDL_SCANCODE_W] || 
                     keys[SDL_SCANCODE_SPACE];

        if (leftHeld)  { daveX -= SPD; daveFacingRight = false; }
        if (rightHeld) { daveX += SPD; daveFacingRight = true;  }
        if (jumpHeld && !isJumping) {
        yVelocity = JUMP;
        isJumping = true;
    }

        // Set moving based on keys directly
        daveMoving = leftHeld || rightHeld;
}
        

        // --- BOUNDARIES ---
        if (daveX < 0) daveX = 0;
        if (daveX + DW > SCREEN_WIDTH) daveX = SCREEN_WIDTH - DW;

        if (daveY > SCREEN_HEIGHT) {
            daveX = 80; daveY = 100; yVelocity = 0;
            lives--;
            std::cout << deathMsgs[rand() % deathMsgs.size()] << "\n";
        }

        // --- BUILD TILE LISTS ---
        std::vector<SDL_Rect> solidTiles, hazardTiles;
        std::vector<Collectible> items;

        for (int row = 0; row < (int)levelMap.size(); ++row) {
            for (int col = 0; col < (int)levelMap[row].size(); ++col) {
                SDL_Rect r = { col*TILE_SIZE, row*TILE_SIZE,
                               TILE_SIZE, TILE_SIZE };
                char t = levelMap[row][col];
                if (t=='R' || t=='P') solidTiles.push_back(r);
                else if (t=='F' || t=='W') hazardTiles.push_back(r);
                else if (t=='D') {
                    SDL_Rect gr = { r.x+10, r.y+8, 20, 24 };
                    items.push_back({ row, col, gr, 'D' });
                }
                else if (t=='T') {
                    SDL_Rect lr = { r.x+10, r.y+4, 20, 32 };
                    items.push_back({ row, col, lr, 'T' });
                }
            }
        }

        // --- PHYSICS ---
        yVelocity += GRAV;
        daveY     += yVelocity;
        SDL_Rect daveRect = { (int)daveX, (int)daveY, DW, DH };
        isJumping = true;

        for (const auto& tile : solidTiles) {
            if (SDL_HasIntersection(&daveRect, &tile)) {
                if (yVelocity >= 0 &&
                    (daveRect.y + daveRect.h) - yVelocity <= tile.y + 12) {
                    daveY     = tile.y - DH;
                    yVelocity = 0;
                    isJumping = false;
                    break;
                }
            }
        }

        // --- HAZARDS ---
        daveRect.y = (int)daveY;
        for (const auto& h : hazardTiles) {
            if (SDL_HasIntersection(&daveRect, &h)) {
                daveX = 80; daveY = 100; yVelocity = 0;
                lives--;
                std::cout << deathMsgs[rand() % deathMsgs.size()] << "\n";
                break;
            }
        }

        // --- COLLECTIBLES ---
        for (auto& item : items) {
            if (SDL_HasIntersection(&daveRect, &item.rect)) {
                if (item.type == 'D') {
                    score += 100;
                    gemsCollected++;
                } else if (item.type == 'T') {
                    levelComplete = true;
                }
                levelMap[item.row][item.col] = '.';
                break;
            }
        }

        // --- UPDATE GHOSTS ---
        for (auto& g : ghosts) {
            g.update();
            SDL_Rect gr = g.getRect();
            if (SDL_HasIntersection(&daveRect, &gr)) {
                daveX = 80; daveY = 100;
                yVelocity = 0; lives--;
                std::cout << "The ghost was faster than it looked.\n";
            }
        }

        // --- NPC TRIGGER ---
        if (!butler.talked && !dialogueActive) {
            if (SDL_HasIntersection(&daveRect, &butler.triggerZone)) {
                dialogueActive = true;
                butler.active  = true;
            }
        }

        // ============================================
        // RENDER
        // ============================================

        // Background image
        SDL_RenderCopy(ren, texBg, NULL, NULL);

        // Atmospheric dark overlay
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ren, 5, 0, 15,
            120 + rand() % 6);  // subtle flicker
        SDL_Rect overlay = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
        SDL_RenderFillRect(ren, &overlay);

        // --- DRAW TILES ---
        for (int row = 0; row < (int)levelMap.size(); ++row) {
            for (int col = 0; col < (int)levelMap[row].size(); ++col) {
                char tile = levelMap[row][col];
                if (tile == '.') continue;
                SDL_Rect dst = { col*TILE_SIZE, row*TILE_SIZE,
                                 TILE_SIZE, TILE_SIZE };
                drawTile(ren, texTiles, tile, dst);
            }
        }

        // --- DRAW GHOSTS ---
        for (const auto& g : ghosts) {
            SDL_Rect src = g.getSrcRect();
            SDL_Rect dst = {
                (int)g.x - 4, (int)g.y - 4,
                g.width + 8,  g.height + 8
            };
            // Ghost glow
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(ren, 100, 150, 255, 40);
            SDL_RenderFillRect(ren, &dst);
            // Ghost sprite
            SDL_RenderCopy(ren, texGhost, &src, &dst);
        }

        // --- DRAW NPC ---
        // --- DRAW NPC ---
        if (!butler.talked) {
            // Draw butler sprite
            SDL_Rect npcDst = {
            butler.drawRect.x - 10,
            butler.drawRect.y - 20,  // less negative = lower on screen
            60,
            90
    };
        SDL_RenderCopy(ren, texNPC, NULL, &npcDst);

        // Blinking E prompt above NPC
        if ((SDL_GetTicks() / 600) % 2 == 0) {
            SDL_Color talkCol = { 200, 150, 255, 255 };
            renderText(ren, fontSmall, "[ E ]",
            butler.drawRect.x - 5,
            butler.drawRect.y - 80, talkCol);
    }
}
        

        

   // --- DRAW DAVE ---

// --- DRAW DAVE ---
{
    SDL_Texture* frameTex = texDaveStand;

    if (yVelocity < -1.0f || yVelocity > 2.0f) {
        // Actually airborne
        frameTex = texDaveJump;
    } else if (daveMoving) {
        frameTex = texDaveRun1;
    } else {
        frameTex = texDaveStand;
    }

    SDL_Rect dst = { (int)daveX, (int)daveY, DW, DH };
    SDL_RendererFlip flip = daveFacingRight ?
        SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
    SDL_RenderCopyEx(ren, frameTex, NULL, &dst, 0, NULL, flip);
}




        // --- DRAW HUD ---
        drawHUD(ren, fontMain, score, lives,
                gemsCollected, totalGems);

        // --- LEVEL COMPLETE ---
        if (levelComplete) {
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(ren, 10, 5, 30, 200);
            SDL_Rect banner = { 100, 200, 600, 120 };
            SDL_RenderFillRect(ren, &banner);
            SDL_SetRenderDrawColor(ren, 150, 0, 255, 255);
            SDL_RenderDrawRect(ren, &banner);

            SDL_Color gold = { 255, 180, 0, 255 };
            SDL_Color grey = { 180, 160, 220, 255 };
            renderText(ren, fontMain,
                "THE DOOR CREAKS OPEN...", 180, 230, gold);
            renderText(ren, fontSmall,
                "You found the haunted lantern.",
                220, 270, grey);
            renderText(ren, fontSmall,
                "Press ESC to proceed.",
                270, 300, grey);
        }

        // --- DRAW DIALOGUE ---
        if (dialogueActive &&
            butler.lineIndex < (int)butler.lines.size()) {
            drawDialogue(ren, fontMain, fontSmall,
                         butler.name,
                         butler.lines[butler.lineIndex]);
        }

        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }

    // --- CLEANUP ---
    TTF_CloseFont(fontMain);
    TTF_CloseFont(fontSmall);
    SDL_DestroyTexture(texBg);
    SDL_DestroyTexture(texTiles);
    SDL_DestroyTexture(texGhost);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    // ADD:
    SDL_DestroyTexture(texDaveStand);
    SDL_DestroyTexture(texDaveRun1);
    SDL_DestroyTexture(texDaveRun2);
    SDL_DestroyTexture(texDaveJump);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}