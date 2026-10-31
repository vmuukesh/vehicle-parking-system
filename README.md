 Vehicle Parking Management System

What it can do

- Parking a vehicle (bike, car or truck) and give it a free slot of its own type
- Removing a vehicle and print the bill
- Searching a vehicle by its number
- Showing all the slots, the parked vehicles, and how many slots are used
- Remember parked vehicles after the program is closed (saved in a text file)
- Keep a history of all finished parkings in `history.csv`

How the bill is calculated

Every hour that has started is charged, and the minimum is 1 hour. So 1 hour 10 minutes counts as 2 hours.

| Vehicle | Rate per hour |
|---------|---------------|
| Bike    | Rs. 10        |
| Car     | Rs. 30        |
| Truck   | Rs. 60        |

The lot has 10 bike slots, 20 car slots and 5 truck slots. These numbers can be changed in `main()`.

## How to run

You need a C++ compiler that supports C++11 or later.

Compile the program:

```bash
g++ -std=c++17 main.cpp -o parking
```

Run on Linux or macOS:

```bash
./parking
```

On Windows, run parking.exe.

