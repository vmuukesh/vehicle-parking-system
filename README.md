# Vehicle Parking Management System

This is my C++ project for OOPs C++ . It is a console program that manages a small parking lot: it gives a slot to each vehicle, notes the entry time, and makes a bill when the vehicle leaves.

**Name:** V.MUKESH
**Reg. No:** AP2511011021
**University:** SRM University AP

## What it can do

- Park a vehicle (bike, car or truck) and give it a free slot of its own type
- Remove a vehicle and print the bill
- Search a vehicle by its number
- Show all slots, the parked vehicles, and how many slots are used
- Remember parked vehicles after the program is closed (saved in a text file)
- Keep a history of all finished parkings in `history.csv`

## How the bill is calculated

Every hour that has started is charged, and the minimum is 1 hour. So 1 hour 10 minutes counts as 2 hours.

| Vehicle | Rate per hour |
|---------|---------------|
| Bike    | Rs. 10        |
| Car     | Rs. 30        |
| Truck   | Rs. 60        |

The lot has 10 bike slots, 20 car slots and 5 truck slots. These numbers can be changed in `main()`.

## How to run

You need a C++ compiler like g++.

```
g++ -std=c++17 -o parking main.cpp
./parking
```

On Windows, run `parking.exe` instead.

