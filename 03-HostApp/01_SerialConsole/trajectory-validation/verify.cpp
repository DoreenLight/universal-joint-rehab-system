#include "quintic_candidate.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
using namespace visual_reference_candidate;
static int checks = 0;
static void require(bool value) {
    ++checks;
    if (!value) { std::cerr << "FAIL check " << checks << '\n'; std::exit(1); }
}
static bool near(double a, double b, double eps=1e-8) { return std::abs(a-b) <= eps; }
int main() {
    // Synthetic fixtures, NOT live M1/M2 parameters or verified device timing.
    const double cases[][6] = {{-30,30,2.23,2.01,2.23,2.01},
        {-12,23,1.37,0.4,2.13,0.7}, {25,-15,3,0,1,0}, {5,5,2,1,2,1}};
    std::ofstream csv("candidate_samples.csv");
    require(bool(csv));
    csv << "fixture,time,position,velocity,acceleration\n";
    for (int n=0;n<4;++n) {
        const auto &p=cases[n];
        const double period=p[2]+p[3]+p[4]+p[5];
        for (int direction=0; direction<2; ++direction) {
            double a=p[direction], b=p[1-direction], T=p[direction?4:2];
            auto start=segment(a,b,T,0), end=segment(a,b,T,T);
            require(near(start.position,a) && near(end.position,b));
            require(start.velocity==0 && end.velocity==0);
            require(start.acceleration==0 && end.acceleration==0);
            for (int j=1;j<1000;++j) {
                double t=T*j/1000, h=T*1e-5;
                auto s=segment(a,b,T,t), l=segment(a,b,T,t-h), r=segment(a,b,T,t+h);
                require(near(s.velocity,(r.position-l.position)/(2*h),1e-5));
                require(near(s.acceleration,(r.velocity-l.velocity)/(2*h),1e-5));
                require(s.velocity*(b-a)>=0);
            }
        }
        const double boundaries[]={0,p[2],p[2]+p[3],p[2]+p[3]+p[4],period};
        for(double b:boundaries) {
            auto c=cycle(p[0],p[1],p[2],p[3],p[4],p[5],b+period);
            auto l=cycle(p[0],p[1],p[2],p[3],p[4],p[5],b+period-1e-7);
            auto r=cycle(p[0],p[1],p[2],p[3],p[4],p[5],b+period+1e-7);
            require(near(l.position,r.position));
            require(near(l.velocity,r.velocity));
            require(near(l.acceleration,r.acceleration,0.001));
            require(near(c.velocity,0) && near(c.acceleration,0));
        }
        for(int j=0;j<=5000;++j) {
            double t=5*period*j/5000;
            auto s=cycle(p[0],p[1],p[2],p[3],p[4],p[5],t);
            require(s.position>=std::min(p[0],p[1])-1e-9 && s.position<=std::max(p[0],p[1])+1e-9);
            double phase=std::fmod(t,period);
            if(phase>=p[2] && phase<p[2]+p[3]) require(near(s.position,p[1]) && s.velocity==0 && s.acceleration==0);
            if(phase>=p[2]+p[3]+p[4]) require(near(s.position,p[0]) && s.velocity==0 && s.acceleration==0);
            csv << n << ',' << t << ',' << s.position << ',' << s.velocity << ',' << s.acceleration << '\n';
        }
        require(near(cycle(p[0],p[1],p[2],p[3],p[4],p[5],5*period).position,p[0]));
    }
    for(double T: {0.0,-1.0,std::numeric_limits<double>::infinity()}) {
        bool rejected=false;
        try { segment(0,1,T,0); } catch(const std::invalid_argument&) { rejected=true; }
        require(rejected);
    }
    require(segment(0,1,2,-1).position==0 && segment(0,1,2,3).position==1);
    std::cout << "PASS " << checks << " checks; 4 synthetic fixtures; NOT integrated/device-validated\n";
}
