#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <stdexcept>
#include <regex>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <cctype>
#include <cstdlib>
#include <utility>
using namespace std;

enum VehicleType { BIKE = 1, CAR = 2, TRUCK = 3 };

string typeNameOf(int t) { return t == BIKE ? "Bike" : (t == CAR ? "Car" : "Truck"); }

string getTimeText(time_t t) {
    char text[30];
    strftime(text, sizeof(text), "%d-%m-%Y %H:%M", localtime(&t));
    return text;
}

vector<string> split(const string &line, char sep) {
    vector<string> parts;
    string item;
    stringstream ss(line);
    while (getline(ss, item, sep)) parts.push_back(item);
    return parts;
}

template <typename T>
int argMax(const vector<T> &v) {
    int best = 0;
    for (int i = 1; i < (int)v.size(); i++)
        if (v[i] > v[best]) best = i;
    return best;
}

string normalizePlate(const string &line) {
    string out;
    for (char c : line) {
        if (c == ' ' || c == '-') continue;
        out += (char)toupper((unsigned char)c);
    }
    return out;
}
bool validPlate(const string &plate) {          // e.g. AP39AB1234
    static const regex format("^[A-Z]{2}[0-9]{2}[A-Z]{1,2}[0-9]{4}$");
    return regex_match(plate, format);
}

class ParkingException : public runtime_error {
public:
    explicit ParkingException(const string &m) : runtime_error(m) {}
};
class LotFullException : public ParkingException {
public:
    LotFullException(const string &t, const string &why)
        : ParkingException("Sorry, no free slot for a " + t + " (" + why + ").") {}
};
class VehicleNotFoundException : public ParkingException {
public:
    explicit VehicleNotFoundException(const string &n)
        : ParkingException("Vehicle " + n + " is not in the parking lot.") {}
};
class DuplicateVehicleException : public ParkingException {
public:
    explicit DuplicateVehicleException(const string &n)
        : ParkingException("Vehicle " + n + " is already parked.") {}
};

class Config {
    Config() : bikeRate(10), carRate(30), truckRate(60), carReserve(2), truckReserve(1),
               graceMinutes(15), dailyCapHours(8), peakStart(17), peakEnd(21), peakPercent(150),
               passDiscount(50), passDays(30), truckSurcharge(20), passHash(hashText("parking123")) {}
public:
    int bikeRate, carRate, truckRate;
    int carReserve, truckReserve;    
    int graceMinutes, dailyCapHours;
    int peakStart, peakEnd, peakPercent;
    int passDiscount, passDays, truckSurcharge;
    unsigned long long passHash;

    Config(const Config &) = delete;
    Config &operator=(const Config &) = delete;
    static Config &get() { static Config c; return c; }

    static unsigned long long hashText(const string &s) {
        unsigned long long h = 5381;
        for (char c : s) h = h * 33 + (unsigned char)c;
        return h;
    }
    void load() {
        ifstream f("config.txt");
        int b, c, t, cr, tr;
        unsigned long long h;
        if (f >> b >> c >> t >> cr >> tr >> h) {       
            bikeRate = b; carRate = c; truckRate = t; carReserve = cr; truckReserve = tr; passHash = h;
        }
    }
    void save() const {
        ofstream f("config.txt");
        f << bikeRate << " " << carRate << " " << truckRate << " "
          << carReserve << " " << truckReserve << " " << passHash << endl;
    }
};

class Vehicle {
protected:
    string number, owner, phone;
    time_t entryTime;
public:
    Vehicle(string n, string o, string p, time_t e = time(NULL))
        : number(n), owner(o), phone(p), entryTime(e) {}
    virtual ~Vehicle() = default;

    virtual int type() const = 0;
    virtual int ratePerHour() const = 0;
    virtual int surcharge() const { return 0; }
    string typeName() const { return typeNameOf(type()); }

    const string &getNumber() const { return number; }
    const string &getOwner() const { return owner; }
    const string &getPhone() const { return phone; }
    time_t getEntry() const { return entryTime; }

    bool operator==(const Vehicle &o) const { return number == o.number; }

    friend ostream &operator<<(ostream &os, const Vehicle &v) {
        os << "Vehicle number : " << v.number << "\n"
           << "Vehicle type   : " << v.typeName() << "\n"
           << "Owner name     : " << v.owner << "\n"
           << "Entry time     : " << getTimeText(v.entryTime) << "\n"
           << "Rate per hour  : Rs. " << v.ratePerHour() << "\n";
        return os;
    }
};
class Bike : public Vehicle {
public:
    using Vehicle::Vehicle;
    int type() const override { return BIKE; }
    int ratePerHour() const override { return Config::get().bikeRate; }
};
class Car : public Vehicle {
public:
    using Vehicle::Vehicle;
    int type() const override { return CAR; }
    int ratePerHour() const override { return Config::get().carRate; }
};
class Truck : public Vehicle {
public:
    using Vehicle::Vehicle;
    int type() const override { return TRUCK; }
    int ratePerHour() const override { return Config::get().truckRate; }
    int surcharge() const override { return Config::get().truckSurcharge; }
};

unique_ptr<Vehicle> makeVehicle(int type, const string &n, const string &o, const string &p,
                                time_t e = time(NULL)) {                      // Factory
    if (type == BIKE) return unique_ptr<Vehicle>(new Bike(n, o, p, e));
    if (type == CAR) return unique_ptr<Vehicle>(new Car(n, o, p, e));
    return unique_ptr<Vehicle>(new Truck(n, o, p, e));
}

class Slot {
    int slotNo, type;                     // type = physical bay size
    unique_ptr<Vehicle> vehicle;
public:
    Slot(int no, int t) : slotNo(no), type(t) {}
    int getSlotNo() const { return slotNo; }
    int getType() const { return type; }
    bool isFull() const { return vehicle != nullptr; }
    bool isOverflow() const { return vehicle && vehicle->type() != type; }
    const Vehicle *getVehicle() const { return vehicle.get(); }
    void park(unique_ptr<Vehicle> v) { vehicle = move(v); }
    unique_ptr<Vehicle> release() { return move(vehicle); }
};

void buildSlots(vector<Slot> &slots, int bikes, int cars, int trucks) {
    int no = 1;
    for (int i = 0; i < bikes; i++) slots.emplace_back(no++, BIKE);
    for (int i = 0; i < cars; i++) slots.emplace_back(no++, CAR);
    for (int i = 0; i < trucks; i++) slots.emplace_back(no++, TRUCK);
}

struct Decision { int index; string reason; };

class AllocationPolicy {
protected:
    static int firstFree(const vector<Slot> &s, int bayType) {   // lowest number = nearest entrance
        for (int i = 0; i < (int)s.size(); i++)
            if (s[i].getType() == bayType && !s[i].isFull()) return i;
        return -1;
    }
    static int freeCount(const vector<Slot> &s, int bayType) {
        int n = 0;
        for (const Slot &x : s) if (x.getType() == bayType && !x.isFull()) n++;
        return n;
    }
public:
    virtual ~AllocationPolicy() = default;
    virtual string name() const = 0;
    virtual Decision choose(const vector<Slot> &slots, int vehicleType) const = 0;
};

// Baseline: exact vehicle type only (the classic textbook rule)
class FixedPolicy : public AllocationPolicy {
public:
    string name() const override { return "fixed"; }
    Decision choose(const vector<Slot> &s, int t) const override {
        int i = firstFree(s, t);
        if (i >= 0) return {i, "native bay"};
        return {-1, "native bays full"};
    }
};

class ReservePolicy : public AllocationPolicy {
    int reserveFor(int bayType) const {
        return bayType == CAR ? Config::get().carReserve : (bayType == TRUCK ? Config::get().truckReserve : 0);
    }
public:
    string name() const override { return "reserve"; }
    Decision choose(const vector<Slot> &s, int t) const override {
        int i = firstFree(s, t);
        if (i >= 0) return {i, "native bay"};
        int target = t + 1;                                   // bike->car, car->truck only
        if (target <= TRUCK) {
            int freeNow = freeCount(s, target), reserve = reserveFor(target);
            if (freeNow > reserve) {
                int j = firstFree(s, target);
                return {j, "overflow into " + typeNameOf(target) + " bay; free after=" +
                           to_string(freeNow - 1) + "; reserve=" + to_string(reserve)};
            }
        }
        return {-1, "full or protected reserve; no legal upgrade"};
    }
};

struct FeeResult { int hours; int fee; bool freeGrace; bool discounted; bool peakApplied; };

class FeeCalculator {
public:
    static FeeResult compute(const Vehicle &v, time_t exitTime, bool hasPass) {
        const Config &c = Config::get();
        FeeResult r = {0, 0, false, false, false};
        long long seconds = exitTime - v.getEntry();
        if (seconds <= c.graceMinutes * 60LL) { r.freeGrace = true; return r; }

        int hours = (int)(seconds / 3600);
        if (seconds % 3600 != 0) hours++;
        if (hours == 0) hours = 1;
        r.hours = hours;

        int fee = 0;
        for (int h = 0; h < hours; h++) {                 // each started hour, peak-aware
            time_t blockStart = v.getEntry() + (time_t)h * 3600;
            int hr = localtime(&blockStart)->tm_hour;
            int rate = v.ratePerHour();
            if (hr >= c.peakStart && hr < c.peakEnd) { rate = rate * c.peakPercent / 100; r.peakApplied = true; }
            fee += rate;
        }
        int days = (hours + 23) / 24;                      // daily cap
        int cap = v.ratePerHour() * c.dailyCapHours * days;
        if (fee > cap) fee = cap;
        fee += v.surcharge();
        if (hasPass) { fee = fee * (100 - c.passDiscount) / 100; r.discounted = true; }
        r.fee = fee;
        return r;
    }
};

class Bill {
public:
    string number, type, owner, bayType;
    int slotNo, hours, fee;
    time_t entry, exitTime;
    bool freeGrace, discounted, peak;

    void saveReceipt() const {
        ofstream f("receipt_" + number + ".txt");
        f << *this;
    }
    friend ostream &operator<<(ostream &os, const Bill &b) {
        os << "------------- PARKING BILL -------------\n"
           << "Vehicle number : " << b.number << "\n"
           << "Vehicle type   : " << b.type << "\n"
           << "Owner name     : " << b.owner << "\n"
           << "Slot number    : " << b.slotNo << " (" << b.bayType << " bay)\n";
        if (b.bayType != b.type) os << "Allocation     : overflow, billed at " << b.type << " rate\n";
        os << "Entry time     : " << getTimeText(b.entry) << "\n"
           << "Exit time      : " << getTimeText(b.exitTime) << "\n"
           << "Total hours    : " << b.hours << "\n";
        if (b.freeGrace) os << "Grace period   : free exit\n";
        if (b.peak) os << "Peak pricing   : applied\n";
        if (b.discounted) os << "Monthly pass   : discount applied\n";
        os << "Total amount   : Rs. " << b.fee << "\n"
           << "----------------------------------------\n";
        return os;
    }
};

class PassManager {
    map<string, time_t> passes;
    string file;                                  // empty = in-memory only (tests)
public:
    explicit PassManager(string f = "") : file(f) {}
    void load() {
        if (file.empty()) return;
        ifstream f(file);
        string n; long long e;
        while (f >> n >> e) passes[n] = (time_t)e;
    }
    void save() const {
        if (file.empty()) return;
        ofstream f(file);
        for (auto &p : passes) f << p.first << " " << (long long)p.second << endl;
    }
    bool isValid(const string &n) const {
        auto it = passes.find(n);
        return it != passes.end() && it->second > time(NULL);
    }
    time_t addOrRenew(const string &n) {
        time_t base = time(NULL);
        auto it = passes.find(n);
        if (it != passes.end() && it->second > base) base = it->second;
        passes[n] = base + (time_t)Config::get().passDays * 86400;
        save();
        return passes[n];
    }
    void show() const {
        if (passes.empty()) { cout << "No monthly passes have been added yet." << endl; return; }
        cout << "\n" << left << setw(16) << "Vehicle" << setw(18) << "Expires" << "Status" << endl;
        for (auto &p : passes)
            cout << left << setw(16) << p.first << setw(18) << getTimeText(p.second)
                 << (p.second > time(NULL) ? "Active" : "Expired") << endl;
    }
};

struct HistoryRecord { string number; int type, slot; time_t entry, exitTime; int fee; };

vector<HistoryRecord> loadHistory(const string &file) {
    vector<HistoryRecord> out;
    ifstream f(file);
    string line;
    while (getline(f, line)) {
        vector<string> p = split(line, ',');
        if (p.size() != 6 || p[0] == "number") continue;
        try {
            out.push_back({p[0], stoi(p[1]), stoi(p[2]), (time_t)stoll(p[3]), (time_t)stoll(p[4]), stoi(p[5])});
        } catch (...) { /* skip corrupt line */ }
    }
    return out;
}

struct SimResult { int accepted; int byType[4]; };

SimResult simulate(const AllocationPolicy &policy, int bikes, int cars, int trucks) {
    vector<Slot> slots;
    buildSlots(slots, 10, 20, 5);
    SimResult r = {0, {0, 0, 0, 0}};
    int counts[4] = {0, bikes, cars, trucks};
    int id = 0;
    for (int t = BIKE; t <= TRUCK; t++)                       // bikes first, then cars, then trucks
        for (int i = 0; i < counts[t]; i++) {
            Decision d = policy.choose(slots, t);
            if (d.index < 0) continue;
            slots[d.index].park(makeVehicle(t, "SIM" + to_string(id++), "sim", "0000000000"));
            r.accepted++; r.byType[t]++;
        }
    return r;
}

void runComparison() {
    FixedPolicy fixed;
    ReservePolicy reserve;
    struct Scenario { const char *name; int b, c, t; };
    Scenario sc[] = {{"A", 16, 2, 1}, {"B", 10, 20, 5}, {"C", 30, 20, 5}, {"D", 12, 3, 1}};
    cout << "\n===== POLICY COMPARISON: FIXED (baseline) vs RESERVE (proposed) =====\n"
         << "Lot: 10 bike, 20 car, 5 truck bays | reserves: car=" << Config::get().carReserve
         << ", truck=" << Config::get().truckReserve << "\n"
         << "Arrival order: bikes, then cars, then trucks; no departures.\n\n"
         << left << setw(10) << "Scenario" << setw(12) << "Arrivals" << setw(16) << "Fixed (B/C/T)"
         << setw(16) << "Reserve (B/C/T)" << "Gain" << endl;
    cout << string(66, '-') << endl;
    for (const Scenario &s : sc) {
        SimResult f = simulate(fixed, s.b, s.c, s.t), r = simulate(reserve, s.b, s.c, s.t);
        string arr = to_string(s.b) + "/" + to_string(s.c) + "/" + to_string(s.t);
        string fm = to_string(f.accepted) + " (" + to_string(f.byType[1]) + "/" + to_string(f.byType[2]) + "/" + to_string(f.byType[3]) + ")";
        string rm = to_string(r.accepted) + " (" + to_string(r.byType[1]) + "/" + to_string(r.byType[2]) + "/" + to_string(r.byType[3]) + ")";
        int gain = r.accepted - f.accepted;
        cout << left << setw(10) << s.name << setw(12) << arr << setw(16) << fm << setw(16) << rm
             << (gain > 0 ? "+" : "") << gain << endl;
    }
    cout << "\nReading the table: the reserve policy helps when spare larger bays exist (A, D).\n"
         << "In C total admissions do not improve and later cars/trucks lose access, because\n"
         << "bikes arriving first consume larger bays. The policy is not claimed optimal." << endl;
}

class ParkingLot {
    vector<Slot> slots;
    PassManager passes;
    const AllocationPolicy &policy;
    string prefix;                                   // empty = no file I/O (tests)

    bool persist() const { return !prefix.empty(); }
    string dataFile() const { return prefix + "parking_data.txt"; }
    string historyFile() const { return prefix + "history.csv"; }

    int findVehicle(const string &n) const {
        for (int i = 0; i < (int)slots.size(); i++)
            if (slots[i].isFull() && slots[i].getVehicle()->getNumber() == n) return i;
        return -1;
    }
    void saveHistory(const Bill &b, int type) const {
        if (!persist()) return;
        bool fresh = !ifstream(historyFile()).good();
        ofstream f(historyFile(), ios::app);
        if (fresh) f << "number,type,slot,entry,exit,fee" << endl;
        f << b.number << "," << type << "," << b.slotNo << "," << (long long)b.entry << ","
          << (long long)b.exitTime << "," << b.fee << endl;
    }
public:
    ParkingLot(const AllocationPolicy &p, const string &filePrefix, int bikes = 10, int cars = 20, int trucks = 5)
        : passes(filePrefix.empty() ? "" : "passes.txt"), policy(p), prefix(filePrefix) {
        buildSlots(slots, bikes, cars, trucks);
    }
    PassManager &getPasses() { return passes; }
    const AllocationPolicy &getPolicy() const { return policy; }
    int totalSlots() const { return (int)slots.size(); }
    int slotOf(const string &n) const { int i = findVehicle(n); return i < 0 ? -1 : slots[i].getSlotNo(); }
    bool isOverflowAt(int slotNo) const { return slots[slotNo - 1].isOverflow(); }

    // Silent core operation (used by the UI wrapper and by unit tests)
    Decision tryPark(int type, const string &number, const string &owner, const string &phone) {
        if (findVehicle(number) != -1) throw DuplicateVehicleException(number);
        Decision d = policy.choose(slots, type);
        if (d.index < 0) throw LotFullException(typeNameOf(type), d.reason);
        slots[d.index].park(makeVehicle(type, number, owner, phone));
        saveData();
        return d;
    }
    void parkVehicle(int type, const string &number, const string &owner, const string &phone) {
        Decision d = tryPark(type, number, owner, phone);
        cout << "\nVehicle parked successfully.\n" << *slots[d.index].getVehicle()
             << "Slot number    : " << slots[d.index].getSlotNo() << " (" << typeNameOf(slots[d.index].getType()) << " bay)\n"
             << "Allocation     : " << d.reason << endl;
        if (passes.isValid(number)) cout << "Active monthly pass found: " << Config::get().passDiscount << "% off." << endl;
    }

    // simulatedMinutes < 0 -> real exit now; otherwise demo exit N minutes after entry
    Bill removeVehicle(const string &number, long long simulatedMinutes = -1) {
        int idx = findVehicle(number);
        if (idx == -1) throw VehicleNotFoundException(number);
        unique_ptr<Vehicle> v = slots[idx].release();
        time_t exitT = simulatedMinutes < 0 ? time(NULL) : v->getEntry() + (time_t)(simulatedMinutes * 60);
        FeeResult r = FeeCalculator::compute(*v, exitT, passes.isValid(number));

        Bill b;
        b.number = v->getNumber(); b.type = v->typeName(); b.owner = v->getOwner();
        b.bayType = typeNameOf(slots[idx].getType());
        b.slotNo = slots[idx].getSlotNo(); b.entry = v->getEntry(); b.exitTime = exitT;
        b.hours = r.hours; b.fee = r.fee;
        b.freeGrace = r.freeGrace; b.discounted = r.discounted; b.peak = r.peakApplied;

        saveHistory(b, v->type());
        if (persist()) b.saveReceipt();
        saveData();
        return b;
    }

    void searchVehicle(const string &number) const {
        int idx = findVehicle(number);
        if (idx == -1) throw VehicleNotFoundException(number);
        const Vehicle *v = slots[idx].getVehicle();
        cout << "Vehicle found.\n" << *v << "Slot number    : " << slots[idx].getSlotNo()
             << " (" << typeNameOf(slots[idx].getType()) << " bay)"
             << "\nPhone          : " << v->getPhone() << endl;
    }

    void showSlots() const {
        cout << "\n" << left << setw(8) << "Slot" << setw(8) << "Bay" << "Status" << endl;
        cout << "------------------------------------" << endl;
        for (const Slot &s : slots) {
            cout << left << setw(8) << s.getSlotNo() << setw(8) << typeNameOf(s.getType());
            if (!s.isFull()) cout << "Free";
            else cout << s.getVehicle()->getNumber() << " (" << s.getVehicle()->typeName()
                      << (s.isOverflow() ? ", overflow" : "") << ")";
            cout << endl;
        }
    }

    void showVehicles() const {
        bool found = false;
        cout << "\n" << left << setw(6) << "Slot" << setw(14) << "Number" << setw(8) << "Type"
             << setw(20) << "Owner" << "Entry time" << endl;
        cout << "-----------------------------------------------------------" << endl;
        for (const Slot &s : slots) {
            if (!s.isFull()) continue;
            const Vehicle *v = s.getVehicle();
            cout << left << setw(6) << s.getSlotNo() << setw(14) << v->getNumber()
                 << setw(8) << v->typeName() << setw(20) << v->getOwner() << getTimeText(v->getEntry()) << endl;
            found = true;
        }
        if (!found) cout << "No vehicles are parked right now." << endl;
    }

    void showCount() const {
        cout << "\nPolicy in use: " << policy.name() << endl;
        int allUsed = 0, overflow = 0;
        for (int t = BIKE; t <= TRUCK; t++) {
            int total = 0, used = 0;
            for (const Slot &s : slots)
                if (s.getType() == t) { total++; if (s.isFull()) used++; }
            allUsed += used;
            cout << typeNameOf(t) << " bays used: " << used << " out of " << total
                 << " (" << (total ? used * 100 / total : 0) << "%)" << endl;
        }
        for (const Slot &s : slots) if (s.isOverflow()) overflow++;
        cout << "Vehicles parked in overflow bays: " << overflow << endl;
        cout << "Overall occupancy: " << allUsed * 100 / (int)slots.size() << "%" << endl;
    }

    void showHistory() const {
        vector<HistoryRecord> h = loadHistory(historyFile());
        if (h.empty()) { cout << "There is no history yet." << endl; return; }
        cout << "\n" << left << setw(15) << "Number" << setw(8) << "Type" << setw(6) << "Slot"
             << setw(19) << "Entry" << setw(19) << "Exit" << "Amount" << endl;
        cout << "----------------------------------------------------------------------" << endl;
        for (const HistoryRecord &r : h)
            cout << left << setw(15) << r.number << setw(8) << typeNameOf(r.type) << setw(6) << r.slot
                 << setw(19) << getTimeText(r.entry) << setw(19) << getTimeText(r.exitTime) << "Rs. " << r.fee << endl;
    }

    void showEarnings(bool onlyToday) const {
        time_t now = time(NULL);
        tm today = *localtime(&now);
        int count = 0, total = 0;
        for (const HistoryRecord &r : loadHistory(historyFile())) {
            time_t t = r.exitTime;
            tm d = *localtime(&t);
            if (onlyToday && (d.tm_yday != today.tm_yday || d.tm_year != today.tm_year)) continue;
            count++; total += r.fee;
        }
        cout << "\n" << (onlyToday ? "Today's report" : "Total report") << endl;
        cout << "Vehicles served : " << count << endl;
        cout << "Money collected : Rs. " << total << endl;
    }

    void showAnalytics() const {
        vector<HistoryRecord> h = loadHistory(historyFile());
        if (h.empty()) { cout << "Not enough history for analytics yet." << endl; return; }
        vector<int> byHour(24, 0);
        int revenue[4] = {0, 0, 0, 0}, count[4] = {0, 0, 0, 0};
        for (const HistoryRecord &r : h) {
            time_t t = r.entry;
            byHour[localtime(&t)->tm_hour]++;
            if (r.type >= 1 && r.type <= 3) { revenue[r.type] += r.fee; count[r.type]++; }
        }
        int busy = argMax(byHour);
        cout << "\n----- ANALYTICS -----" << endl;
        cout << "Busiest entry hour : " << setfill('0') << setw(2) << busy << ":00 - "
             << setw(2) << (busy + 1) % 24 << ":00 (" << setfill(' ') << byHour[busy] << " vehicles)" << endl;
        for (int t = BIKE; t <= TRUCK; t++)
            cout << left << setw(6) << typeNameOf(t) << ": " << count[t] << " vehicles, Rs. " << revenue[t]
                 << (count[t] ? " (avg Rs. " + to_string(revenue[t] / count[t]) + ")" : "") << endl;
        showCount();
    }

    void saveData() const {
        if (!persist()) return;
        ofstream f(dataFile());
        for (const Slot &s : slots) {
            if (!s.isFull()) continue;
            const Vehicle *v = s.getVehicle();
            f << v->getNumber() << "|" << v->type() << "|" << s.getSlotNo() << "|"
              << (long long)v->getEntry() << "|" << v->getOwner() << "|" << v->getPhone() << endl;
        }
    }
    void loadData() {
        passes.load();
        if (!persist()) return;
        ifstream f(dataFile());
        string line;
        while (getline(f, line)) {
            vector<string> p = split(line, '|');
            if (p.size() != 6) continue;                                   // corrupt line: skip
            try {
                int type = stoi(p[1]), slotNo = stoi(p[2]);
                if (type < BIKE || type > TRUCK) continue;
                if (slotNo < 1 || slotNo > (int)slots.size()) continue;
                int bay = slots[slotNo - 1].getType();
                if (slots[slotNo - 1].isFull() || bay < type || bay - type > 1) continue;   // legal bay only
                slots[slotNo - 1].park(makeVehicle(type, p[0], p[4], p[5], (time_t)stoll(p[3])));
            } catch (...) {}
        }
    }
};

string readLine(const string &prompt) {
    cout << prompt;
    string s;
    if (!getline(cin, s)) { cout << "\nInput closed. Goodbye!" << endl; exit(0); }
    return s;
}
int getChoice(const string &msg, int low, int high) {
    while (true) {
        string s = readLine(msg);
        try {
            size_t pos;
            int c = stoi(s, &pos);
            if (pos == s.size() && c >= low && c <= high) return c;
        } catch (...) {}
        cout << "Wrong input. Please enter a number from " << low << " to " << high << "." << endl;
    }
}
string getVehicleNumber() {
    while (true) {
        string number = normalizePlate(readLine("Enter vehicle number (example AP39AB1234): "));
        if (validPlate(number)) return number;
        cout << "Invalid format. Use 2 letters, 2 digits, 1-2 letters, 4 digits." << endl;
    }
}
string getOwnerName() {
    while (true) {
        string line = readLine("Enter owner name: ");
        bool ok = line.size() >= 2 && line.size() <= 20;
        for (char c : line) if (!isalpha((unsigned char)c) && c != ' ') ok = false;
        if (ok) return line;
        cout << "Name should be 2 to 20 letters (letters and spaces only)." << endl;
    }
}
string getPhone() {
    while (true) {
        string line = readLine("Enter 10 digit mobile number: ");
        bool ok = line.size() == 10;
        for (char c : line) if (!isdigit((unsigned char)c)) ok = false;
        if (ok) return line;
        cout << "Mobile number must have exactly 10 digits." << endl;
    }
}
int getVehicleType() {
    cout << "1. Bike\n2. Car\n3. Truck" << endl;
    return getChoice("Choose vehicle type: ", 1, 3);
}

void showMenu(const AllocationPolicy &p) {
    cout << "\n===== PARKRESERVE - VEHICLE PARKING MANAGEMENT [" << p.name() << " policy] =====\n"
         << "1. Park a vehicle\n2. Remove a vehicle and get bill\n3. Search a vehicle\n"
         << "4. Show all slots\n5. Show parked vehicles\n6. Show slots used\n"
         << "7. Show parking history\n8. Admin login\n9. Policy comparison (fixed vs reserve)\n0. Exit" << endl;
}

void changeRates() {
    Config &c = Config::get();
    cout << "Current rates per hour: Bike Rs. " << c.bikeRate << ", Car Rs. " << c.carRate
         << ", Truck Rs. " << c.truckRate << endl;
    c.bikeRate = getChoice("Enter new bike rate: ", 1, 1000);
    c.carRate = getChoice("Enter new car rate: ", 1, 1000);
    c.truckRate = getChoice("Enter new truck rate: ", 1, 1000);
    c.save();
    cout << "Rates updated." << endl;
}

void changeReserves() {
    Config &c = Config::get();
    cout << "Protected reserves: car bays=" << c.carReserve << ", truck bays=" << c.truckReserve << endl;
    c.carReserve = getChoice("New car-bay reserve (0-19): ", 0, 19);
    c.truckReserve = getChoice("New truck-bay reserve (0-4): ", 0, 4);
    c.save();
    cout << "Reserves updated (they take effect immediately for new arrivals)." << endl;
}

void changePassword() {
    string p1 = readLine("Enter new password (min 6 characters): ");
    if (p1.size() < 6) { cout << "Password too short." << endl; return; }
    if (readLine("Confirm new password: ") != p1) { cout << "Passwords do not match." << endl; return; }
    Config::get().passHash = Config::hashText(p1);
    Config::get().save();
    cout << "Password changed." << endl;
}

void adminMenu(ParkingLot &lot) {
    int tries = 0;
    bool ok = false;
    while (tries < 3 && !ok) {
        ok = Config::hashText(readLine("Enter admin password: ")) == Config::get().passHash;
        if (!ok) { tries++; cout << "Wrong password." << endl; }
    }
    if (!ok) { cout << "Too many wrong tries. Going back to the main menu." << endl; return; }

    while (true) {
        cout << "\n----- ADMIN MENU -----\n1. Today's earnings\n2. Total earnings\n3. Analytics\n"
             << "4. Add / renew monthly pass\n5. Show monthly passes\n6. Change hourly rates\n"
             << "7. Change admin password\n8. Change protected reserves\n"
             << "9. Demo: exit a vehicle as if N minutes passed\n0. Back to main menu" << endl;
        int choice = getChoice("Enter your choice: ", 0, 9);
        try {
            if (choice == 1) lot.showEarnings(true);
            else if (choice == 2) lot.showEarnings(false);
            else if (choice == 3) lot.showAnalytics();
            else if (choice == 4) {
                time_t e = lot.getPasses().addOrRenew(getVehicleNumber());
                cout << "Pass valid until " << getTimeText(e) << "." << endl;
            } else if (choice == 5) lot.getPasses().show();
            else if (choice == 6) changeRates();
            else if (choice == 7) changePassword();
            else if (choice == 8) changeReserves();
            else if (choice == 9) {
                string n = getVehicleNumber();
                int mins = getChoice("Minutes since entry (0-5000): ", 0, 5000);
                cout << lot.removeVehicle(n, mins);
            } else return;
        } catch (const ParkingException &e) {
            cout << e.what() << endl;
        }
    }
}

#ifndef PARKING_NO_MAIN
int main(int argc, char **argv) {
    bool baseline = argc == 2 && string(argv[1]) == "--baseline";
    if (argc > 1 && !baseline) { cerr << "Usage: parking [--baseline]" << endl; return 1; }

    Config::get().load();
    FixedPolicy fixedPolicy;
    ReservePolicy reservePolicy;
    const AllocationPolicy &policy = baseline ? static_cast<const AllocationPolicy &>(fixedPolicy)
                                              : static_cast<const AllocationPolicy &>(reservePolicy);
    ParkingLot lot(policy, policy.name() + "_");     // separate data files per policy
    lot.loadData();

    while (true) {
        showMenu(policy);
        int choice = getChoice("Enter your choice: ", 0, 9);
        try {                                         // errors never end the session
            if (choice == 1) {
                int type = getVehicleType();
                string number = getVehicleNumber();
                string owner = getOwnerName();
                string phone = getPhone();
                lot.parkVehicle(type, number, owner, phone);
            } else if (choice == 2) cout << "\n" << lot.removeVehicle(getVehicleNumber());
            else if (choice == 3) lot.searchVehicle(getVehicleNumber());
            else if (choice == 4) lot.showSlots();
            else if (choice == 5) lot.showVehicles();
            else if (choice == 6) lot.showCount();
            else if (choice == 7) lot.showHistory();
            else if (choice == 8) adminMenu(lot);
            else if (choice == 9) runComparison();
            else { lot.saveData(); cout << "Thank you. Goodbye!" << endl; break; }
        } catch (const ParkingException &e) {
            cout << e.what() << endl;
        } catch (const exception &e) {
            cout << "Unexpected error: " << e.what() << endl;
        }
    }
    return 0;
}
#endif
