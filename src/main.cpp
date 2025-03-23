#include <iostream>
#include <list>
#include <queue>
#include <vector>
#include <chrono>
#include <unordered_map>
#include <SDL.h>
#include <SDL_image.h>

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
const auto WALK_TIME = chrono::duration_cast<std::chrono::nanoseconds>(chrono::milliseconds(300));

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
          const SDL_RendererFlip flip = SDL_FLIP_NONE,
          const double rotate = 0)
{
  const SDL_Rect scaledDstrect = {
    x : dstrect->x * SCALING_FACTOR,
    y : dstrect->y * SCALING_FACTOR,
    w : dstrect->w * SCALING_FACTOR,
    h : dstrect->h * SCALING_FACTOR,
  };
  SDL_Point temp = {0, 0};
  SDL_RenderCopyEx(renderer, texture, srcrect, &scaledDstrect, rotate, &temp, flip);
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

  void DrawCharAt(
      const char c,
      SDL_Rect *textArea,
      int positionInRow,
      int rowIndex)
  {
    int relativeX = positionInRow * LETTER_W;
    int relativeY = rowIndex * LETTER_H;
    SDL_Rect dstRect = {x : textArea->x + relativeX, y : textArea->y + relativeY, w : LETTER_W, h : LETTER_H};
    Draw(renderer, font, &letterRects[c], &dstRect);
  }

  void DrawTextWrapped(
      const string &text,
      SDL_Rect *textArea,
      int charsToRender = -1)
  {
    const int rowLength = (textArea->w / LETTER_W),
              totalRows = (textArea->h / LETTER_H);

    string strToRender = text + " ";

    queue<char> charBuf;
    int curCharsInRow = 0;
    int curRowIndex = 0;
    int totalCharsRendered = 0;
    for (char c : strToRender)
    {
      if (c == ' ' || c == '\n')
      {
        if (charBuf.size() > rowLength)
        {
          break;
        }

        int spaceCharNeeded = curCharsInRow > 0;
        if (curCharsInRow + spaceCharNeeded + charBuf.size() > rowLength)
        {
          curRowIndex++;
          curCharsInRow = 0;
          spaceCharNeeded = 0;
        }

        if (curRowIndex >= totalRows)
        {
          break;
        }

        curCharsInRow += spaceCharNeeded;
        totalCharsRendered += spaceCharNeeded;
        while (charBuf.size() > 0)
        {
          if (charsToRender >= 0 && totalCharsRendered >= charsToRender)
          {
            return;
          }
          DrawCharAt(charBuf.front(), textArea, curCharsInRow, curRowIndex);
          curCharsInRow++;
          totalCharsRendered++;
          charBuf.pop();
        }

        if (c == '\n')
        {
          curRowIndex++;
          curCharsInRow = 0;
          spaceCharNeeded = 0;
          totalCharsRendered++;
        }
      }
      else
      {
        if (letterRects.contains(c))
        {
          charBuf.push(c);
        }
      }
    }
  }

  void SetTextColor(
      int r, int g, int b)
  {
    SDL_SetTextureColorMod(font, r, g, b);
  }

private:
  SDL_Renderer *renderer;
  SDL_Texture *font;
  unordered_map<char, SDL_Rect> letterRects;
};

const int GUI_Y = 0;
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
  Draw(renderer, gui, &borderHorizontal, &guiRect);

  if (endpointL != NULL)
  {
    guiRect = {x : lineRect->x, y : lineRect->y, w : GUI_BORDER_W, h : GUI_BORDER_H};
    Draw(renderer, gui, endpointL, &guiRect);
  }
  if (endpointR != NULL)
  {
    guiRect = {x : lineRect->x + lineRect->w - GUI_BORDER_W, y : lineRect->y, w : GUI_BORDER_W, h : GUI_BORDER_H};
    Draw(renderer, gui, endpointR, &guiRect);
  }
}

void DrawGuiLineV(SDL_Renderer *renderer, SDL_Texture *gui, SDL_Rect *lineRect, SDL_Rect *endpointT = NULL, SDL_Rect *endpointB = NULL)
{
  SDL_SetTextureColorMod(gui, 255, 255, 255);
  SDL_Rect guiRect;
  int marginY = (endpointT != NULL) * GUI_BORDER_W;
  int marginH = (endpointB != NULL) * GUI_BORDER_W + marginY;
  guiRect = {x : lineRect->x, y : lineRect->y + marginY, w : GUI_BORDER_W, h : lineRect->h - marginH};
  Draw(renderer, gui, &borderVertical, &guiRect);

  if (endpointT != NULL)
  {
    guiRect = {x : lineRect->x, y : lineRect->y, w : GUI_BORDER_W, h : GUI_BORDER_H};
    Draw(renderer, gui, endpointT, &guiRect);
  }
  if (endpointB != NULL)
  {
    guiRect = {x : lineRect->x, y : lineRect->y + lineRect->h - GUI_BORDER_H, w : GUI_BORDER_W, h : GUI_BORDER_H};
    Draw(renderer, gui, endpointB, &guiRect);
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
    Draw(renderer, gui, &guiFill, &guiRect);
  }
}

void DrawTextBox(TextRenderer *textRenderer, const string &text, SDL_Renderer *renderer, SDL_Texture *gui, SDL_Rect *textArea, int r = 0, int g = 0, int b = 0, int charsToRender = -1)
{
  SDL_Rect borderRect = {x : textArea->x - GUI_BORDER_W - 1, y : textArea->y - GUI_BORDER_H - 1, w : textArea->w + GUI_BORDER_W * 2 + 2, h : textArea->h + GUI_BORDER_H * 2 + 2};
  DrawGuiBox(renderer, gui, &borderRect, true, r, g, b);
  textRenderer->DrawTextWrapped(text, textArea, charsToRender);
}

int main(int argc, char **argv)
{
  string exe_path = argv[0];
  string build_dir_path = exe_path.substr(0, exe_path.find_last_of("\\"));
  string project_dir_path = build_dir_path.substr(0, build_dir_path.find_last_of("\\"));

  // Setup
  SDL_Init(SDL_INIT_EVENTS);

  SDL_Window *window = SDL_CreateWindow("TBRPG", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_W, SCREEN_H, SDL_WINDOW_FULLSCREEN_DESKTOP);
  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
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

  int tileScreenLeft = PLAYER_X % TILE_W - TILE_W, tileScreenTop = PLAYER_Y % TILE_H - TILE_H;
  int tileDrawW = SCALING_FACTOR * TILE_W, tileDrawH = SCALING_FACTOR * TILE_H;
  SDL_Rect bgDrawRect = {x : 0, y : 0, w : TILE_W, h : TILE_H};

  auto frameLength = chrono::nanoseconds{(int)(1.0 / MAX_FPS * 1000.0 * 1000.0 * 1000.0)};
  auto currentTime = chrono::steady_clock::now() - frameLength;
  unsigned long long int frameCount = 0;

  bool isWalking = false;
  auto walkStart = currentTime;
  double walkPercentDone;
  Direction walkDirection;
  Direction facing = DOWN;

  bool showText = false;
  int textCharsToShow = 0;
  string bottomText = string("This is some text in a text box! Go forth, wizard, and cast spells! Huzzah! You will win!\n") +
                      string("Furthermore, you may even get to ponder an orb at some point!");

  bool showBattle = false;

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

    if (isWalking && chrono::steady_clock::now() > walkStart + WALK_TIME)
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
      if (keyboardState[SDL_SCANCODE_LEFT])
      {

        isWalking = true;
        walkStart = chrono::steady_clock::now();
        walkDirection = LEFT;
        facing = LEFT;
      }
      else if (keyboardState[SDL_SCANCODE_RIGHT])
      {

        isWalking = true;
        walkStart = chrono::steady_clock::now();
        walkDirection = RIGHT;
        facing = RIGHT;
      }
      else if (keyboardState[SDL_SCANCODE_UP])
      {

        isWalking = true;
        walkStart = chrono::steady_clock::now();
        walkDirection = UP;
        facing = UP;
      }
      else if (keyboardState[SDL_SCANCODE_DOWN])
      {

        isWalking = true;
        walkStart = chrono::steady_clock::now();
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

    if (newlyPressedKeys[SDL_SCANCODE_B])
    {
      showBattle = !showBattle;
    }

    if (newlyPressedKeys[SDL_SCANCODE_R])
    {
      textCharsToShow = 0;
    }

    while (chrono::steady_clock::now() > currentTime + frameLength)
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

      textRenderer->SetTextColor(255, 255, 255);
      guiRect = {x : 20, y : 130, w : 280, h : 40};

      if (showText)
      {
        if (frameCount % 3 == 0)
        {
          textCharsToShow++;
        }
        DrawTextBox(textRenderer, bottomText, renderer, gui, &guiRect, 75, 75, 105, textCharsToShow);
      }

      if (showBattle)
      {
        guiRect = {x : 0, y : 0, w : GAME_W, h : GAME_H};
        DrawGuiBox(renderer, gui, &guiRect);
        guiRect = {x : 0, y : 100, w : GAME_W, h : GUI_BORDER_H};
        DrawGuiLineH(renderer, gui, &guiRect, &junctionR, &junctionL);
        guiRect = {x : 160, y : 0, w : GUI_BORDER_W, h : 105};
        DrawGuiLineV(renderer, gui, &guiRect, &junctionB, &junctionT);
        guiRect = {x : 140, y : 100, w : GUI_BORDER_W, h : 80};
        DrawGuiLineV(renderer, gui, &guiRect, &junctionB, &junctionT);
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
