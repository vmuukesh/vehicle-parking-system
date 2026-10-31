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

## How the code is organised

- `Vehicle` stores the number plate, type and entry time.
- `Slot` is one parking space. It knows if it is free or occupied.
- `ParkingLot` holds all the slots and does the parking, billing, searching and saving to file.
- `main()` shows the menu and checks the user's input.

## What I Learned

Working on this Vehicle Parking Management System was a great learning experience for me. It helped me understand how C++ concepts can be used to solve a real-world problem.

While developing this project, I learned how to use classes and objects to organize a program, structures and vectors to manage vehicle details, and functions to perform different operations. I also gained practical experience with file handling, which helped me store parking records and access them even after restarting the program.

One of the interesting parts was implementing automatic parking slot allocation and calculating parking fees based on the time a vehicle was parked. It helped me improve my logical thinking and understand how different parts of a program work together.

Apart from programming, I also got my first hands-on experience with GitHub. I learned how to create a repository, upload source code, organize project files, and share my work online.

Overall, this project helped me move beyond just learning programming concepts from textbooks and gave me a better understanding of how to build a practical application using C++.

## What I want to add later
I see this initial version as a baseline, with plenty of room to grow as my skills mature.

High on my list is replacing the console view with a proper Graphical User Interface (GUI) to make the experience seamless. I also plan to implement flexible pricing, so cars, bikes, and other vehicles aren't all charged the exact same rate.

For security and stability, I want to add an admin authentication layer and hook the system up to a relational database. That way, vehicle logs, parking history, and payments remain safe and persistent.

Longer term, I'd like to introduce real-time slot tracking, automated revenue reporting, and digital payment integration.

I know these enhancements will require extra effort, but I'm looking forward to tackling them one by one. The main goal is to bring this project as close as possible to how real-world parking operations actually work.
