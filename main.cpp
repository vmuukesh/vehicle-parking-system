#include <algorithm>
#include <cctype>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

const string DATA_FILE = "parking_data.txt";
const string HISTORY_FILE = "history.csv";


enum class VehicleType { Bike = 1, Car = 2, Truck = 3 };

string typeName(VehicleType t) {
    switch (t) {
        case VehicleType::Bike:  return "Bike";
        case VehicleType::Car:   return "Car";
        case VehicleType::Truck: return "Truck";
    }
    return "Unknown";
}

double hourlyRate(VehicleType t) {
    switch (t) {
        case VehicleType::Bike:  return 10.0;
        case VehicleType::Car:   return 30.0;
        case VehicleType::Truck: return 60.0;
    }
    return 0.0;
}
string normalisePlate(const string& raw) {
    string out;
    for (char c : raw) {
        if (!isspace(static_cast<unsigned char>(c)) && c != '-')
            out += static_cast<char>(toupper(static_cast<unsigned char>(c)));
    }
    return out;
}

string formatTime(time_t t) {
    char buf[32];
    strftime(buf, sizeof(buf), "%d-%m-%Y %H:%M", localtime(&t));
    return buf;
}

int readInt(const string& prompt, int lo, int hi) {
    int value;
    while (true) {
        cout << prompt;
        if (cin >> value && value >= lo && value <= hi) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return value;
        }
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "  Invalid input. Enter a number between " << lo << " and " << hi << ".\n";
    }
}

string readPlate() {
    string line, plate;
    while (true) {
        cout << "Enter vehicle number (e.g. AP39AB1234): ";
        getline(cin, line);
        plate = normalisePlate(line);
        if (plate.size() >= 4 && plate.size() <= 12) return plate;
        cout << "  Invalid vehicle number.\n";
    }
}

// Number of billable hours: every started hour counts, minimum one.
int billableHours(time_t entry, time_t exit) {
    double seconds = difftime(exit, entry);
    int hours = static_cast<int>(ceil(seconds / 3600.0));
    return max(hours, 1);
}
struct Vehicle {
    string plate;
    VehicleType type = VehicleType::Car;
    time_t entryTime = 0;
};
public:
    Slot(int number, VehicleType type) : number_(number), type_(type) {}

    int number() const { return number_; }
    VehicleType type() const { return type_; }
    bool isFree() const { return !occupied_; }
    const Vehicle& vehicle() const { return vehicle_; }

    void park(const Vehicle& v) {
        vehicle_ = v;
        occupied_ = true;
    }

    Vehicle release() {
        occupied_ = false;
        return vehicle_;
    }

private:
    int number_;
    VehicleType type_;
    bool occupied_ = false;
    Vehicle vehicle_;
};
class ParkingLot {
public:
    ParkingLot(int bikeSlots, int carSlots, int truckSlots) {
        int n = 1;
        for (int i = 0; i < bikeSlots; ++i)  slots_.emplace_back(n++, VehicleType::Bike);
        for (int i = 0; i < carSlots; ++i)   slots_.emplace_back(n++, VehicleType::Car);
        for (int i = 0; i < truckSlots; ++i) slots_.emplace_back(n++, VehicleType::Truck);
    }

    void parkVehicle(const string& plate, VehicleType type) {
        if (findByPlate(plate)) {
            cout << "  This vehicle is already parked.\n";
            return;
        }
        Slot* slot = findFreeSlot(type);
        if (!slot) {
            cout << "  Sorry, no free " << typeName(type) << " slot available.\n";
            return;
        }
        Vehicle v;
        v.plate = plate;
        v.type = type;
        v.entryTime = time(nullptr);
        slot->park(v);
        save();

        cout << "\n  ---- PARKING TICKET ----\n"
             << "  Vehicle : " << plate << " (" << typeName(type) << ")\n"
             << "  Slot    : " << slot->number() << "\n"
             << "  Entry   : " << formatTime(v.entryTime) << "\n"
             << "  Rate    : Rs. " << hourlyRate(type) << " / hour\n";
    }

    void removeVehicle(const string& plate) {
        Slot* slot = findByPlate(plate);
        if (!slot) {
            cout << "  No parked vehicle found with that number.\n";
            return;
        }
        time_t now = time(nullptr);
        Vehicle v = slot->release();
        int hours = billableHours(v.entryTime, now);
        double fee = hours * hourlyRate(v.type);

        cout << "\n  ---- PARKING BILL ----\n"
             << "  Vehicle : " << v.plate << " (" << typeName(v.type) << ")\n"
             << "  Slot    : " << slot->number() << "\n"
             << "  Entry   : " << formatTime(v.entryTime) << "\n"
             << "  Exit    : " << formatTime(now) << "\n"
             << "  Hours   : " << hours << "\n"
             << fixed << setprecision(2)
             << "  Amount  : Rs. " << fee << "\n";

        appendHistory(v, slot->number(), now, fee);
        save();
    }

    void searchVehicle(const string& plate) const {
        const Slot* slot = findByPlate(plate);
        if (!slot) {
            cout << "  Vehicle not found in the parking lot.\n";
            return;
        }
        cout << "  Found: " << slot->vehicle().plate << " in slot " << slot->number()
             << ", parked since " << formatTime(slot->vehicle().entryTime) << "\n";
    }

    void showSlots() const {
        cout << "\n  Slot  Type   Status\n  ----------------------------\n";
        for (const Slot& s : slots_) {
            cout << "  " << setw(4) << left << s.number()
                 << setw(7) << typeName(s.type())
                 << (s.isFree() ? "Free" : "Occupied (" + s.vehicle().plate + ")") << "\n";
        }
    }

    void showParkedVehicles() const {
        bool any = false;
        cout << "\n  Slot  Vehicle       Type   Entry time\n"
             << "  ------------------------------------------\n";
        for (const Slot& s : slots_) {
            if (s.isFree()) continue;
            any = true;
            cout << "  " << setw(4) << left << s.number()
                 << setw(14) << s.vehicle().plate
                 << setw(7) << typeName(s.type())
                 << formatTime(s.vehicle().entryTime) << "\n";
        }
        if (!any) cout << "  (no vehicles parked)\n";
    }

    void showSummary() const {
        for (VehicleType t : {VehicleType::Bike, VehicleType::Car, VehicleType::Truck}) {
            int total = 0, used = 0;
            for (const Slot& s : slots_) {
                if (s.type() != t) continue;
                ++total;
                if (!s.isFree()) ++used;
            }
            cout << "  " << setw(6) << left << typeName(t)
                 << used << " / " << total << " occupied\n";
        }
    }

    void showHistory() const {
        ifstream in(HISTORY_FILE);
        if (!in) {
            cout << "  No history yet.\n";
            return;
        }
        string line;
        getline(in, line);  // skip header
        cout << "\n  Vehicle      Type   Slot  Entry              Exit               Fee\n"
             << "  ---------------------------------------------------------------------\n";
        bool any = false;
        while (getline(in, line)) {
            vector<string> f;
            stringstream ss(line);
            string part;
            while (getline(ss, part, ',')) f.push_back(part);
            if (f.size() != 6) continue;
            any = true;
            cout << "  " << setw(13) << left << f[0] << setw(7) << f[1] << setw(6) << f[2]
                 << setw(19) << formatTime(stol(f[3])) << setw(19) << formatTime(stol(f[4]))
                 << "Rs. " << f[5] << "\n";
        }
        if (!any) cout << "  No history yet.\n";
    }

    // Saved format, one parked vehicle per line:  plate type slotNumber entryTime
    void save() const {
        ofstream out(DATA_FILE);
        for (const Slot& s : slots_) {
            if (s.isFree()) continue;
            out << s.vehicle().plate << ' ' << static_cast<int>(s.vehicle().type) << ' '
                << s.number() << ' ' << s.vehicle().entryTime << '\n';
        }
    }

    void load() {
        ifstream in(DATA_FILE);
        if (!in) return;
        string plate;
        int type, slotNo;
        long long entry;
        while (in >> plate >> type >> slotNo >> entry) {
            if (slotNo < 1 || slotNo > static_cast<int>(slots_.size())) continue;
            if (type < 1 || type > 3) continue;
            Slot& s = slots_[slotNo - 1];
            if (s.type() != static_cast<VehicleType>(type)) continue;
            Vehicle v;
            v.plate = plate;
            v.type = s.type();
            v.entryTime = static_cast<time_t>(entry);
            s.park(v);
        }
    }

private:
    vector<Slot> slots_;

    Slot* findFreeSlot(VehicleType type) {
        for (Slot& s : slots_)
            if (s.type() == type && s.isFree()) return &s;
        return nullptr;
    }

    Slot* findByPlate(const string& plate) {
        for (Slot& s : slots_)
            if (!s.isFree() && s.vehicle().plate == plate) return &s;
        return nullptr;
    }

    const Slot* findByPlate(const string& plate) const {
        for (const Slot& s : slots_)
            if (!s.isFree() && s.vehicle().plate == plate) return &s;
        return nullptr;
    }

    void appendHistory(const Vehicle& v, int slotNo, time_t exitTime, double fee) const {
        bool newFile = false;
        {
            ifstream check(HISTORY_FILE);
            newFile = !check.good();
        }
        ofstream out(HISTORY_FILE, ios::app);
        if (newFile) out << "plate,type,slot,entry,exit,fee\n";
        out << v.plate << ',' << typeName(v.type) << ',' << slotNo << ','
            << v.entryTime << ',' << exitTime << ',' << fixed << setprecision(2) << fee << '\n';
    }
};

VehicleType askVehicleType() {
    cout << "  1. Bike\n  2. Car\n  3. Truck\n";
    return static_cast<VehicleType>(readInt("Select vehicle type: ", 1, 3));
}

void printMenu() {
    cout << "\n==== VEHICLE PARKING MANAGEMENT SYSTEM ====\n"
         << "  1. Park a vehicle\n"
         << "  2. Remove a vehicle and generate bill\n"
         << "  3. Search a vehicle\n"
         << "  4. Show all slots\n"
         << "  5. Show parked vehicles\n"
         << "  6. Occupancy summary\n"
         << "  7. Parking history\n"
         << "  0. Exit\n";
}

int main() {
    ParkingLot lot(10, 20, 5);  // bike, car, truck slots
    lot.load();

    while (true) {
        printMenu();
        int choice = readInt("Enter your choice: ", 0, 7);
        switch (choice) {
            case 1: {
                VehicleType t = askVehicleType();
                lot.parkVehicle(readPlate(), t);
                break;
            }
            case 2: lot.removeVehicle(readPlate()); break;
            case 3: lot.searchVehicle(readPlate()); break;
            case 4: lot.showSlots(); break;
            case 5: lot.showParkedVehicles(); break;
            case 6: lot.showSummary(); break;
            case 7: lot.showHistory(); break;
            case 0:
                lot.save();
                cout << "Goodbye!\n";
                return 0;
        }
    }
}
