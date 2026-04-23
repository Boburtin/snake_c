#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

#define PX_PER_CELL 20
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define COLS (WINDOW_WIDTH / PX_PER_CELL)
#define ROWS (WINDOW_HEIGHT / PX_PER_CELL)
#define START_X (COLS / 2)
#define START_Y (ROWS / 2)
#define START_INDEX ((START_Y * COLS) + START_X)
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

typedef struct {
  int x;
  int y;
} Pvec;

typedef enum { RIGHT, DOWN, LEFT, UP } Direction;

static Pvec DIR_DELTAS[] = {
    [RIGHT] = {1, 0}, [DOWN] = {0, 1}, [LEFT] = {-1, 0}, [UP] = {0, -1}};

typedef struct {
  Direction dir;
  Direction pen_dir;
  Pvec cells[ROWS * COLS];
  int head;
  int len;
} Snake;

Snake snake = {0};
Pvec food;
int board[ROWS * COLS];

HBRUSH hBrushSnake;
HBRUSH hBrushFood;
HBRUSH hBrushBg;

typedef enum { EMPTY, FOOD, SNAKE_BODY } TileItems;

BOOL CanPlaceFood(Pvec p) {
  for (int i = 0; i < snake.len; i++) {
    int idx = (snake.head - i + ROWS * COLS) % (ROWS * COLS);
    if (snake.cells[idx].x == p.x && snake.cells[idx].y == p.y) {
      return FALSE;
    }
  }
  return TRUE;
}

void WipeBoard(int* board, int b_size) {
  memset(board, 0, sizeof(int) * b_size);
}

void InitGameState(void) {
  snake.head = 0;
  snake.len = 1;
  snake.dir = RIGHT;
  snake.pen_dir = RIGHT;
  snake.cells[0] = (Pvec){START_X, START_Y};
  do {
    food.x = (rand() % COLS);
    food.y = (rand() % ROWS);
  } while (!CanPlaceFood(food));
  board[(food.y * COLS) + food.x] = FOOD;
  board[START_INDEX] = SNAKE_BODY;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
  srand(time(NULL));
  hBrushSnake = CreateSolidBrush(RGB(255, 255, 255));
  hBrushFood = CreateSolidBrush(RGB(0, 255, 0));
  hBrushBg = CreateSolidBrush(RGB(0, 0, 0));
  const wchar_t CLASS_NAME[] = L"Snake Window Class";
  WNDCLASS wc = {0};
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = CLASS_NAME;
  RegisterClass(&wc);

  RECT rc = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
  DWORD style = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
  AdjustWindowRectEx(&rc, style, FALSE, 0);

  HWND hwnd =
      CreateWindowEx(0, CLASS_NAME, L"Snake by 3_meo",
                     style, CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left,
                     rc.bottom - rc.top, NULL, NULL, hInstance, NULL);
  if (hwnd == NULL) {
    return 0;
  }
  ShowWindow(hwnd, nCmdShow);
  InitGameState();
  SetTimer(hwnd, 1, 150, NULL);

  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return 0;
}

BOOL UpdateGameState(void) {
  snake.dir = snake.pen_dir;
  Pvec next = {snake.cells[snake.head].x + DIR_DELTAS[snake.dir].x,
               snake.cells[snake.head].y + DIR_DELTAS[snake.dir].y};
  if ((next.x < 0 || next.x >= COLS || next.y < 0 || next.y >= ROWS) ||
      board[(next.y * COLS) + next.x] == SNAKE_BODY) {
    return FALSE;
  }
  if (next.x == food.x && next.y == food.y) {
    snake.head = (snake.head + 1) % (ROWS * COLS);
    snake.cells[snake.head] = next;
    snake.len++;
    board[(next.y * COLS) + next.x] = SNAKE_BODY;
    do {
      food.x = (rand() % COLS);
      food.y = (rand() % ROWS);
    } while (!CanPlaceFood(food));
    board[(food.y * COLS) + food.x] = FOOD;
  } else {
    int tail = (snake.head - snake.len + 1 + ROWS * COLS) % (ROWS * COLS);
    board[(snake.cells[tail].y * COLS) + snake.cells[tail].x] = EMPTY;
    snake.head = (snake.head + 1) % (ROWS * COLS);
    snake.cells[snake.head] = next;
    board[(next.y * COLS) + next.x] = SNAKE_BODY;
  }
  return TRUE;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                            LPARAM lParam) {
  switch (uMsg) {
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hwnd, &ps);
      FillRect(hdc, &ps.rcPaint, hBrushBg);

      for (int i = 0; i < snake.len; i++) {
        int idx = (snake.head - i + ROWS * COLS) % (ROWS * COLS);
        RECT cellRect = {snake.cells[idx].x * PX_PER_CELL,
                         snake.cells[idx].y * PX_PER_CELL,
                         snake.cells[idx].x * PX_PER_CELL + PX_PER_CELL,
                         snake.cells[idx].y * PX_PER_CELL + PX_PER_CELL};
        FillRect(hdc, &cellRect, hBrushSnake);
      }  // food
      RECT foodRect = {food.x * PX_PER_CELL, food.y * PX_PER_CELL,
                       food.x * PX_PER_CELL + PX_PER_CELL,
                       food.y * PX_PER_CELL + PX_PER_CELL};
      FillRect(hdc, &foodRect, hBrushFood);

      EndPaint(hwnd, &ps);
      return 0;
    }
    case WM_TIMER: {
      if (!UpdateGameState()) {
        WipeBoard(board, COLS * ROWS);
        InitGameState();
      }
      InvalidateRect(hwnd, NULL, FALSE);
      return 0;
    }
    case WM_KEYDOWN: {
      Direction candidate;
      switch (wParam) {
        case VK_RIGHT:
          candidate = RIGHT;
          break;
        case VK_DOWN:
          candidate = DOWN;
          break;
        case VK_LEFT:
          candidate = LEFT;
          break;
        case VK_UP:
          candidate = UP;
          break;
        default:
          return 0;
      }
      Pvec canDir = DIR_DELTAS[candidate];
      Pvec curDir = DIR_DELTAS[snake.dir];
      if (canDir.x + curDir.x != 0 || canDir.y + curDir.y != 0) {
        snake.pen_dir = candidate;
      }
      return 0;
    }
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
