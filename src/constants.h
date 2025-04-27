#pragma once

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
