#include <iostream>
#include <list>
#include <vector>
#include <chrono>
#include <unordered_map>
#include <SDL.h>
#include <SDL_image.h>

using namespace std;

/**
 * To do:
 * Recalculate biggest integer scaling factor based on window size
 */

// Game coords
const int GAME_W = 320, GAME_H = 180;
const int TILE_W = 16, TILE_H = 16;
const int PLAYER_X = (GAME_W - TILE_W) / 2, PLAYER_Y = (GAME_H - TILE_H) / 2;

const int TILES_L = GAME_W / TILE_W / 2 + 2,
          TILES_R = GAME_W / TILE_W / 2 + 2,
          TILES_U = GAME_H / TILE_H / 2 + 2,
          TILES_D = GAME_H / TILE_H / 2 + 2;

// Window coords
const int SCALING_FACTOR = 6; // 1920 x 1080
const int SCREEN_W = GAME_W * SCALING_FACTOR, SCREEN_H = GAME_H * SCALING_FACTOR;

const double MAX_FPS = 240.0;
const auto WALK_TIME = chrono::duration_cast<std::chrono::nanoseconds>(chrono::milliseconds(300));

list<SDL_Texture *> g_textures;
SDL_Texture *LoadTexture(string path, SDL_Renderer *renderer)
{
  SDL_Texture *new_texture = IMG_LoadTexture(renderer, path.c_str());
  if (new_texture == NULL)
  {
    printf("Unable to create texture from %s! SDL Error: %s\n", path.c_str(), SDL_GetError());
  }
  else
  {
    g_textures.push_back(new_texture);
  }

  return new_texture;
}

enum Direction
{
  LEFT,
  RIGHT,
  UP,
  DOWN,
};

enum Tile
{
  G,
  W,
  M,
  H
};

// const vector<vector<Tile>> tiles = {
//   {W, G, G, H, G, G, W, W, W, G},
//   {W, H, H, H, G, G, W, W, G, G},
//   {W, H, M, M, G, G, W, G, G, G},
//   {W, H, M, H, G, G, W, G, G, G},
//   {W, H, H, H, G, G, W, G, M, M},
//   {W, G, H, G, G, W, W, G, M, G},
//   {G, G, G, G, G, W, W, G, M, G},
//   {G, G, G, H, W, W, G, G, G, G},
//   {W, G, H, H, W, G, G, G, G, W},
//   {W, W, H, W, W, G, G, G, W, W},
// };

/**
 * Given an item with game coordinates, draws it at the right
 * place on the screen
 */
void Draw(SDL_Renderer *renderer,
          SDL_Texture *texture,
          const SDL_Rect *srcrect,
          const SDL_Rect *dstrect,
          const SDL_RendererFlip flip = SDL_FLIP_NONE)
{
  const SDL_Rect scaledDstrect = {
    x : dstrect->x * SCALING_FACTOR,
    y : dstrect->y * SCALING_FACTOR,
    w : dstrect->w * SCALING_FACTOR,
    h : dstrect->h * SCALING_FACTOR,
  };
  SDL_RenderCopyEx(renderer, texture, srcrect, &scaledDstrect, 0, NULL, flip);
}

const int LETTER_W = 8, LETTER_H = 8;
const string FONT_CHARACTERS = " !',-.0123456789?ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz:";
const int FONT_ROWS = 7, FONT_COLUMNS = 10;
class TextRenderer
{
public:
  TextRenderer(SDL_Renderer *renderer, SDL_Texture *font) : renderer(renderer), font(font)
  {
    for (int col = 0; col < FONT_COLUMNS; col++)
    {
      for (int row = 0; row < FONT_ROWS; row++)
      {
        char c = FONT_CHARACTERS.at(row * FONT_COLUMNS + col);
        letterRects[c] = {x : col * LETTER_W, y : row * LETTER_H, w : LETTER_W, h : LETTER_H};
      }
    }
  };

  void DrawText(
      const string &text,
      SDL_Rect *textArea)
  {
    SDL_Rect dstRect;
    const int textAreaW = (textArea->w / LETTER_W) * LETTER_W,
              textAreaH = (textArea->h / LETTER_H) * LETTER_H;
    for (int pos = 0; pos < text.length(); ++pos)
    {
      int relativeY = ((pos * LETTER_W) / textAreaW) * LETTER_H;
      if (relativeY + LETTER_H > textArea->h)
      {
        break;
      }
      int relativeX = (pos * LETTER_W) % textAreaW;
      dstRect = {x : textArea->x + relativeX, y : textArea->y + relativeY, w : LETTER_W, h : LETTER_H};
      Draw(renderer, font, &letterRects[text.at(pos)], &dstRect);
    }
  }

private:
  SDL_Renderer *renderer;
  SDL_Texture *font;
  unordered_map<char, SDL_Rect> letterRects;
};

int main(int argc, char **argv)
{
  std::string exe_path = argv[0];
  std::string build_dir_path = exe_path.substr(0, exe_path.find_last_of("\\"));
  std::string project_dir_path = build_dir_path.substr(0, build_dir_path.find_last_of("\\"));

  // Setup
  SDL_Init(SDL_INIT_EVENTS);

  SDL_Window *window = SDL_CreateWindow("TBRPG", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_W, SCREEN_H, SDL_WINDOW_FULLSCREEN_DESKTOP);
  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  if (NULL == window || NULL == renderer)
  {
    return EXIT_FAILURE;
  }

  int keyboard_size;
  const Uint8 *keyboard_state = SDL_GetKeyboardState(&keyboard_size);

  SDL_Texture *characters = LoadTexture(project_dir_path + "/assets/characters.png", renderer);
  SDL_Rect wizardSprite = {x : 0, y : 0, w : TILE_W, h : TILE_H};
  int playerAnimIndex = 0;
  int playerAnimIndexOffset = 0;

  SDL_Texture *worldMap = LoadTexture(project_dir_path + "/assets/worldmap.png", renderer);
  SDL_Rect grassRect = {x : 0, y : 0, w : 16, h : 16};
  SDL_Rect waterRect = {x : 16, y : 0, w : 16, h : 16};
  SDL_Rect mountainRect = {x : 32, y : 0, w : 16, h : 16};
  SDL_Rect hillsRect = {x : 48, y : 0, w : 16, h : 16};

  SDL_Texture *font = LoadTexture(project_dir_path + "/assets/font.png", renderer);
  TextRenderer *textRenderer = new TextRenderer(renderer, font);
  SDL_Rect textRect;

  SDL_Event windowEvent;

  SDL_Rect playerPosition = {x : PLAYER_X, y : PLAYER_Y, w : TILE_W, h : TILE_H};

  vector<vector<Tile>> tiles;
  for (int y = 0; y < 100; y++)
  {
    tiles.emplace_back();
    for (int x = 0; x < 100; x++)
    {
      tiles.at(y).push_back((Tile)(rand() % 4));
    }
  }

  int playerPosX = 50, playerPosY = 50;

  bool isRunning = true;
  SDL_Event quit_event = {type : SDL_QUIT};

  int tileScreenLeft = PLAYER_X % TILE_W - TILE_W, tileScreenTop = PLAYER_Y % TILE_H - TILE_H;
  int tileDrawW = SCALING_FACTOR * TILE_W, tileDrawH = SCALING_FACTOR * TILE_H;
  SDL_Rect bgDrawRect = {x : 0, y : 0, w : TILE_W, h : TILE_H};

  auto frameLength = chrono::nanoseconds{(int)(1.0 / MAX_FPS * 1000.0 * 1000.0 * 1000.0)};
  auto currentTime = chrono::steady_clock::now() - frameLength;

  bool isWalking = false;
  auto walkStart = currentTime;
  double walkPercentDone;
  Direction walkDirection;
  Direction facing = DOWN;

  // Main loop
  while (isRunning)
  {
    // Handle inputs
    if (SDL_PollEvent(&windowEvent))
    {
      switch (windowEvent.type)
      {
      case SDL_QUIT:
        isRunning = false;
        break;
      case SDL_KEYDOWN:
        switch (windowEvent.key.keysym.scancode)
        {
        case (SDL_SCANCODE_ESCAPE):
          SDL_PushEvent(&quit_event);
          break;
        }
        break;
      }
    }

    if (isWalking && std::chrono::steady_clock::now() > walkStart + WALK_TIME)
    {
      isWalking = false;
      switch (walkDirection)
      {
      case LEFT:
        playerPosX--;
        break;
      case RIGHT:
        playerPosX++;
        break;
      case UP:
        playerPosY--;
        break;
      case DOWN:
        playerPosY++;
        break;
      }
    }

    if (!isWalking)
    {
      if (keyboard_state[SDL_SCANCODE_LEFT])
      {

        isWalking = true;
        walkStart = chrono::steady_clock::now();
        walkDirection = LEFT;
        facing = LEFT;
      }
      else if (keyboard_state[SDL_SCANCODE_RIGHT])
      {

        isWalking = true;
        walkStart = chrono::steady_clock::now();
        walkDirection = RIGHT;
        facing = RIGHT;
      }
      else if (keyboard_state[SDL_SCANCODE_UP])
      {

        isWalking = true;
        walkStart = chrono::steady_clock::now();
        walkDirection = UP;
        facing = UP;
      }
      else if (keyboard_state[SDL_SCANCODE_DOWN])
      {

        isWalking = true;
        walkStart = chrono::steady_clock::now();
        walkDirection = DOWN;
        facing = DOWN;
      }
    }

    while (std::chrono::steady_clock::now() > currentTime + frameLength)
    {
      currentTime += frameLength;

      // Render
      SDL_RenderClear(renderer);

      if (isWalking)
      {
        walkPercentDone = (double)(currentTime - walkStart).count() / (double)WALK_TIME.count();
        if (playerAnimIndex != (int)(walkPercentDone * 2 + 1) % 2)
        {
          playerAnimIndex = (int)(walkPercentDone * 2 + 1) % 2;
          if (playerAnimIndex == 0)
          {
            playerAnimIndexOffset = (playerAnimIndexOffset + 1) % 2;
          }
        }
      }
      else
      {
        walkPercentDone = 0;
        playerAnimIndex = 0;
      }

      for (int x = -TILES_L; x <= TILES_R; x++)
      {
        for (int y = -TILES_U; y <= TILES_D; y++)
        {
          bgDrawRect.x = x * TILE_W + playerPosition.x;
          bgDrawRect.y = y * TILE_H + playerPosition.y;

          if (isWalking)
          {
            int HORIZONTAL_ADJUST = (int)(TILE_W * walkPercentDone) + 1,
                VERTICAL_ADJUST = (int)(TILE_H * walkPercentDone) + 1;
            switch (walkDirection)
            {
            case LEFT:
              bgDrawRect.x += HORIZONTAL_ADJUST;
              break;
            case RIGHT:
              bgDrawRect.x -= HORIZONTAL_ADJUST;
              break;
            case UP:
              bgDrawRect.y += VERTICAL_ADJUST;
              break;
            case DOWN:
              bgDrawRect.y -= VERTICAL_ADJUST;
              break;
            }
          }

          int relativeX = playerPosX + x, relativeY = playerPosY + y;

          Tile i;
          if (relativeX >= 0 && relativeX < 100 && relativeY >= 0 && relativeY < 100)
          {
            i = tiles.at(relativeY).at(relativeX);
          }
          else
          {
            i = W;
          }
          switch (i)
          {
          case G:
            Draw(renderer, worldMap, &grassRect, &bgDrawRect);
            break;
          case W:
            Draw(renderer, worldMap, &waterRect, &bgDrawRect);
            break;
          case M:
            Draw(renderer, worldMap, &mountainRect, &bgDrawRect);
            break;
          case H:
            Draw(renderer, worldMap, &hillsRect, &bgDrawRect);
            break;
          }
        }
      }

      int facingOffset;
      SDL_RendererFlip flip = SDL_FLIP_NONE;
      if (facing == DOWN)
      {
        facingOffset = 0;
      }
      else if (facing == UP)
      {
        facingOffset = 128;
      }
      else
      {
        facingOffset = 64;
        if (facing == RIGHT)
        {
          flip = SDL_FLIP_HORIZONTAL;
        }
      }
      wizardSprite = {x : (playerAnimIndex + playerAnimIndexOffset * 2) * TILE_W + facingOffset, y : 0, w : TILE_W, h : TILE_H};
      Draw(renderer, characters, &wizardSprite, &playerPosition, flip);

      textRect = {x : 8, y : 8, w : 88, h : 72};
      textRenderer->DrawText("Welcome to WIZARD LAND", &textRect);
      textRect = {x: 8, y: 32, w: 100, h: 8};
      textRenderer->DrawText("Pop. 1", &textRect);

      SDL_RenderPresent(renderer);
    }
  }

  // Cleanup
  delete textRenderer;
  while (!g_textures.empty())
  {
    SDL_DestroyTexture(g_textures.back());
    g_textures.pop_back();
  }
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_QuitSubSystem(SDL_INIT_EVENTS);

  return EXIT_SUCCESS;
}
