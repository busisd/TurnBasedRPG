#include <string>
#include <queue>
#include <unordered_map>
#include <SDL.h>
#include "text_renderer.h"

using namespace std;

const int LETTER_W = 8, LETTER_H = 8;
const string FONT_CHARACTERS = " !',-.0123456789?ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz:";
const int FONT_ROWS = 7, FONT_COLUMNS = 10;

TextRenderer::TextRenderer(SDL_Renderer *renderer, SDL_Texture *font) : renderer(renderer), font(font)
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

void TextRenderer::DrawText(
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
    SDL_RenderCopy(renderer, font, &letterRects[text.at(pos)], &dstRect);
  }
}

void TextRenderer::DrawCharAt(
    const char c,
    const SDL_Rect *textArea,
    int positionInRow,
    int rowIndex)
{
  int relativeX = positionInRow * LETTER_W;
  int relativeY = rowIndex * LETTER_H;
  SDL_Rect dstRect = {x : textArea->x + relativeX, y : textArea->y + relativeY, w : LETTER_W, h : LETTER_H};
  SDL_RenderCopy(renderer, font, &letterRects[c], &dstRect);
}

void TextRenderer::DrawTextWrapped(
    const string &text,
    const SDL_Rect *textArea,
    int charsToRender)
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

void TextRenderer::SetTextColor(int r, int g, int b)
{
  SDL_SetTextureColorMod(font, r, g, b);
}
