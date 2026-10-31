#define PARKING_NO_MAIN
#include "main.cpp"
#include <functional>
int passed = 0;
void check(bool ok, const string& label) {
    if (!ok) throw runtime_error("FAIL: " + label);
    cout << "PASS " << ++passed << " " << label << '\n';
}
bool rejects(function<void()> f) {
    try { f(); } catch (const exception&) { return true; } return false;
}
int main() {
    Policy fixed; ReservePolicy reserve;
    ParkingLot a(reserve);
    check(a.park("ap-1234", 1, 0).index == 0, "native allocation");
    check(a.find("AP1234") == 0, "normalization and search");
    check(rejects([&]{a.park("AP1234",1,0);}), "duplicate rejected");
    check(rejects([&]{a.park("BAD!",1,0);}), "invalid plate rejected");
    check(rejects([&]{a.park("ABCD",4,0);}), "invalid type rejected");
    check(a.leave("ap1234",0) == 10, "minimum one hour");
    a.park("ABCD",2,0);
    check(a.leave("ABCD",3600) == 30, "exact hour");
    a.park("ABCD",2,3600);
    check(a.leave("ABCD",7201) == 60, "started hour rounded up");
    check(rejects([&]{a.leave("ABCD",7201);}), "unknown departure");
    check(rejects([&]{a.park("EFGH",1,7200);}), "backward clock");
    check(rejects([&]{a.park("EFGH",1,4102444801LL);}), "timestamp bound");
    ParkingLot b(reserve);
    for (int i=0;i<10;++i) b.park("BIKE"+to_string(i),1,0);
    auto d=b.park("NEXT",1,0);
    check(d.index==10 && d.reason.find("reserve=2")!=string::npos,
          "bike overflow with explanation");
    check(b.leave("NEXT",3601)==20, "overflow charged by vehicle type");
    for (int i=0;i<18;++i) b.park("OVER"+to_string(i),1,3601);
    check(rejects([&]{b.park("STOP",1,3601);}), "car reserve protected");
    check(b.park("CAR1",2,3601).index==28, "native may consume reserve");
    b.park("CAR2",2,3601);
    check(b.park("CAR3",2,3601).index==30, "car to truck overflow");
    for(int i=4;i<=6;++i) b.park("CAR"+to_string(i),2,3601);
    check(rejects([&]{b.park("CAR7",2,3601);}), "truck reserve protected");
    check(b.park("TRK1",3,3601).index==34, "truck consumes reserve");
    check(rejects([&]{b.park("TRK2",3,3601);}), "full lot rejection");
    ParkingLot c(fixed);
    for(int i=0;i<10;++i) c.park("BASE"+to_string(i),1,0);
    check(rejects([&]{c.park("OVER",1,0);}), "fixed policy blocks overflow");
    string f="test_journal.log"; filesystem::remove(f);
    { ParkingLot p(reserve,f); p.park("SAVE",1,100); }
    { ParkingLot p(reserve,f); check(p.find("SAVE")==0,"restart replay");
      check(p.leave("SAVE",3701)==20,"replayed billing"); }
    { ParkingLot p(reserve,f); check(p.find("SAVE")==-1,"departure replay"); }
    check(rejects([&]{ParkingLot p(fixed,f);}), "policy mismatch blocked");
    { ofstream out(f,ios::app); out << "BROKEN\n"; }
    check(rejects([&]{ParkingLot p(reserve,f);}), "corrupt journal blocked");
    filesystem::remove(f);
    { ofstream out(f); out << "PARK1 reserve\nP SAVE 1 100\nP SAVE 1 100\n"; }
    check(rejects([&]{ParkingLot p(reserve,f);}), "duplicate journal event");
    filesystem::remove(f);
    check(rejects([&]{ParkingLot p(reserve,"missing_dir/data.log");}),
          "unwritable path rejected");
    cout << "TOTAL " << passed << " tests passed\n";
    cout << "SCENARIO,policy,accepted,bikes,cars,trucks\n";
    const int demand[3][3]={{16,2,1},{10,20,5},{30,20,5}};
    for(int k=0;k<3;++k) for(int mode=0;mode<2;++mode) {
        const Policy& pol=mode ? static_cast<const Policy&>(reserve) : fixed;
        ParkingLot p(pol); int ok[3]={0,0,0};
        for(int t=1;t<=3;++t) for(int i=0;i<demand[k][t-1];++i) {
            try { p.park("T"+to_string(t)+"X"+to_string(i),t,0); ++ok[t-1]; }
            catch(const exception&) {}
        }
        cout<<char('A'+k)<<","<<pol.name()<<","<<ok[0]+ok[1]+ok[2]
            <<","<<ok[0]<<","<<ok[1]<<","<<ok[2]<<'\n';
    }
}
