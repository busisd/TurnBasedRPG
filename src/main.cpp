#include <iostream>
#include <list>
#include <vector>
#include <SDL.h>
#include <SDL_image.h>

using namespace std;

/**
 * To do:
 * Recalculate biggest integer scaling factor based on window size
 * Render in its own loop with max fps
 * Animated/"smooth" walking
 * Repeat walking without hitting key again
 */

// Game coords
const int GAME_W = 320, GAME_H = 180;
const int TILE_W = 16, TILE_H = 16;
const int TILES_L = GAME_W / TILE_W / 2 + 1,
          TILES_R = GAME_W / TILE_W / 2 + 1,
          TILES_U = GAME_H / TILE_H / 2 + 1,
          TILES_D = GAME_H / TILE_H / 2 + 1;
const int PLAYER_X = (GAME_W - TILE_W) / 2, PLAYER_Y = (GAME_H - TILE_H) / 2;

// Window coords
const int SCALING_FACTOR = 6; // 1920 x 1080
const int SCREEN_W = GAME_W * SCALING_FACTOR, SCREEN_H = GAME_H * SCALING_FACTOR;

list<SDL_Texture *> g_textures;
SDL_Texture *LoadTexture(std::string path, SDL_Renderer *renderer)
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

  SDL_Texture *characters = LoadTexture(project_dir_path + "/assets/characters.png", renderer);
  SDL_Rect wizardSprite = {x : 0, y : 0, w : 16, h : 16};

  SDL_Texture *worldMap = LoadTexture(project_dir_path + "/assets/worldmap.png", renderer);
  SDL_Rect grassRect = {x : 0, y : 0, w : 16, h : 16};
  SDL_Rect riverRect = {x : 16, y : 0, w : 16, h : 16};
  SDL_Rect mountainRect = {x : 32, y : 0, w : 16, h : 16};
  SDL_Rect hillsRect = {x : 48, y : 0, w : 16, h : 16};

  SDL_Event windowEvent;

  SDL_Rect playerRect = {x : PLAYER_X * SCALING_FACTOR, y : PLAYER_Y * SCALING_FACTOR, w : TILE_W * SCALING_FACTOR, h : TILE_H * SCALING_FACTOR};

  cout << TILES_L << " " << TILES_R << endl;
  cout << TILES_U << " " << TILES_D << endl;

  vector<vector<int>> tiles = vector<vector<int>>();
  for (int x = 0; x < 100; x++)
  {
    tiles.emplace_back();
    for (int y = 0; y < 100; y++)
    {
      tiles.at(x).push_back(rand() % 4);
    }
  }

  int playerPosX = 50, playerPosY = 50;

  bool isRunning = true;
  SDL_Event quit_event = {type : SDL_QUIT};

  int tileScreenLeft = PLAYER_X % TILE_W - TILE_W, tileScreenTop = PLAYER_Y % TILE_H - TILE_H;
  int tileDrawW = SCALING_FACTOR * TILE_W, tileDrawH = SCALING_FACTOR * TILE_H;
  SDL_Rect bgDrawRect;

  // Main loop
  while (isRunning)
  {
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
        case (SDL_SCANCODE_LEFT):
          if (windowEvent.key.repeat == 0)
          {
            playerPosX--;
          }
          break;
        case (SDL_SCANCODE_RIGHT):
          if (windowEvent.key.repeat == 0)
          {
            playerPosX++;
          }
          break;
        case (SDL_SCANCODE_UP):
          if (windowEvent.key.repeat == 0)
          {
            playerPosY--;
          }
          break;
        case (SDL_SCANCODE_DOWN):
          if (windowEvent.key.repeat == 0)
          {
            playerPosY++;
          }
          break;
        }
        break;
      }
    }

    // Render
    SDL_RenderClear(renderer);
    for (int x = -TILES_L; x <= TILES_R; x++)
    {
      for (int y = -TILES_U; y <= TILES_D; y++)
      {
        bgDrawRect = {x : x * tileDrawW + playerRect.x, y : y * tileDrawH + playerRect.y, w : tileDrawW, h : tileDrawH};

        int relativeX = playerPosX + x, relativeY = playerPosY + y;

        int i;
        if (relativeX >= 0 && relativeX < 100 && relativeY >= 0 && relativeY < 100)
        {
          i = tiles.at(relativeX).at(relativeY);
        }
        else
        {
          i = 0;
        }
        switch (i)
        {
        case 0:
          SDL_RenderCopy(renderer, worldMap, &grassRect, &bgDrawRect);
          break;
        case 1:
          SDL_RenderCopy(renderer, worldMap, &riverRect, &bgDrawRect);
          break;
        case 2:
          SDL_RenderCopy(renderer, worldMap, &mountainRect, &bgDrawRect);
          break;
        case 3:
          SDL_RenderCopy(renderer, worldMap, &hillsRect, &bgDrawRect);
          break;
        }
      }
    }
    SDL_RenderCopy(renderer, characters, &wizardSprite, &playerRect);
    SDL_RenderPresent(renderer);
  }

  // Cleanup
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
