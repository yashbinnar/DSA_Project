#include "parking_backend.h"
#include <cstdlib>
#include <ctime>
#include <queue>
#include <fstream>
#include <cmath>

bool occupied[ROWS][COLS] = {};
std::map<std::pair<int, int>, std::string> slotVehicleMap;
std::vector<std::pair<int, int>> gates = {{0,0}, {0, COLS-1}, {ROWS-1, 0}, {ROWS-1, COLS-1}};

bool isValid(int x, int y) {
    return x >= 0 && x < ROWS && y >= 0 && y < COLS;
}

void initRandomOccupancy() {
    srand((unsigned)time(0));
    int count = rand() % 50;
    for (int i = 0; i < count;) {
        int r = rand() % ROWS;
        int c = rand() % COLS;
        if (!occupied[r][c]) {
            occupied[r][c] = true;
            slotVehicleMap[{r, c}] = "XYZ" + std::to_string(rand() % 100);
            i++;
        }
    }
}

std::pair<int, int> findNearestEmptySlot(std::pair<int, int> start) {
    bool visited[ROWS][COLS] = {};
    std::queue<std::pair<int, int>> q;
    q.push(start);
    visited[start.first][start.second] = true;
    
    while (!q.empty()) {
        std::pair<int, int> current = q.front(); q.pop(); // Using a named pair
        int r = current.first;
        int c = current.second;
        
        if (!occupied[r][c]) return {r, c}; // Return the empty slot
        
        // Directions for exploring neighbors
        std::vector<std::pair<int, int>> directions = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
        for (auto d : directions) {
            int nr = r + d.first, nc = c + d.second;
            if (isValid(nr, nc) && !visited[nr][nc]) {
                visited[nr][nc] = true;
                q.push({nr, nc});
            }
        }
    }
    
    return {-1, -1}; // No empty slot found
}

int findNearestGate(int r, int c) {
    int minDist = 1e9, gateIndex = 0;
    for (int i = 0; i < gates.size(); ++i) {
        int dist = abs(gates[i].first - r) + abs(gates[i].second - c);
        if (dist < minDist) {
            minDist = dist;
            gateIndex = i;
        }
    }
    return gateIndex;
}

void logToFile(const std::string& action, const std::string& plate, int slotNo, int gateNo) {
    std::ofstream file("log.txt", std::ios::app);
    file << action << ": Vehicle " << plate
         << " | Slot " << slotNo
         << " | Gate " << gateNo << "\n";
    file.close();
}
