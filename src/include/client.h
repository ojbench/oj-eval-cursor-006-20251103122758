#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <utility>
#include <vector>
#include <string>

extern int rows;         // The count of rows of the game map.
extern int columns;      // The count of columns of the game map.
extern int total_mines;  // The count of mines of the game map.

// You MUST NOT use any other external variables except for rows, columns and total_mines.

// Client-side knowledge of the board
static std::vector<std::string> g_board;

/**
 * @brief The definition of function Execute(int, int, bool)
 *
 * @details This function is designed to take a step when player the client's (or player's) role, and the implementation
 * of it has been finished by TA. (I hope my comments in code would be easy to understand T_T) If you do not understand
 * the contents, please ask TA for help immediately!!!
 *
 * @param r The row coordinate (0-based) of the block to be visited.
 * @param c The column coordinate (0-based) of the block to be visited.
 * @param type The type of operation to a certain block.
 * If type == 0, we'll execute VisitBlock(row, column).
 * If type == 1, we'll execute MarkMine(row, column).
 * If type == 2, we'll execute AutoExplore(row, column).
 * You should not call this function with other type values.
 */
void Execute(int r, int c, int type);

/**
 * @brief The definition of function InitGame()
 *
 * @details This function is designed to initialize the game. It should be called at the beginning of the game, which
 * will read the scale of the game map and the first step taken by the server (see README).
 */
void InitGame() {
  // Initialize all client-side knowledge structures
  // We'll maintain the latest observed map as characters
  // '?' for unknown, '@' for marked mines, '0'-'8' for revealed numbers
  // Note: We will size containers based on rows/columns globals
  int first_row, first_column;
  std::cin >> first_row >> first_column;
  g_board.assign(rows, std::string(columns, '?'));
  Execute(first_row, first_column, 0);
}

/**
 * @brief The definition of function ReadMap()
 *
 * @details This function is designed to read the game map from stdin when playing the client's (or player's) role.
 * Since the client (or player) can only get the limited information of the game map, so if there is a 3 * 3 map as
 * above and only the block (2, 0) has been visited, the stdin would be
 *     ???
 *     12?
 *     01?
 */
void ReadMap() {
  if (static_cast<int>(g_board.size()) != rows) {
    g_board.assign(rows, std::string(columns, '?'));
  }
  for (int i = 0; i < rows; ++i) {
    std::string line;
    std::cin >> line;
    if (static_cast<int>(line.size()) == columns) {
      g_board[i] = line;
    } else {
      // Fallback to preserve robustness if input malformed
      for (int j = 0; j < columns && j < static_cast<int>(line.size()); ++j) {
        g_board[i][j] = line[j];
      }
    }
  }
}

/**
 * @brief The definition of function Decide()
 *
 * @details This function is designed to decide the next step when playing the client's (or player's) role. Open up your
 * mind and make your decision here! Caution: you can only execute once in this function.
 */
void Decide() {
  std::vector<std::string>& board = g_board;

  auto in_bounds = [&](int r, int c) { return r >= 0 && r < rows && c >= 0 && c < columns; };

  // 1) Auto-explore when number equals marked neighbors
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < columns; ++c) {
      char ch = board[r][c];
      if (ch >= '0' && ch <= '8') {
        int number = ch - '0';
        int marked = 0;
        int unknown = 0;
        for (int dr = -1; dr <= 1; ++dr) {
          for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) continue;
            int nr = r + dr, nc = c + dc;
            if (!in_bounds(nr, nc)) continue;
            if (board[nr][nc] == '@') ++marked;
            else if (board[nr][nc] == '?') ++unknown;
          }
        }
        if (marked == number && unknown > 0) {
          Execute(r, c, 2);
          return;
        }
      }
    }
  }

  // 2) Mark mines when number == marked + unknown (all unknown are mines)
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < columns; ++c) {
      char ch = board[r][c];
      if (ch >= '0' && ch <= '8') {
        int number = ch - '0';
        int marked = 0;
        int unknown = 0;
        int unk_r = -1, unk_c = -1;
        for (int dr = -1; dr <= 1; ++dr) {
          for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) continue;
            int nr = r + dr, nc = c + dc;
            if (!in_bounds(nr, nc)) continue;
            if (board[nr][nc] == '@') ++marked;
            else if (board[nr][nc] == '?') { ++unknown; unk_r = nr; unk_c = nc; }
          }
        }
        if (unknown > 0 && number == marked + unknown) {
          Execute(unk_r, unk_c, 1);
          return;
        }
      }
    }
  }

  // 3) Visit an unknown cell ? simple heuristic: prefer unknown neighbors of zeros
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < columns; ++c) {
      if (board[r][c] == '0') {
        for (int dr = -1; dr <= 1; ++dr) {
          for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) continue;
            int nr = r + dr, nc = c + dc;
            if (in_bounds(nr, nc) && board[nr][nc] == '?') {
              Execute(nr, nc, 0);
              return;
            }
          }
        }
      }
    }
  }

  // 4) Fallback: choose an unknown cell with minimal estimated risk
  int total_unknown = 0;
  int total_marked = 0;
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < columns; ++c) {
      total_unknown += (board[r][c] == '?');
      total_marked += (board[r][c] == '@');
    }
  }
  double global_risk = 1.0;
  if (total_unknown > 0) {
    int remaining_mines = total_mines - total_marked;
    if (remaining_mines < 0) remaining_mines = 0;
    global_risk = static_cast<double>(remaining_mines) / static_cast<double>(total_unknown);
  }

  double best_risk = 2.0;
  int best_r = -1, best_c = -1;
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < columns; ++c) {
      if (board[r][c] != '?') continue;
      double cell_risk = -1.0;
      // Use the maximum local constraint risk among adjacent numbers; fall back to global risk
      for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
          if (dr == 0 && dc == 0) continue;
          int nr = r + dr, nc = c + dc;
          if (!in_bounds(nr, nc)) continue;
          char ch2 = board[nr][nc];
          if (ch2 >= '0' && ch2 <= '8') {
            int number = ch2 - '0';
            int marked = 0;
            int unknown = 0;
            for (int ddr = -1; ddr <= 1; ++ddr) {
              for (int ddc = -1; ddc <= 1; ++ddc) {
                if (ddr == 0 && ddc == 0) continue;
                int rr = nr + ddr, cc = nc + ddc;
                if (!in_bounds(rr, cc)) continue;
                if (board[rr][cc] == '@') ++marked;
                else if (board[rr][cc] == '?') ++unknown;
              }
            }
            int remaining = number - marked;
            if (remaining < 0) remaining = 0;
            if (unknown > 0) {
              double local = static_cast<double>(remaining) / static_cast<double>(unknown);
              if (local > cell_risk) cell_risk = local;
            }
          }
        }
      }
      if (cell_risk < 0.0) cell_risk = global_risk;
      if (cell_risk < best_risk) {
        best_risk = cell_risk;
        best_r = r; best_c = c;
      }
    }
  }
  if (best_r != -1) {
    Execute(best_r, best_c, 0);
    return;
  }
}

#endif