#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <string>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <chrono>
#include <iomanip>

using namespace std;
using namespace chrono;

const int SLOT_ROWS = 7;
const int SLOT_COLS = 10;
const int GRID_SIZE_ROW = SLOT_ROWS + 2;
const int GRID_SIZE_COL = SLOT_COLS + 2;
bool occupied[SLOT_ROWS][SLOT_COLS];
map<pair<int, int>, string> slotVehicleMap;
map<string, steady_clock::time_point> entryTimes;
double totalCharges = 0.0;

vector<pair<int, int>> gates = {
    {1, 1}, {1, GRID_SIZE_COL - 2}, {GRID_SIZE_ROW - 2, 1}, {GRID_SIZE_ROW - 2, GRID_SIZE_COL - 2}
};

bool isValid(int x, int y) {
    return x >= 0 && x < SLOT_ROWS && y >= 0 && y < SLOT_COLS;
}

void initRandomOccupancy() {
    srand(time(0));
    int count = rand() % 35;
    for (int i = 0; i < count;) {
        int r = rand() % SLOT_ROWS;
        int c = rand() % SLOT_COLS;
        if (!occupied[r][c]) {
            occupied[r][c] = true;
            string plate = "XYZ" + to_string(rand() % 100);
            slotVehicleMap[{r, c}] = plate;
            entryTimes[plate] = steady_clock::now();
            i++;
        }
    }
}

pair<int, int> findNearestEmptySlot(pair<int, int> gate) {
    int startR = gate.first - 1;
    int startC = gate.second - 1;
    queue<pair<int, int>> q;
    bool visited[SLOT_ROWS][SLOT_COLS] = {};
    q.push({startR, startC});
    visited[startR][startC] = true;

    while (!q.empty()) {
        auto [r, c] = q.front(); q.pop();
        if (!occupied[r][c]) return {r, c};
        for (auto d : vector<pair<int, int>>{{-1,0},{1,0},{0,-1},{0,1}}) {
            int nr = r + d.first, nc = c + d.second;
            if (isValid(nr, nc) && !visited[nr][nc]) {
                visited[nr][nc] = true;
                q.push({nr, nc});
            }
        }
    }
    return {-1, -1};
}

int findNearestGate(int r, int c) {
    int minDist = 1e9, gateIndex = 0;
    for (int i = 0; i < gates.size(); ++i) {
        int gr = gates[i].first - 1;
        int gc = gates[i].second - 1;
        int dist = abs(gr - r) + abs(gc - c);
        if (dist < minDist) {
            minDist = dist;
            gateIndex = i;
        }
    }
    return gateIndex;
}

void logToFile(const string& action, const string& plate, int slotNo, int gateNo, double charge = 0) {
    ofstream file("log.txt", ios::app);
    auto now = system_clock::to_time_t(system_clock::now());
    file << action << ": Vehicle " << plate
         << " | Slot " << slotNo
         << " | Gate " << gateNo
         << " | Time " << put_time(localtime(&now), "%Y-%m-%d %H:%M:%S");
    if (action == "Exited") {
        file << " | Charge ₹" << charge;
    }
    file << "\n";
    file.close();
}

void displaySlots() {
    cout << "\nCurrent Parking Grid:\n";
    for (int i = 0; i < GRID_SIZE_ROW; ++i) {
        for (int j = 0; j < GRID_SIZE_COL; ++j) {
            if (i == 0 || i == GRID_SIZE_ROW - 1 || j == 0 || j == GRID_SIZE_COL - 1) {
                if (i == 0 && j == 0) cout << "G1";
                else if (i == 0 && j == GRID_SIZE_COL - 1) cout << "G2";
                else if (i == GRID_SIZE_ROW - 1 && j == 0) cout << "G3";
                else if (i == GRID_SIZE_ROW - 1 && j == GRID_SIZE_COL - 1) cout << "G4";
                else cout << " .";
            } else {
                int r = i - 1, c = j - 1;
                if (occupied[r][c])
                    cout << " \033[31mx\033[0m"; // red x for occupied
                else
                    cout << " \033[32mo\033[0m"; // green o for empty
            }
        }
        cout << "\n";
    }
}

int main() {
    ofstream appendFile("log.txt", ios::app);
    appendFile << "\n=== New Parking Session Started ===\n";
    appendFile.close();

    initRandomOccupancy();

    while (true) {
        displaySlots();
        cout << "\n1. Enter Vehicle\n2. Remove Vehicle\n3. Search Slot\n4. Show Charges\n5. Exit\nChoose option: ";
        int opt;
        cin >> opt;

        if (opt == 1) {
            int g;
            cout << "Choose Gate (1-4): ";
            cin >> g;
            if (g < 1 || g > 4) continue;
            pair<int, int> slot = findNearestEmptySlot(gates[g - 1]);
            if (slot.first == -1) {
                cout << "No empty slots!\n";
            } else {
                string plate;
                cout << "Enter Vehicle Plate: ";
                cin >> plate;
                cout << "\n";
                occupied[slot.first][slot.second] = true;
                slotVehicleMap[slot] = plate;
                entryTimes[plate] = steady_clock::now();
                int slotNo = slot.first * SLOT_COLS + slot.second + 1;
                cout << "Allocated to Slot " << slotNo << "\n";
                logToFile("Entered", plate, slotNo, g);
            }
        }

        else if (opt == 2) {
            int s;
            cout << "Enter Slot Number to remove vehicle (1-" << SLOT_ROWS * SLOT_COLS << "): ";
            cin >> s;
            cout << "\n";
            if (s < 1 || s > SLOT_ROWS * SLOT_COLS) {
                cout << "Invalid Slot Number.\n";
                continue;
            }
            int r = (s - 1) / SLOT_COLS;
            int c = (s - 1) % SLOT_COLS;
            if (!occupied[r][c]) {
                cout << "Slot is already empty.\n";
                continue;
            }
            string plate = slotVehicleMap[{r, c}];
            auto now = steady_clock::now();
            auto duration = duration_cast<seconds>(now - entryTimes[plate]).count();
            double charge = duration * 2;
            totalCharges += charge;

            int gateNo = findNearestGate(r, c);
            occupied[r][c] = false;
            slotVehicleMap.erase({r, c});
            entryTimes.erase(plate);

            cout << "Vehicle " << plate << " removed from Slot " << s << "\n";
            cout << "Exited through Gate " << (gateNo + 1) << "\n";
            cout << "Parking Time: " << duration << " seconds\n";
            cout << "Charge: Rs." << charge << "\n";

            logToFile("Exited", plate, s, gateNo + 1, charge);
        }

        else if (opt == 3) {
            string plate;
            cout << "Enter Vehicle Plate to Search: ";
            cin >> plate;
            cout << "\n";
            bool found = false;
            for (auto& [slot, val] : slotVehicleMap) {
                if (val == plate) {
                    int slotNo = slot.first * SLOT_COLS + slot.second + 1;
                    cout << "Vehicle " << plate << " is at Slot " << slotNo << "\n";
                    found = true;
                    break;
                }
            }
            if (!found) cout << "Vehicle not found in the parking.\n";
        }

        else if (opt == 4) {
            cout << "\nTotal Charges Collected: Rs." << totalCharges << "\n";
        }

        else {
            break;
        }

        cout << "\n";
    }
    return 0;
}
