// ==============================================================================
// FLOOD FILL MAZE SOLVING ALGORITHM (16x16 GRID)
// Adapted from Technoxian reference for ESP32-C6
// ==============================================================================

// Direction neighbor offset additions for 1D array index
const int16_t DIR_OFFSET[4] = { -MAZE_COLS, 1, MAZE_COLS, -1 };

// Direction preference penalties (prefer straight=0, turn=1, u-turn=2)
const uint8_t DIR_PENALTY[4] = { 0, 1, 2, 1 };

CellInfo s_maze[TOTAL_CELLS];

// Center destination cells (7,7), (7,8), (8,7), (8,8)
const uint8_t TARGET_CELLS[4] = {
  (7 * MAZE_COLS + 7), // 119
  (7 * MAZE_COLS + 8), // 120
  (8 * MAZE_COLS + 7), // 135
  (8 * MAZE_COLS + 8)  // 136
};

// State variables
uint8_t currentCell = 0;
uint8_t currentDir  = DIR_NORTH;
uint8_t targetCell  = 0;
uint8_t targetRelativeDir = DIR_NORTH;
uint8_t runStepBlocks = 1;

// Self-contained circular queue for Flood Fill
class Queue512 {
private:
  uint8_t data[512];
  uint16_t head = 0;
  uint16_t tail = 0;
  uint16_t count = 0;
public:
  void enqueue(uint8_t item) {
    if (count < 512) {
      data[tail] = item;
      tail = (tail + 1) % 512;
      count++;
    }
  }
  uint8_t dequeue() {
    if (count > 0) {
      uint8_t item = data[head];
      head = (head + 1) % 512;
      count--;
      return item;
    }
    return 0;
  }
  bool isEmpty() const { return count == 0; }
  void clear() { head = tail = count = 0; }
};

static Queue512 s_floodQueue;

// Helper Macros
inline uint8_t cellRow(uint8_t loc) { return loc / MAZE_COLS; }
inline uint8_t cellCol(uint8_t loc) { return loc % MAZE_COLS; }
inline bool isTarget(uint8_t loc) {
  return (loc == TARGET_CELLS[0] || loc == TARGET_CELLS[1] ||
          loc == TARGET_CELLS[2] || loc == TARGET_CELLS[3]);
}

inline bool hasWall(uint8_t loc, uint8_t dir) {
  return (s_maze[loc].walls & (1 << dir)) != 0;
}

void setWall(uint8_t loc, uint8_t dir) {
  s_maze[loc].walls |= (1 << dir);
  // Set opposite wall on neighbor if valid
  int16_t nLoc = (int16_t)loc + DIR_OFFSET[dir];
  if (nLoc >= 0 && nLoc < TOTAL_CELLS) {
    uint8_t oppDir = (dir + 2) % 4;
    s_maze[nLoc].walls |= (1 << oppDir);
  }
}

bool isValidNeighbor(uint8_t loc, uint8_t dir) {
  if (dir == DIR_NORTH) return cellRow(loc) > 0;
  if (dir == DIR_EAST)  return cellCol(loc) < (MAZE_COLS - 1);
  if (dir == DIR_SOUTH) return cellRow(loc) < (MAZE_ROWS - 1);
  if (dir == DIR_WEST)  return cellCol(loc) > 0;
  return false;
}

uint8_t getNeighborLoc(uint8_t loc, uint8_t dir) {
  return (uint8_t)((int16_t)loc + DIR_OFFSET[dir]);
}

uint8_t getNeighborDistance(uint8_t loc, uint8_t dir) {
  if (hasWall(loc, dir) || !isValidNeighbor(loc, dir)) {
    return 255;
  }
  return s_maze[getNeighborLoc(loc, dir)].flood;
}

void initMaze() {
  for (uint16_t i = 0; i < TOTAL_CELLS; i++) {
    s_maze[i].walls = 0;
    s_maze[i].visited = 0;

    // Initialize Manhattan distance to center target
    int r = cellRow((uint8_t)i);
    int c = cellCol((uint8_t)i);
    int dr = min(abs(r - 7), abs(r - 8));
    int dc = min(abs(c - 7), abs(c - 8));
    s_maze[i].flood = (uint8_t)(dr + dc);
  }

  // Set boundary walls around the 16x16 perimeter
  for (uint8_t c = 0; c < MAZE_COLS; c++) {
    s_maze[c].walls |= (1 << DIR_NORTH);                               // Top row
    s_maze[(MAZE_ROWS - 1) * MAZE_COLS + c].walls |= (1 << DIR_SOUTH); // Bottom row
  }
  for (uint8_t r = 0; r < MAZE_ROWS; r++) {
    s_maze[r * MAZE_COLS].walls |= (1 << DIR_WEST);                    // Left col
    s_maze[r * MAZE_COLS + (MAZE_COLS - 1)].walls |= (1 << DIR_EAST);  // Right col
  }

  // Starting cell walls (typically start cell 0 has east wall)
  currentCell = 0;
  currentDir  = DIR_NORTH;
}

void updateWallsFromSensors() {
  readDistances();

  uint8_t leftDir  = (currentDir + 3) % 4;
  uint8_t rightDir = (currentDir + 1) % 4;

  if (hasLeftWall()) {
    setWall(currentCell, leftDir);
  }
  if (hasFrontWall()) {
    setWall(currentCell, currentDir);
  }
  if (hasRightWall()) {
    setWall(currentCell, rightDir);
  }
}

void runFloodFill() {
  s_floodQueue.clear();
  s_floodQueue.enqueue(currentCell);

  while (!s_floodQueue.isEmpty()) {
    uint8_t loc = s_floodQueue.dequeue();
    if (s_maze[loc].walls == 15) continue; // Completely enclosed

    uint8_t minNeighbor = 255;
    for (uint8_t d = 0; d < 4; d++) {
      uint8_t dist = getNeighborDistance(loc, d);
      if (dist < minNeighbor) {
        minNeighbor = dist;
      }
    }

    // A cell's flood value must equal min(neighbor) + 1
    if (minNeighbor != 255 && s_maze[loc].flood != (minNeighbor + 1)) {
      s_maze[loc].flood = minNeighbor + 1;

      // Push valid non-destination neighbors into queue to propagate
      for (uint8_t d = 0; d < 4; d++) {
        if (isValidNeighbor(loc, d) && !hasWall(loc, d)) {
          uint8_t nLoc = getNeighborLoc(loc, d);
          if (!isTarget(nLoc)) {
            s_floodQueue.enqueue(nLoc);
          }
        }
      }
    }
  }
}

uint8_t getAbsoluteDirection(uint8_t fromLoc, uint8_t toLoc) {
  int16_t diff = (int16_t)toLoc - (int16_t)fromLoc;
  if (diff == -MAZE_COLS) return DIR_NORTH;
  if (diff == 1)          return DIR_EAST;
  if (diff == MAZE_COLS)  return DIR_SOUTH;
  if (diff == -1)         return DIR_WEST;
  return DIR_NORTH;
}

void planNextCell() {
  uint8_t bestDist = 255;
  uint8_t bestScore = 3;
  targetCell = currentCell;

  for (uint8_t d = 0; d < 4; d++) {
    if (!hasWall(currentCell, d) && isValidNeighbor(currentCell, d)) {
      uint8_t nLoc = getNeighborLoc(currentCell, d);
      uint8_t nDist = s_maze[nLoc].flood;
      uint8_t relDir = (d + 4 - currentDir) % 4;
      uint8_t penalty = DIR_PENALTY[relDir];

      // Prefer smaller distance, or equal distance with lower turn penalty (prefer straight)
      if ((nDist < bestDist) || (nDist == bestDist && penalty < bestScore)) {
        bestDist = nDist;
        bestScore = penalty;
        targetCell = nLoc;
      }
    }
  }

  uint8_t absDir = getAbsoluteDirection(currentCell, targetCell);
  targetRelativeDir = (absDir + 4 - currentDir) % 4;

  // Check if tunnel continues ahead (straight run optimization)
  runStepBlocks = 1;
  uint8_t testCell = targetCell;
  uint8_t straightDir = absDir;

  while (isValidNeighbor(testCell, straightDir) && !hasWall(testCell, straightDir)) {
    uint8_t ahead = getNeighborLoc(testCell, straightDir);
    uint8_t leftD  = (straightDir + 3) % 4;
    uint8_t rightD = (straightDir + 1) % 4;

    // If both left and right walls exist and flood decreases, sprint straight!
    if (hasWall(testCell, leftD) && hasWall(testCell, rightD) &&
        s_maze[ahead].flood == (s_maze[testCell].flood - 1)) {
      testCell = ahead;
      runStepBlocks++;
      if (runStepBlocks >= 4) break; // Limit burst to 4 cells for safety
    } else {
      break;
    }
  }
}

void executeNavigationStep() {
  // 1. Turn to face target cell
  if (targetRelativeDir == 1) {
    turnAngle(90);  // Turn Right
  } else if (targetRelativeDir == 2) {
    turnAngle(180); // U-turn
  } else if (targetRelativeDir == 3) {
    turnAngle(-90); // Turn Left
  }

  if (s_robotState != STATE_RUNNING) return;

  // Update robot absolute heading
  currentDir = (currentDir + targetRelativeDir) % 4;

  // 2. Move forward target number of cells
  moveForwardCells(runStepBlocks);

  if (s_robotState != STATE_RUNNING) return;

  // Update current cell location
  for (uint8_t b = 0; b < runStepBlocks; b++) {
    currentCell = getNeighborLoc(currentCell, currentDir);
    s_maze[currentCell].visited = 1;
  }
}

void executeLeftWallFollowerStep() {
  // 1. Scan walls using precise laser distance sensors
  readDistances();
  bool wallL = hasLeftWall();
  bool wallF = hasFrontWall();
  bool wallR = hasRightWall();

  Serial.printf("[Wall-Follower] Cell %d (r:%d, c:%d) Facing %d | Walls: L=%d, F=%d, R=%d\n",
                currentCell, cellRow(currentCell), cellCol(currentCell), currentDir,
                wallL, wallF, wallR);

  // Record walls into maze map while exploring
  uint8_t leftDir  = (currentDir + 3) % 4;
  uint8_t rightDir = (currentDir + 1) % 4;
  if (wallL) setWall(currentCell, leftDir);
  if (wallF) setWall(currentCell, currentDir);
  if (wallR) setWall(currentCell, rightDir);

  // 2. Left-Hand Wall Follower Priority:
  // Priority 1: Opening on the Left? -> Turn Left and advance
  // Priority 2: Front is Clear?      -> Drive straight ahead
  // Priority 3: Opening on Right?    -> Turn Right and advance
  // Priority 4: Dead end?            -> Turn 180 (U-turn) and advance
  if (!wallL) {
    Serial.println(F(" -> Action: Opening on LEFT -> Turn Left 90 and move"));
    turnAngle(-90);
    currentDir = (currentDir + 3) % 4;
  } else if (!wallF) {
    Serial.println(F(" -> Action: Front is CLEAR -> Move straight (hugging left wall)"));
  } else if (!wallR) {
    Serial.println(F(" -> Action: Left & Front blocked -> Turn Right 90 and move"));
    turnAngle(90);
    currentDir = (currentDir + 1) % 4;
  } else {
    Serial.println(F(" -> Action: DEAD END -> Turn 180 (U-turn) and move"));
    turnAngle(180);
    currentDir = (currentDir + 2) % 4;
  }

  if (s_robotState != STATE_RUNNING) return;

  // 3. Move forward exactly 1 cell
  moveForwardCells(1);

  if (s_robotState != STATE_RUNNING) return;

  // 4. Update cell coordinates
  currentCell = getNeighborLoc(currentCell, currentDir);
  s_maze[currentCell].visited = 1;
}
