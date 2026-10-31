#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <ctime>
#include <iomanip>
#include <limits>
using namespace std;

enum VehicleType { BIKE = 1, CAR = 2, TRUCK = 3 };

const string ADMIN_PASSWORD = "parking123";

int bikeRate = 10;
int carRate = 30;
int truckRate = 60;

string getTypeName(int type) {
    if (type == BIKE) return "Bike";
    if (type == CAR) return "Car";
    return "Truck";
}

int getRate(int type) {
    if (type == BIKE) return bikeRate;
    if (type == CAR) return carRate;
    return truckRate;
}

string getTimeText(time_t t) {
    char text[30];
    strftime(text, sizeof(text), "%d-%m-%Y %H:%M", localtime(&t));
    return text;
}

string showName(string name) {
    for (int i = 0; i < (int)name.size(); i++) {
        if (name[i] == '_') {
            name[i] = ' ';
        }
    }
    return name;
}

void loadRates() {
    ifstream file("rates.txt");
    if (file) {
        file >> bikeRate >> carRate >> truckRate;
    }
}

void saveRates() {
    ofstream file("rates.txt");
    file << bikeRate << " " << carRate << " " << truckRate << endl;
}

struct Vehicle {
    string number;
    int type;
    time_t entryTime;
    string owner;
    string phone;
};

class Slot {
private:
    int slotNo;
    int type;
    bool full;
    Vehicle vehicle;

public:
    Slot(int no, int t) {
        slotNo = no;
        type = t;
        full = false;
    }

    int getSlotNo() { return slotNo; }
    int getType() { return type; }
    bool isFull() { return full; }
    Vehicle getVehicle() { return vehicle; }

    void parkVehicle(Vehicle v) {
        vehicle = v;
        full = true;
    }

    Vehicle removeVehicle() {
        full = false;
        return vehicle;
    }
};

class ParkingLot {
private:
    vector<Slot> slots;
    vector<string> passes;

    int findFreeSlot(int type) {
        for (int i = 0; i < (int)slots.size(); i++) {
            if (slots[i].getType() == type && !slots[i].isFull()) {
                return i;
            }
        }
        return -1;
    }

    int findVehicle(string number) {
        for (int i = 0; i < (int)slots.size(); i++) {
            if (slots[i].isFull() && slots[i].getVehicle().number == number) {
                return i;
            }
        }
        return -1;
    }

    bool hasPass(string number) {
        for (int i = 0; i < (int)passes.size(); i++) {
            if (passes[i] == number) {
                return true;
            }
        }
        return false;
    }

    void saveHistory(Vehicle v, int slotNo, time_t exitTime, int fee) {
        ofstream file("history.txt", ios::app);
        file << v.number << " " << v.type << " " << slotNo << " "
             << v.entryTime << " " << exitTime << " " << fee << endl;
    }

    void savePasses() {
        ofstream file("passes.txt");
        for (int i = 0; i < (int)passes.size(); i++) {
            file << passes[i] << endl;
        }
    }

public:
    ParkingLot(int bikes, int cars, int trucks) {
        int no = 1;
        for (int i = 0; i < bikes; i++) {
            slots.push_back(Slot(no, BIKE));
            no++;
        }
        for (int i = 0; i < cars; i++) {
            slots.push_back(Slot(no, CAR));
            no++;
        }
        for (int i = 0; i < trucks; i++) {
            slots.push_back(Slot(no, TRUCK));
            no++;
        }
    }

    void parkVehicle(string number, int type, string owner, string phone) {
        if (findVehicle(number) != -1) {
            cout << "This vehicle is already parked." << endl;
            return;
        }

        int index = findFreeSlot(type);
        if (index == -1) {
            cout << "Sorry, no free slot for a " << getTypeName(type) << "." << endl;
            return;
        }

        Vehicle v;
        v.number = number;
        v.type = type;
        v.entryTime = time(NULL);
        v.owner = owner;
        v.phone = phone;
        slots[index].parkVehicle(v);
        saveData();

        cout << endl;
        cout << "Vehicle parked successfully." << endl;
        cout << "Vehicle number : " << number << endl;
        cout << "Vehicle type   : " << getTypeName(type) << endl;
        cout << "Owner name     : " << showName(owner) << endl;
        cout << "Slot number    : " << slots[index].getSlotNo() << endl;
        cout << "Entry time     : " << getTimeText(v.entryTime) << endl;
        cout << "Rate per hour  : Rs. " << getRate(type) << endl;
        if (hasPass(number)) {
            cout << "This vehicle has a monthly pass. You get 50% off." << endl;
        }
    }

    void removeVehicle(string number) {
        int index = findVehicle(number);
        if (index == -1) {
            cout << "This vehicle is not in the parking lot." << endl;
            return;
        }

        time_t now = time(NULL);
        Vehicle v = slots[index].removeVehicle();

        long long seconds = now - v.entryTime;
        int hours = seconds / 3600;
        if (seconds % 3600 != 0) {
            hours++;
        }
        if (hours == 0) {
            hours = 1;
        }
        int fee = hours * getRate(v.type);
        bool discount = hasPass(v.number);
        if (discount) {
            fee = fee / 2;
        }

        cout << endl;
        cout << "Vehicle removed. Here is the bill." << endl;
        cout << "Vehicle number : " << v.number << endl;
        cout << "Vehicle type   : " << getTypeName(v.type) << endl;
        cout << "Owner name     : " << showName(v.owner) << endl;
        cout << "Slot number    : " << slots[index].getSlotNo() << endl;
        cout << "Entry time     : " << getTimeText(v.entryTime) << endl;
        cout << "Exit time      : " << getTimeText(now) << endl;
        cout << "Total hours    : " << hours << endl;
        if (discount) {
            cout << "Monthly pass   : 50% off" << endl;
        }
        cout << "Total amount   : Rs. " << fee << endl;

        saveHistory(v, slots[index].getSlotNo(), now, fee);
        saveData();
    }

    void searchVehicle(string number) {
        int index = findVehicle(number);
        if (index == -1) {
            cout << "This vehicle is not in the parking lot." << endl;
            return;
        }
        Vehicle v = slots[index].getVehicle();
        cout << "Vehicle found." << endl;
        cout << "Slot number : " << slots[index].getSlotNo() << endl;
        cout << "Owner name  : " << showName(v.owner) << endl;
        cout << "Phone       : " << v.phone << endl;
        cout << "Entry time  : " << getTimeText(v.entryTime) << endl;
    }

    void showSlots() {
        cout << endl;
        cout << left << setw(8) << "Slot" << setw(8) << "Type" << "Status" << endl;
        cout << "------------------------------------" << endl;
        for (int i = 0; i < (int)slots.size(); i++) {
            cout << left << setw(8) << slots[i].getSlotNo()
                 << setw(8) << getTypeName(slots[i].getType());
            if (slots[i].isFull()) {
                cout << slots[i].getVehicle().number << endl;
            } else {
                cout << "Free" << endl;
            }
        }
    }

    void showVehicles() {
        bool found = false;
        cout << endl;
        cout << left << setw(6) << "Slot" << setw(14) << "Number"
             << setw(8) << "Type" << setw(18) << "Owner" << "Entry time" << endl;
        cout << "-----------------------------------------------------------" << endl;
        for (int i = 0; i < (int)slots.size(); i++) {
            if (slots[i].isFull()) {
                Vehicle v = slots[i].getVehicle();
                cout << left << setw(6) << slots[i].getSlotNo()
                     << setw(14) << v.number
                     << setw(8) << getTypeName(v.type)
                     << setw(18) << showName(v.owner)
                     << getTimeText(v.entryTime) << endl;
                found = true;
            }
        }
        if (!found) {
            cout << "No vehicles are parked right now." << endl;
        }
    }

    void showCount() {
        cout << endl;
        for (int type = BIKE; type <= TRUCK; type++) {
            int total = 0;
            int used = 0;
            for (int i = 0; i < (int)slots.size(); i++) {
                if (slots[i].getType() == type) {
                    total++;
                    if (slots[i].isFull()) {
                        used++;
                    }
                }
            }
            cout << getTypeName(type) << " slots used: " << used << " out of " << total << endl;
        }
    }

    void showHistory() {
        ifstream file("history.txt");
        if (!file) {
            cout << "There is no history yet." << endl;
            return;
        }

        string number;
        int type, slotNo, fee;
        long long entry, exitTime;
        bool found = false;

        cout << endl;
        cout << left << setw(15) << "Number" << setw(8) << "Type" << setw(6) << "Slot"
             << setw(19) << "Entry" << setw(19) << "Exit" << "Amount" << endl;
        cout << "----------------------------------------------------------------------" << endl;
        while (file >> number >> type >> slotNo >> entry >> exitTime >> fee) {
            cout << left << setw(15) << number << setw(8) << getTypeName(type)
                 << setw(6) << slotNo << setw(19) << getTimeText(entry)
                 << setw(19) << getTimeText(exitTime) << "Rs. " << fee << endl;
            found = true;
        }
        if (!found) {
            cout << "There is no history yet." << endl;
        }
    }

    void showEarnings(bool onlyToday) {
        ifstream file("history.txt");
        string number;
        int type, slotNo, fee;
        long long entry, exitTime;
        int count = 0;
        int total = 0;

        time_t now = time(NULL);
        tm *today = localtime(&now);
        int todayDay = today->tm_yday;
        int todayYear = today->tm_year;

        while (file >> number >> type >> slotNo >> entry >> exitTime >> fee) {
            if (onlyToday) {
                time_t t = exitTime;
                tm *day = localtime(&t);
                if (day->tm_yday != todayDay || day->tm_year != todayYear) {
                    continue;
                }
            }
            count++;
            total = total + fee;
        }

        cout << endl;
        if (onlyToday) {
            cout << "Today's report" << endl;
        } else {
            cout << "Total report" << endl;
        }
        cout << "Vehicles served : " << count << endl;
        cout << "Money collected : Rs. " << total << endl;
    }

    void addPass(string number) {
        if (hasPass(number)) {
            cout << "This vehicle already has a monthly pass." << endl;
            return;
        }
        passes.push_back(number);
        savePasses();
        cout << "Monthly pass added for " << number << "." << endl;
    }

    void showPasses() {
        if (passes.size() == 0) {
            cout << "No monthly passes have been added yet." << endl;
            return;
        }
        cout << endl;
        cout << "Vehicles with monthly pass:" << endl;
        for (int i = 0; i < (int)passes.size(); i++) {
            cout << i + 1 << ". " << passes[i] << endl;
        }
    }

    void loadPasses() {
        ifstream file("passes.txt");
        string number;
        while (file >> number) {
            passes.push_back(number);
        }
    }

    void saveData() {
        ofstream file("parking_data.txt");
        for (int i = 0; i < (int)slots.size(); i++) {
            if (slots[i].isFull()) {
                Vehicle v = slots[i].getVehicle();
                file << v.number << " " << v.type << " " << slots[i].getSlotNo() << " "
                     << v.entryTime << " " << v.owner << " " << v.phone << endl;
            }
        }
    }

    void loadData() {
        ifstream file("parking_data.txt");
        if (!file) {
            return;
        }
        string number, owner, phone;
        int type, slotNo;
        long long entry;
        while (file >> number >> type >> slotNo >> entry >> owner >> phone) {
            if (slotNo < 1 || slotNo > (int)slots.size()) {
                continue;
            }
            if (slots[slotNo - 1].getType() != type) {
                continue;
            }
            Vehicle v;
            v.number = number;
            v.type = type;
            v.entryTime = entry;
            v.owner = owner;
            v.phone = phone;
            slots[slotNo - 1].parkVehicle(v);
        }
    }
};

int getChoice(string message, int low, int high) {
    int choice;
    while (true) {
        cout << message;
        if (cin >> choice && choice >= low && choice <= high) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return choice;
        }
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Wrong input. Please enter a number from " << low << " to " << high << "." << endl;
    }
}

string getVehicleNumber() {
    string line;
    while (true) {
        cout << "Enter vehicle number (example AP39AB1234): ";
        getline(cin, line);

        string number = "";
        for (int i = 0; i < (int)line.size(); i++) {
            char c = line[i];
            if (c == ' ' || c == '-') {
                continue;
            }
            if (c >= 'a' && c <= 'z') {
                c = c - 32;
            }
            number += c;
        }

        if (number.size() >= 4 && number.size() <= 12) {
            return number;
        }
        cout << "That vehicle number is not valid. Try again." << endl;
    }
}

string getOwnerName() {
    string line;
    while (true) {
        cout << "Enter owner name: ";
        getline(cin, line);
        if (line.size() < 2 || line.size() > 20) {
            cout << "Name should be 2 to 20 letters long." << endl;
            continue;
        }
        for (int i = 0; i < (int)line.size(); i++) {
            if (line[i] == ' ') {
                line[i] = '_';
            }
        }
        return line;
    }
}

string getPhone() {
    string line;
    while (true) {
        cout << "Enter 10 digit mobile number: ";
        getline(cin, line);
        bool ok = (line.size() == 10);
        for (int i = 0; i < (int)line.size(); i++) {
            if (line[i] < '0' || line[i] > '9') {
                ok = false;
            }
        }
        if (ok) {
            return line;
        }
        cout << "Mobile number must have exactly 10 digits." << endl;
    }
}

int getVehicleType() {
    cout << "1. Bike" << endl;
    cout << "2. Car" << endl;
    cout << "3. Truck" << endl;
    return getChoice("Choose vehicle type: ", 1, 3);
}

void showMenu() {
    cout << endl;
    cout << "===== VEHICLE PARKING MANAGEMENT SYSTEM =====" << endl;
    cout << "1. Park a vehicle" << endl;
    cout << "2. Remove a vehicle and get bill" << endl;
    cout << "3. Search a vehicle" << endl;
    cout << "4. Show all slots" << endl;
    cout << "5. Show parked vehicles" << endl;
    cout << "6. Show slots used" << endl;
    cout << "7. Show parking history" << endl;
    cout << "8. Admin login" << endl;
    cout << "0. Exit" << endl;
}

void changeRates() {
    cout << "Current rates per hour: Bike Rs. " << bikeRate << ", Car Rs. " << carRate
         << ", Truck Rs. " << truckRate << endl;
    bikeRate = getChoice("Enter new bike rate: ", 1, 1000);
    carRate = getChoice("Enter new car rate: ", 1, 1000);
    truckRate = getChoice("Enter new truck rate: ", 1, 1000);
    saveRates();
    cout << "Rates updated." << endl;
}

void adminMenu(ParkingLot &lot) {
    string password;
    int tries = 0;
    while (tries < 3) {
        cout << "Enter admin password: ";
        getline(cin, password);
        if (password == ADMIN_PASSWORD) {
            break;
        }
        tries++;
        cout << "Wrong password." << endl;
    }
    if (tries == 3) {
        cout << "Too many wrong tries. Going back to the main menu." << endl;
        return;
    }

    while (true) {
        cout << endl;
        cout << "----- ADMIN MENU -----" << endl;
        cout << "1. Today's earnings" << endl;
        cout << "2. Total earnings" << endl;
        cout << "3. Add monthly pass" << endl;
        cout << "4. Show monthly pass vehicles" << endl;
        cout << "5. Change hourly rates" << endl;
        cout << "0. Back to main menu" << endl;
        int choice = getChoice("Enter your choice: ", 0, 5);

        if (choice == 1) {
            lot.showEarnings(true);
        } else if (choice == 2) {
            lot.showEarnings(false);
        } else if (choice == 3) {
            lot.addPass(getVehicleNumber());
        } else if (choice == 4) {
            lot.showPasses();
        } else if (choice == 5) {
            changeRates();
        } else {
            return;
        }
    }
}

int main() {
    ParkingLot lot(10, 20, 5);
    loadRates();
    lot.loadData();
    lot.loadPasses();

    while (true) {
        showMenu();
        int choice = getChoice("Enter your choice: ", 0, 8);

        if (choice == 1) {
            int type = getVehicleType();
            string number = getVehicleNumber();
            string owner = getOwnerName();
            string phone = getPhone();
            lot.parkVehicle(number, type, owner, phone);
        } else if (choice == 2) {
            lot.removeVehicle(getVehicleNumber());
        } else if (choice == 3) {
            lot.searchVehicle(getVehicleNumber());
        } else if (choice == 4) {
            lot.showSlots();
        } else if (choice == 5) {
            lot.showVehicles();
        } else if (choice == 6) {
            lot.showCount();
        } else if (choice == 7) {
            lot.showHistory();
        } else if (choice == 8) {
            adminMenu(lot);
        } else {
            lot.saveData();
            cout << "Thank you. Goodbye!" << endl;
            break;
        }
    }
    return 0;
}
