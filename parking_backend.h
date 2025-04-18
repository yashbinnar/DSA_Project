#ifndef PARKING_BACKEND_H
#define PARKING_BACKEND_H

#include <vector>
#include <map>
#include <string>
#include <utility>

const int ROWS = 10;
const int COLS = 10;

extern bool occupied[ROWS][COLS];
extern std::map<std::pair<int, int>, std::string> slotVehicleMap;
extern std::vector<std::pair<int, int>> gates;

bool isValid(int x, int y);
void initRandomOccupancy();
std::pair<int, int> findNearestEmptySlot(std::pair<int, int> start);
int findNearestGate(int r, int c);
void logToFile(const std::string& action, const std::string& plate, int slotNo, int gateNo);

#endif
