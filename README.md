# ParkReserve - Vehicle Parking Management System (C++17)

CSE 202 - Object Oriented Programming with C++, SRM University AP.

**Headline contribution:** an explainable *native-first, protected-overflow* allocation policy
(Strategy pattern) compared head-to-head with the classic exact-type rule by a built-in simulator,
inside a complete, tested parking system (billing engine, expiring passes, admin console, analytics).

## Build, run, test
```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic main.cpp -o parking
./parking              # reserve policy (proposed)      | Windows: parking.exe
./parking --baseline   # exact-type policy (classic rule)

g++ -std=c++17 -Wall -Wextra -Wpedantic tests.cpp -o parking_tests
./parking_tests        # run in a scratch folder; prints "42/42 checks passed."
```
Default admin password: `parking123` (changeable in the admin menu).

## Allocation policy (the novelty)
- Lot: 10 bike bays (1-10), 20 car bays (11-30), 5 truck bays (31-35).
- Always use the lowest-numbered free **native** bay.
- If none: a bike may use a car bay only if free car bays > 2; a car may use a truck bay only if free truck bays > 1.
- One size upgrade only: bikes never skip to truck bays; trucks never overflow.
- Billing follows the **vehicle**, not the borrowed bay.
- Every decision is explained: `overflow into Car bay; free after=18; reserve=2`.
- Reserves are editable in Admin > Change protected reserves.

Main menu option 9 runs the **policy comparison** (arrival order bikes, cars, trucks, no departures):

| Scenario (B/C/T) | Fixed | Reserve | Reserve mix (B/C/T) |
|---|---|---|---|
| A 16/2/1 | 13 | 19 | 16/2/1 |
| B 10/20/5 | 35 | 35 | 10/20/5 |
| C 30/20/5 | 35 | 35 | 28/6/1 (fixed: 10/20/5) |
| D 12/3/1 | 14 | 16 | 12/3/1 |

The policy helps when spare larger bays exist (A, D). In C total admissions do not improve and later
cars and trucks lose access. It is **not** claimed optimal; these are synthetic experiments, not SRM AP measurements.

## Other features
- Abstract `Vehicle` -> `Bike`/`Car`/`Truck` (polymorphic rate and surcharge), factory, `unique_ptr` ownership
- Fee engine: 15-min grace, peak 1.5x (17:00-21:00), daily cap (8 h), truck surcharge Rs. 20, pass discount 50%
- Monthly passes with expiry and renewal; receipts saved as `receipt_<number>.txt`
- Exception hierarchy (`LotFullException`, `VehicleNotFoundException`, `DuplicateVehicleException`)
- Operator overloading (`<<`, `==`), template `argMax<T>`, Singleton `Config`, hashed admin password
- Analytics: busiest hour, revenue per type, occupancy, overflow count
- Errors never end the session; corrupt or illegal saved rows are skipped on load
- Admin demo option 9: exit a vehicle "as if N minutes passed" to show billing rules instantly

## Files
`main.cpp` program | `tests.cpp` 42 checks | `test_results.txt` | `demo_output.txt`.
Runtime files are kept per policy: `<policy>_parking_data.txt`, `<policy>_history.csv`; shared: `passes.txt`, `config.txt`.

## Limitations
Single-operator console prototype; no sensors, payment gateway or concurrency control; plate check is
syntax only (Indian format); the password hash is simple (not production grade); the reserve values are
demonstration parameters, not calibrated demand estimates; a real lot must confirm which bays physically allow shared use.
Tests were built and run with GCC (`-Wall -Wextra -Wpedantic`).

## Attribution
Baseline allocation rule taken from https://github.com/vmuukesh/vehicle-parking-system (exact vehicle-type slots).
AI assistance was used in developing this submission; the authors must review, understand and be able to explain all code.
