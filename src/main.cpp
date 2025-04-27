#include <iostream>
#include <list>
#include <queue>
#include <vector>
#include <chrono>
#include <unordered_map>
#include <SDL.h>
#include <SDL_image.h>
#include <boost/json/parse.hpp>
#include <boost/json/value.hpp>
#include "./text_renderer.cpp"

using namespace std;

/**
 * To do:
 * Recalculate biggest integer scaling factor based on window size
 * Find a better way to get relative path to assets
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
const int WALK_FRAMES = 72;

list<SDL_Texture *> g_textures;
SDL_Texture *LoadTexture(string path, SDL_Renderer *renderer)
{
  SDL_Texture *newTexture = IMG_LoadTexture(renderer, path.c_str());
  if (newTexture == NULL)
  {
    printf("Unable to create texture from %s! SDL Error: %s\n", path.c_str(), SDL_GetError());
  }
  else
  {
    g_textures.push_back(newTexture);
  }

  return newTexture;
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

const int GUI_Y = 24;
SDL_Rect borderVertical = {x : 0, y : GUI_Y, w : 5, h : 1};
SDL_Rect borderHorizontal = {x : 8, y : GUI_Y, w : 1, h : 5};
SDL_Rect cornerTL = {x : 16, y : GUI_Y, w : 5, h : 5};
SDL_Rect cornerTR = {x : 24, y : GUI_Y, w : 5, h : 5};
SDL_Rect cornerBR = {x : 32, y : GUI_Y, w : 5, h : 5};
SDL_Rect cornerBL = {x : 40, y : GUI_Y, w : 5, h : 5};
SDL_Rect junctionR = {x : 48, y : GUI_Y, w : 5, h : 5};
SDL_Rect junctionB = {x : 56, y : GUI_Y, w : 5, h : 5};
SDL_Rect junctionL = {x : 64, y : GUI_Y, w : 5, h : 5};
SDL_Rect junctionT = {x : 72, y : GUI_Y, w : 5, h : 5};
SDL_Rect guiFill = {x : 80, y : GUI_Y, w : 1, h : 1};
const int GUI_BORDER_W = 5, GUI_BORDER_H = 5;

void DrawGuiLineH(SDL_Renderer *renderer, SDL_Texture *gui, SDL_Rect *lineRect, SDL_Rect *endpointL = NULL, SDL_Rect *endpointR = NULL)
{
  SDL_SetTextureColorMod(gui, 255, 255, 255);
  SDL_Rect guiRect;
  int marginX = (endpointL != NULL) * GUI_BORDER_W;
  int marginW = (endpointR != NULL) * GUI_BORDER_W + marginX;
  guiRect = {x : lineRect->x + marginX, y : lineRect->y, w : lineRect->w - marginW, h : GUI_BORDER_H};
  SDL_RenderCopy(renderer, gui, &borderHorizontal, &guiRect);

  if (endpointL != NULL)
  {
    guiRect = {x : lineRect->x, y : lineRect->y, w : GUI_BORDER_W, h : GUI_BORDER_H};
    SDL_RenderCopy(renderer, gui, endpointL, &guiRect);
  }
  if (endpointR != NULL)
  {
    guiRect = {x : lineRect->x + lineRect->w - GUI_BORDER_W, y : lineRect->y, w : GUI_BORDER_W, h : GUI_BORDER_H};
    SDL_RenderCopy(renderer, gui, endpointR, &guiRect);
  }
}

void DrawGuiLineV(SDL_Renderer *renderer, SDL_Texture *gui, SDL_Rect *lineRect, SDL_Rect *endpointT = NULL, SDL_Rect *endpointB = NULL)
{
  SDL_SetTextureColorMod(gui, 255, 255, 255);
  SDL_Rect guiRect;
  int marginY = (endpointT != NULL) * GUI_BORDER_W;
  int marginH = (endpointB != NULL) * GUI_BORDER_W + marginY;
  guiRect = {x : lineRect->x, y : lineRect->y + marginY, w : GUI_BORDER_W, h : lineRect->h - marginH};
  SDL_RenderCopy(renderer, gui, &borderVertical, &guiRect);

  if (endpointT != NULL)
  {
    guiRect = {x : lineRect->x, y : lineRect->y, w : GUI_BORDER_W, h : GUI_BORDER_H};
    SDL_RenderCopy(renderer, gui, endpointT, &guiRect);
  }
  if (endpointB != NULL)
  {
    guiRect = {x : lineRect->x, y : lineRect->y + lineRect->h - GUI_BORDER_H, w : GUI_BORDER_W, h : GUI_BORDER_H};
    SDL_RenderCopy(renderer, gui, endpointB, &guiRect);
  }
}

void DrawGuiBox(SDL_Renderer *renderer, SDL_Texture *gui, SDL_Rect *boxRect, bool fill = true, int r = 0, int g = 0, int b = 0)
{
  SDL_SetTextureColorMod(gui, 255, 255, 255);
  SDL_Rect guiRect;
  guiRect = {x : boxRect->x + GUI_BORDER_W, y : boxRect->y, w : boxRect->w - GUI_BORDER_W * 2, h : GUI_BORDER_H};
  DrawGuiLineH(renderer, gui, &guiRect);
  guiRect = {x : boxRect->x + GUI_BORDER_W, y : boxRect->y + boxRect->h - GUI_BORDER_H, w : boxRect->w - GUI_BORDER_W * 2, h : GUI_BORDER_H};
  DrawGuiLineH(renderer, gui, &guiRect);

  guiRect = {x : boxRect->x, y : boxRect->y, w : GUI_BORDER_W, h : boxRect->h};
  DrawGuiLineV(renderer, gui, &guiRect, &cornerTL, &cornerBL);
  guiRect = {x : boxRect->x + boxRect->w - GUI_BORDER_W, y : boxRect->y, w : GUI_BORDER_W, h : boxRect->h};
  DrawGuiLineV(renderer, gui, &guiRect, &cornerTR, &cornerBR);

  if (fill)
  {
    SDL_SetTextureColorMod(gui, r, g, b);
    guiRect = {x : boxRect->x + GUI_BORDER_W, y : boxRect->y + GUI_BORDER_H, w : boxRect->w - GUI_BORDER_W * 2, h : boxRect->h - GUI_BORDER_H * 2};
    SDL_RenderCopy(renderer, gui, &guiFill, &guiRect);
  }
}

void DrawTextBox(TextRenderer *textRenderer, const string &text, SDL_Renderer *renderer, SDL_Texture *gui, SDL_Rect *textArea, int r = 0, int g = 0, int b = 0, int charsToRender = -1)
{
  SDL_Rect borderRect = {x : textArea->x - GUI_BORDER_W - 1, y : textArea->y - GUI_BORDER_H - 1, w : textArea->w + GUI_BORDER_W * 2 + 2, h : textArea->h + GUI_BORDER_H * 2 + 2};
  DrawGuiBox(renderer, gui, &borderRect, true, r, g, b);
  textRenderer->DrawTextWrapped(text, textArea, charsToRender);
}

// srcRect
const SDL_Rect battleAttack = {x : 0, y : 0, w : 48, h : 7};
const SDL_Rect battleMagic = {x : 0, y : 7, w : 48, h : 7};
const SDL_Rect battleItem = {x : 0, y : 14, w : 48, h : 7};
const SDL_Rect battleRun = {x : 0, y : 21, w : 48, h : 7};
const SDL_Rect battleSelect = {x : 0, y : 28, w : 4, h : 5};
const SDL_Rect battleHighlightTL = {x : 0, y : 33, w : 3, h : 3};
const SDL_Rect battleHighlightTR = {x : 3, y : 33, w : 3, h : 3};
const SDL_Rect battleHighlightBR = {x : 6, y : 33, w : 3, h : 3};
const SDL_Rect battleHighlightBL = {x : 9, y : 33, w : 3, h : 3};

const SDL_Rect battleBGPlains = {x : 0, y : 0, w : 138, h : 26};

const SDL_Rect enemyClamhead1 = {x : 0, y : 0, w : 24, h : 32};
const SDL_Rect enemyClamhead2 = {x : 24, y : 0, w : 24, h : 32};
const SDL_Rect enemyGoblin1 = {x : 0, y : 32, w : 24, h : 32};
const SDL_Rect enemyGoblin2 = {x : 24, y : 32, w : 24, h : 32};
const SDL_Rect enemyRat1 = {x : 0, y : 64, w : 24, h : 14};
const SDL_Rect enemyRat2 = {x : 24, y : 64, w : 24, h : 14};

// dstRect
const SDL_Rect descriptionBoxPos = {x : 6, y : 110, w : 160, h : 64};
const SDL_Rect battleAttackPos = {x : 182, y : 113, w : 48, h : 7};
const SDL_Rect battleMagicPos = {x : 182, y : 124, w : 48, h : 7};
const SDL_Rect battleItemPos = {x : 182, y : 135, w : 48, h : 7};
const SDL_Rect battleRunPos = {x : 182, y : 146, w : 48, h : 7};
const SDL_Rect battleBGPos = {x : 5, y : 5, w : 138, h : 26};

const SDL_Rect enemySlot0 = {x : 116, y : 34, w : 24, h : 32};
const SDL_Rect enemySlot1 = {x : 116, y : 69, w : 24, h : 32};
const SDL_Rect enemySlot2 = {x : 89, y : 34, w : 24, h : 32};
const SDL_Rect enemySlot3 = {x : 89, y : 69, w : 24, h : 32};
const SDL_Rect enemySlot4 = {x : 62, y : 34, w : 24, h : 14};
const SDL_Rect enemySlot5 = {x : 62, y : 51, w : 24, h : 14};
const SDL_Rect enemySlot6 = {x : 62, y : 69, w : 24, h : 14};
const SDL_Rect enemySlot7 = {x : 62, y : 86, w : 24, h : 14};

void SetEnemySlot(int slotIndex, SDL_Rect &rect)
{
  switch (slotIndex)
  {
  case 0:
    rect = enemySlot0;
    break;
  case 1:
    rect = enemySlot1;
    break;
  case 2:
    rect = enemySlot2;
    break;
  case 3:
    rect = enemySlot3;
    break;
  case 4:
    rect = enemySlot4;
    break;
  case 5:
    rect = enemySlot5;
    break;
  case 6:
    rect = enemySlot6;
    break;
  case 7:
    rect = enemySlot7;
    break;
  }
}

SDL_Rect highlightRect;
void HighlightSlot(SDL_Renderer *renderer, SDL_Texture *battle, const SDL_Rect *slotRect)
{
  highlightRect = {x : slotRect->x - 1, y : slotRect->y - 1, w : 3, h : 3};
  SDL_RenderCopy(renderer, battle, &battleHighlightTL, &highlightRect);
  highlightRect = {x : slotRect->x + slotRect->w + 1 - 3, y : slotRect->y - 1, w : 3, h : 3};
  SDL_RenderCopy(renderer, battle, &battleHighlightTR, &highlightRect);
  highlightRect = {x : slotRect->x - 1, y : slotRect->y + slotRect->h + 1 - 3, w : 3, h : 3};
  SDL_RenderCopy(renderer, battle, &battleHighlightBL, &highlightRect);
  highlightRect = {x : slotRect->x + slotRect->w + 1 - 3, y : slotRect->y + slotRect->h + 1 - 3, w : 3, h : 3};
  SDL_RenderCopy(renderer, battle, &battleHighlightBR, &highlightRect);
}

enum class GameScreen
{
  Map,
  Battle
};

enum class BattleStep
{
  Action,
  Target,
  Result
};

enum class BattleAction
{
  Attack = 0,
  Magic = 1,
  Item = 2,
  Run = 3
};

bool isPositive(int num)
{
  return num > 0;
}

int main(int argc, char **argv)
{
  cout << TEMP_IMPORT << endl;
  string exe_path = argv[0];
  string build_dir_path = exe_path.substr(0, exe_path.find_last_of("\\"));
  string project_dir_path = build_dir_path.substr(0, build_dir_path.find_last_of("\\"));

  // Setup
  SDL_Init(SDL_INIT_EVENTS);

  SDL_Window *window = SDL_CreateWindow("TBRPG", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_W, SCREEN_H, SDL_WINDOW_FULLSCREEN_DESKTOP);
  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
  SDL_RenderSetScale(renderer, SCALING_FACTOR, SCALING_FACTOR);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  if (NULL == window || NULL == renderer)
  {
    return EXIT_FAILURE;
  }

  int keyboardSize;
  const Uint8 *keyboardState = SDL_GetKeyboardState(&keyboardSize);
  Uint8 newlyPressedKeys[keyboardSize];
  fill(newlyPressedKeys, newlyPressedKeys + keyboardSize, 0);

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

  SDL_Texture *gui = LoadTexture(project_dir_path + "/assets/gui.png", renderer);
  SDL_Texture *battle = LoadTexture(project_dir_path + "/assets/battle.png", renderer);
  SDL_Texture *battleBGs = LoadTexture(project_dir_path + "/assets/battleBGs.png", renderer);
  SDL_Texture *enemies = LoadTexture(project_dir_path + "/assets/enemies.png", renderer);
  SDL_SetTextureColorMod(battle, 230, 230, 230);
  SDL_Rect guiRect;

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
  SDL_Event quitEvent = {type : SDL_QUIT};

  SDL_Rect bgDrawRect = {x : 0, y : 0, w : TILE_W, h : TILE_H};

  auto frameLength = chrono::nanoseconds{(int)(1.0 / MAX_FPS * 1000.0 * 1000.0 * 1000.0)};
  auto currentTime = chrono::steady_clock::now() - frameLength;
  unsigned long long int frameCount = 0;

  bool isWalking = false;
  unsigned long long int walkStart = frameCount;
  double walkPercentDone;
  Direction walkDirection;
  Direction facing = DOWN;

  bool showText = false;
  int textCharsToShow = 0;
  string bottomText = string("This is some text in a text box! Go forth, wizard, and cast spells! Huzzah! You will win!\n") +
                      string("Furthermore, you may even get to ponder an orb at some point!");

  int battleCharsToShow = 0;
  int enemyHealth = 10;
  int damageDealt = 0;
  BattleStep battleStep = BattleStep::Action;
  BattleAction battleAction = BattleAction::Attack;
  int battleHighlightIndex = 0;
  int enemyHp[8] = {10, 10, 8, 8, 5, 5, 5, 5};

  string actionText = "What would you like to do?";

  GameScreen currentScreen = GameScreen::Battle;

  // Main loop
  while (isRunning)
  {
    fill(newlyPressedKeys, newlyPressedKeys + keyboardSize, 0);

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
          SDL_PushEvent(&quitEvent);
          break;
        default:
          if (windowEvent.key.repeat == 0)
          {
            newlyPressedKeys[windowEvent.key.keysym.scancode] = 1;
          }
        }
        break;
      }
    }

    if (isWalking && frameCount >= walkStart + WALK_FRAMES)
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

    if (newlyPressedKeys[SDL_SCANCODE_B])
    {
      if (currentScreen == GameScreen::Battle)
      {
        currentScreen = GameScreen::Map;
      }
      else
      {
        currentScreen = GameScreen::Battle;
      }
    }

    switch (currentScreen)
    {
    case GameScreen::Map:
    {
      if (!isWalking)
      {
        if (keyboardState[SDL_SCANCODE_LEFT])
        {

          isWalking = true;
          walkStart = frameCount;
          walkDirection = LEFT;
          facing = LEFT;
        }
        else if (keyboardState[SDL_SCANCODE_RIGHT])
        {

          isWalking = true;
          walkStart = frameCount;
          walkDirection = RIGHT;
          facing = RIGHT;
        }
        else if (keyboardState[SDL_SCANCODE_UP])
        {

          isWalking = true;
          walkStart = frameCount;
          walkDirection = UP;
          facing = UP;
        }
        else if (keyboardState[SDL_SCANCODE_DOWN])
        {

          isWalking = true;
          walkStart = frameCount;
          walkDirection = DOWN;
          facing = DOWN;
        }
      }

      if (newlyPressedKeys[SDL_SCANCODE_Z])
      {
        if (!showText)
        {
          showText = true;
          textCharsToShow = 0;
        }
        else if (textCharsToShow < bottomText.length())
        {
          textCharsToShow = bottomText.length();
        }
        else
        {
          showText = false;
        }
      }

      if (newlyPressedKeys[SDL_SCANCODE_R])
      {
        textCharsToShow = 0;
      }

      break;
    }
    case GameScreen::Battle:
    {
      if (newlyPressedKeys[SDL_SCANCODE_UP] || newlyPressedKeys[SDL_SCANCODE_RIGHT])
      {
        if (battleStep == BattleStep::Action)
        {
          int newAction = static_cast<int>(battleAction) - 1;
          if (newAction < 0)
            newAction += 4;
          battleAction = static_cast<BattleAction>(newAction);
        }
        else if (battleStep == BattleStep::Target)
        {
          battleHighlightIndex--;
          battleHighlightIndex = (battleHighlightIndex + 8) % 8;
        }
      }
      if (newlyPressedKeys[SDL_SCANCODE_DOWN] || newlyPressedKeys[SDL_SCANCODE_LEFT])
      {
        if (battleStep == BattleStep::Action)
        {
          int newAction = static_cast<int>(battleAction) + 1;
          if (newAction >= 4)
            newAction -= 4;
          battleAction = static_cast<BattleAction>(newAction);
        }
        else if (battleStep == BattleStep::Target)
        {
          battleHighlightIndex++;
          battleHighlightIndex %= 8;
        }
      }
      if (newlyPressedKeys[SDL_SCANCODE_Z])
      {
        battleCharsToShow = 0;
        switch (battleStep)
        {
        case BattleStep::Action:
        {
          if (none_of(begin(enemyHp), end(enemyHp), isPositive))
          {
            currentScreen = GameScreen::Map;
            break;
          }

          switch (battleAction)
          {
          case BattleAction::Attack:
          case BattleAction::Magic:
          {
            actionText = "Select a target";
            battleStep = BattleStep::Target;
            break;
          }
          case BattleAction::Item:
          {
            int healing = rand() % 4 + 1;
            actionText = format("Used an item!\n\nHealed {} health!", healing);
            battleStep = BattleStep::Result;
            break;
          }
          case BattleAction::Run:
          {
            actionText = "Attempted to run away!";
            battleStep = BattleStep::Result;
            break;
          }
          }
          break;
        }
        case BattleStep::Target:
        {
          switch (battleAction)
          {
          case BattleAction::Attack:
          {
            damageDealt = rand() % 3 + 1;
            enemyHp[battleHighlightIndex] -= damageDealt;
            actionText = format("Swung with staff!\n\nDid {} damage!\n\nEnemy has {} health left.", damageDealt, enemyHp[battleHighlightIndex]);
            break;
          }
          case BattleAction::Magic:
          {
            damageDealt = rand() % 5 + 1;
            enemyHp[battleHighlightIndex] -= damageDealt;
            actionText = format("Cast a mighty spell!\n\nDid {} damage!\n\nEnemy has {} health left.", damageDealt, enemyHp[battleHighlightIndex]);
            break;
          }
          default:
          {
            break;
          }
          }
          battleStep = BattleStep::Result;
          break;
        }
        case BattleStep::Result:
        {
          if (none_of(begin(enemyHp), end(enemyHp), isPositive))
          {
            actionText = "You win!";
          }
          else
          {
            actionText = "What would you like to do?";
          }

          battleStep = BattleStep::Action;
          break;
        }
        }

        break;
      }
    }
    }

    while (chrono::steady_clock::now() > currentTime + frameLength)
    {
      currentTime += frameLength;

      // Render
      SDL_RenderClear(renderer);

      switch (currentScreen)
      {
      case GameScreen::Map:
      {
        if (isWalking)
        {
          walkPercentDone = (double)(frameCount - walkStart) / (double)WALK_FRAMES;
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
              SDL_RenderCopy(renderer, worldMap, &grassRect, &bgDrawRect);
              break;
            case W:
              SDL_RenderCopy(renderer, worldMap, &waterRect, &bgDrawRect);
              break;
            case M:
              SDL_RenderCopy(renderer, worldMap, &mountainRect, &bgDrawRect);
              break;
            case H:
              SDL_RenderCopy(renderer, worldMap, &hillsRect, &bgDrawRect);
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
        SDL_RenderCopyEx(renderer, characters, &wizardSprite, &playerPosition, 0, NULL, flip);

        textRenderer->SetTextColor(230, 230, 230);
        guiRect = {x : 20, y : 130, w : 280, h : 40};

        if (showText)
        {
          if (frameCount % 3 == 0)
          {
            textCharsToShow++;
          }
          DrawTextBox(textRenderer, bottomText, renderer, gui, &guiRect, 75, 75, 105, textCharsToShow);
        }

        break;
      }
      case GameScreen::Battle:
      {
        guiRect = {x : 0, y : 0, w : GAME_W, h : GAME_H};
        DrawGuiBox(renderer, gui, &guiRect);
        guiRect = {x : 0, y : 104, w : GAME_W, h : GUI_BORDER_H};
        DrawGuiLineH(renderer, gui, &guiRect, &junctionR, &junctionL);
        guiRect = {x : 143, y : 0, w : GUI_BORDER_W, h : 109};
        DrawGuiLineV(renderer, gui, &guiRect, &junctionB, &junctionT);
        guiRect = {x : 167, y : 104, w : GUI_BORDER_W, h : 76};
        DrawGuiLineV(renderer, gui, &guiRect, &junctionB, &junctionT);

        SDL_RenderCopy(renderer, battle, &battleAttack, &battleAttackPos);
        SDL_RenderCopy(renderer, battle, &battleMagic, &battleMagicPos);
        SDL_RenderCopy(renderer, battle, &battleItem, &battleItemPos);
        SDL_RenderCopy(renderer, battle, &battleRun, &battleRunPos);

        SDL_RenderCopy(renderer, battleBGs, &battleBGPlains, &battleBGPos);

        guiRect = {x : 175, y : 114 + 11 * static_cast<int>(battleAction), w : 4, h : 5};
        SDL_RenderCopy(renderer, battle, &battleSelect, &guiRect);

        if (frameCount % 3 == 0)
        {
          battleCharsToShow++;
        }
        textRenderer->SetTextColor(230, 230, 230);
        textRenderer->DrawTextWrapped(actionText, &descriptionBoxPos, battleCharsToShow);

        bool enemyAnimPhase = frameCount / 180 % 2 == 0;

        if (enemyHp[0] > 0)
          SDL_RenderCopy(renderer, enemies, enemyAnimPhase ? &enemyClamhead1 : &enemyClamhead2, &enemySlot0);
        if (enemyHp[1] > 0)
          SDL_RenderCopy(renderer, enemies, enemyAnimPhase ? &enemyClamhead2 : &enemyClamhead1, &enemySlot1);

        if (enemyHp[2] > 0)
          SDL_RenderCopy(renderer, enemies, enemyAnimPhase ? &enemyGoblin1 : &enemyGoblin2, &enemySlot2);
        if (enemyHp[3] > 0)
          SDL_RenderCopy(renderer, enemies, enemyAnimPhase ? &enemyGoblin2 : &enemyGoblin1, &enemySlot3);

        if (enemyHp[4] > 0)
          SDL_RenderCopy(renderer, enemies, enemyAnimPhase ? &enemyRat1 : &enemyRat2, &enemySlot4);
        if (enemyHp[5] > 0)
          SDL_RenderCopy(renderer, enemies, enemyAnimPhase ? &enemyRat2 : &enemyRat1, &enemySlot5);
        if (enemyHp[6] > 0)
          SDL_RenderCopy(renderer, enemies, enemyAnimPhase ? &enemyRat1 : &enemyRat2, &enemySlot6);
        if (enemyHp[7] > 0)
          SDL_RenderCopy(renderer, enemies, enemyAnimPhase ? &enemyRat2 : &enemyRat1, &enemySlot7);

        if (battleStep == BattleStep::Target)
        {
          SDL_Rect currentEnemySlot;
          SetEnemySlot(battleHighlightIndex, currentEnemySlot);
          HighlightSlot(renderer, battle, &currentEnemySlot);
        }

        break;
      }
      }

      SDL_RenderPresent(renderer);

      frameCount++;
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
