#pragma once

#include <string>
#include <queue>
#include <unordered_map>
#include <SDL.h>

using namespace std;

class TextRenderer
{
public:
  TextRenderer(SDL_Renderer *renderer, SDL_Texture *font);

  void DrawText(
      const string &text,
      SDL_Rect *textArea);

  void DrawCharAt(
      const char c,
      const SDL_Rect *textArea,
      int positionInRow,
      int rowIndex);

  void DrawTextWrapped(
      const string &text,
      const SDL_Rect *textArea,
      int charsToRender = -1);

  void SetTextColor(int r, int g, int b);

private:
  SDL_Renderer *renderer;
  SDL_Texture *font;
  unordered_map<char, SDL_Rect> letterRects;
};
