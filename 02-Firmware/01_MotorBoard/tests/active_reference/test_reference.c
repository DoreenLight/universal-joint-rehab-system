#include "../../app/active_reference.h"
#include <stdio.h>
#include <stdlib.h>
#include <float.h>

static float cfg[2][4];
float motor1_get_actv_limit1(void) { return cfg[0][0]; }
float motor1_get_actv_limit2(void) { return cfg[0][1]; }
float motor1_get_actv_speed(void) { return cfg[0][2]; }
float motor1_get_actv_pause(void) { return cfg[0][3]; }
float motor2_get_actv_limit1(void) { return cfg[1][0]; }
float motor2_get_actv_limit2(void) { return cfg[1][1]; }
float motor2_get_actv_speed(void) { return cfg[1][2]; }
float motor2_get_actv_pause(void) { return cfg[1][3]; }
#include "build/legacy.inc"
#include "build/wrappers.inc"

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
static void configure(unsigned m, float a, float b, float v, float p) {
    cfg[m][0]=a; cfg[m][1]=b; cfg[m][2]=v; cfg[m][3]=p;
}
static void new_tick(unsigned m, actv_rehab_t *c) {
    if(m) actv_twave_tick_m2(c); else actv_twave_tick(c);
}
static unsigned old_duration(unsigned m, float a, float b, float speed) {
    actv_rehab_t c={0};
    unsigned i;
    configure(m,a,b,speed,0);
    c.active=1; c.state=ACTV_MOVING_TO_2; c.pos=a;
    for(i=1;i<=131072u;++i) {
        if(m) legacy_tick_m2(&c); else legacy_tick(&c);
        if(c.state==ACTV_PAUSE_2) return i;
    }
    return 0;
}
static void durations(void) {
    const float cases[][3]={{-30,30,30},{-12,23,17},{25,-15,17},
        {0,45,20},{-60,60,15},{5,5,30},{0,0.005f,30},
        {-1,1,0},{-1,1,-1},{-20,35,100},{-45,45,5},{3,7,1.7f}};
    unsigned m,i;
    for(m=0;m<2;++m) for(i=0;i<sizeof(cases)/sizeof(cases[0]);++i) {
        uint8_t status;
        unsigned old=old_duration(m,cases[i][0],cases[i][1],cases[i][2]);
        unsigned now=actv_ref_duration(cases[i][0],cases[i][1],cases[i][2],&status);
        printf("M%u %.6g -> %.6g speed=%.6g old_ticks=%u new_ticks=%u status=%u\n",
               m+1,cases[i][0],cases[i][1],cases[i][2],old,now,status);
        if(old) { CHECK(status==ACTV_REF_OK); CHECK(now==old); }
        else CHECK(now==0 && status==ACTV_REF_LEGACY_CYCLE);
    }
}
static void duration_sweep(void) {
    unsigned m,i,converged=0,unavailable=0;
    for(m=0;m<2;++m) for(i=0;i<96;++i) {
        float a=-47.0f+(float)(i%19)*2.1f;
        float b=a+3.7f+(float)(i%23)*2.3f;
        float speed=1.3f+(float)(i%29)*3.1f;
        uint8_t status;
        unsigned old,now;
        if(i%2) { float swap=a; a=b; b=swap; }
        old=old_duration(m,a,b,speed);
        now=actv_ref_duration(a,b,speed,&status);
        if(old) { CHECK(now==old && status==ACTV_REF_OK); ++converged; }
        else { CHECK(now==0 && status==ACTV_REF_LEGACY_CYCLE); ++unavailable; }
    }
    printf("Sweep: %u legacy-convergent timings preserved; %u proven cycles unavailable, no duration invented (both channels)\n",converged,unavailable);
}
static void trace_old_cycle(void) {
    actv_rehab_t c={0}; unsigned i;
    FILE *f=fopen("old-stall-trace.csv","w"); CHECK(f!=NULL);
    configure(0,-12,23,17,0.4f);
    c.active=1; c.state=ACTV_MOVING_TO_2; c.pos=-12;
    fprintf(f,"tick,position,velocity,error,state\n");
    for(i=1;i<=260;++i) {
        legacy_tick(&c);
        if(i>=200) fprintf(f,"%u,%.9g,%.9g,%.9g,%u\n",i,c.pos,c.vel,23-c.pos,c.state);
    }
    fclose(f);
}
static void cycle_test(unsigned m, float a, float b, float speed, float pause, FILE *csv) {
    actv_rehab_t c={0}, legacy={0}; uint16_t pause_ticks;
    unsigned forward=old_duration(m,a,b,speed);
    unsigned backward=old_duration(m,b,a,speed);
    unsigned tick, transitions=0,last=0,period;
    CHECK(forward>0 && backward>0);
    CHECK(actv_ref_pause(pause,&pause_ticks));
    period=forward+backward+2u*(pause_ticks+1u);
    configure(m,a,b,speed,pause); actv_ref_reset(&c,a,pause);
    legacy.active=1; legacy.state=ACTV_PAUSE_1; legacy.pos=a;
    legacy.pause_ticks=(uint16_t)(pause*100.0f);
    for(tick=1;tick<=5*period;++tick) {
        unsigned prev=c.state;
        float previous=c.pos;
        new_tick(m,&c);
        if(m) legacy_tick_m2(&legacy); else legacy_tick(&legacy);
        CHECK(c.state==legacy.state && c.pause_ticks==legacy.pause_ticks);
        CHECK(c.status==ACTV_REF_OK);
        if(c.state==ACTV_PAUSE_1 || c.state==ACTV_PAUSE_2) CHECK(c.pos==legacy.pos);
        CHECK(isfinite(c.pos));
        CHECK(c.pos>=fminf(a,b)-1e-5f && c.pos<=fmaxf(a,b)+1e-5f);
        if(prev==ACTV_PAUSE_1 || prev==ACTV_PAUSE_2) CHECK(c.pos==previous && c.vel==0);
        if(prev==ACTV_MOVING_TO_2) CHECK((c.pos-previous)*(b-a)>=-1e-5f);
        if(prev==ACTV_MOVING_TO_1) CHECK((c.pos-previous)*(a-b)>=-1e-5f);
        if(prev!=c.state) {
            unsigned expected=(prev==ACTV_PAUSE_1 || prev==ACTV_PAUSE_2)?pause_ticks+1u:
                (prev==ACTV_MOVING_TO_2?forward:backward);
            CHECK(tick-last==expected); CHECK(c.vel==0);
            last=tick; ++transitions;
        }
        if(tick%5==0) fprintf(csv,"%u,%u,%.9g,%.9g,%u\n",m+1,tick,c.pos,c.vel,c.state);
    }
    CHECK(transitions==20 && c.state==ACTV_PAUSE_1 && c.pos==a);
    printf("M%u 5 cycles: %u ticks, %.2f nominal seconds; forward=%u backward=%u pause=%u+1\n",
           m+1,5*period,0.01*5*period,forward,backward,pause_ticks);
}
static void math_test(void) {
    const unsigned durations[]={1,2,17,223,65536};
    unsigned n,i; float p,v;
    for(n=0;n<5;++n) {
        unsigned T=durations[n]; float last=-12;
        for(i=0;i<=T;++i) {
            double u=(double)i/T, s=u*u*u*(10+u*(-15+6*u));
            double expected=-12+35*s;
            actv_ref_sample(-12,23,i,T,&p,&v);
            CHECK(fabs(p-expected)<1e-5);
            CHECK(fabs(v-35.0/T*30*u*u*(1-u)*(1-u))<1e-5);
            CHECK(p>=last-1e-5f); last=p;
            if(i==0 || i==T) CHECK(v==0);
        }
        actv_ref_sample(-12,23,T+1,T,&p,&v); CHECK(p==23 && v==0);
        /* Independent endpoint derivatives of the polynomial: both zero. */
        for(i=0;i<2;++i) {
            double u=i, dv=30*u*u-60*u*u*u+30*u*u*u*u;
            double da=60*u-180*u*u+120*u*u*u;
            CHECK(dv==0 && da==0);
        }
    }
    actv_ref_sample(2,3,1,0,&p,&v); CHECK(p==2 && v==0);
}
static void dynamic_and_invalid(void) {
    actv_rehab_t c={0}, frozen; unsigned i; uint8_t status; uint16_t ticks;
    configure(0,-30,30,30,0); actv_ref_reset(&c,-30,0); new_tick(0,&c);
    for(i=0;i<50;++i) new_tick(0,&c);
    frozen=c;
    configure(0,-10,20,17,0);
    while(c.state==ACTV_MOVING_TO_2) {
        new_tick(0,&c); actv_ref_tick(&frozen,-30,30,30,0);
        CHECK(c.pos==frozen.pos && c.vel==frozen.vel);
    }
    CHECK(c.pos==30); new_tick(0,&c); CHECK(c.segment_to==-10);
    CHECK(c.pos==30 && c.vel==0);
    c.active=0; frozen=c; new_tick(0,&c); CHECK(memcmp(&c,&frozen,sizeof(c))==0);
    actv_ref_reset(&c,-12,0.4f); CHECK(c.pos==-12 && c.segment_tick==0 && c.pause_ticks==40);
    CHECK(!actv_ref_pause(-1,&ticks)); CHECK(!actv_ref_pause(NAN,&ticks));
    CHECK(!actv_ref_pause(655.36f,&ticks)); CHECK(actv_ref_pause(0,&ticks) && ticks==0);
    CHECK(actv_ref_duration(0,1,NAN,&status)==0 && status==ACTV_REF_INVALID);
    CHECK(actv_ref_duration(0,INFINITY,30,&status)==0 && status==ACTV_REF_INVALID);
    CHECK(actv_ref_duration(0,1000,0,&status)==0 && status==ACTV_REF_TIMING_LIMIT);
    actv_ref_reset(&c,5,0); actv_ref_tick(&c,5,10,NAN,0);
    CHECK(c.pos==5 && c.status==ACTV_REF_INVALID);
    actv_ref_tick(&c,5,10,30,0); CHECK(c.state==ACTV_MOVING_TO_2);
    /* Motor2 changes cannot leak into Motor1's independent state or getters. */
    configure(0,-30,30,30,0); configure(1,-15,25,17,0);
    actv_ref_reset(&c,-30,0); actv_ref_reset(&frozen,-15,0);
    new_tick(0,&c); new_tick(1,&frozen);
    CHECK(c.segment_to==30 && frozen.segment_to==25);
    CHECK(c.segment_ticks!=frozen.segment_ticks);
}
static void unavailable_timing(void) {
    unsigned m,i;
    for(m=0;m<2;++m) {
        actv_rehab_t c={0}, saved, legacy={0};
        float positions[4],velocities[4];
        configure(m,-12,23,17,0.4f);
        legacy.active=1; legacy.state=ACTV_MOVING_TO_2; legacy.pos=-12;
        /* Verify the actual old functions repeat a complete four-step state,
         * independently of the production cycle detector. */
        for(i=0;i<268;++i) {
            if(m) legacy_tick_m2(&legacy); else legacy_tick(&legacy);
            if(i>=260 && i<264) {
                positions[i-260]=legacy.pos; velocities[i-260]=legacy.vel;
            }
            if(i>=264) {
                CHECK(legacy.pos==positions[i-264] && legacy.vel==velocities[i-264]);
                CHECK(legacy.state==ACTV_MOVING_TO_2 && fabsf(legacy.pos-23)>0.01f);
            }
        }
        actv_ref_reset(&c,-12,0.4f);
        for(i=0;i<41;++i) new_tick(m,&c);
        CHECK(c.status==ACTV_REF_LEGACY_CYCLE && c.state==ACTV_PAUSE_1);
        CHECK(c.pos==-12 && c.vel==0 && c.segment_ticks==0);
        saved=c;
        for(i=0;i<1000;++i) new_tick(m,&c);
        CHECK(memcmp(&c,&saved,sizeof(c))==0); /* failure stays cached */
        configure(m,-12,-12,17,0.4f); new_tick(m,&c);
        CHECK(c.status==ACTV_REF_OK && c.state==ACTV_MOVING_TO_2);
        actv_ref_reset(&c,0,0); configure(m,0,1000,0,0); new_tick(m,&c);
        CHECK(c.status==ACTV_REF_TIMING_LIMIT && c.pos==0 && c.segment_ticks==0);
        saved=c; new_tick(m,&c); CHECK(memcmp(&c,&saved,sizeof(c))==0);
    }
}
int main(void) {
    FILE *csv=fopen("firmware-samples.csv","w"); CHECK(csv!=NULL);
    fprintf(csv,"motor,tick,reference_deg,velocity_deg_per_tick,state\n");
    trace_old_cycle(); durations(); duration_sweep(); math_test(); dynamic_and_invalid();
    unavailable_timing();
    cycle_test(0,-30,30,30,2,csv); cycle_test(1,-30,30,30,2,csv);
    cycle_test(0,-15,25,17,0.4f,csv); cycle_test(1,25,-15,17,0,csv);
    cycle_test(1,0,0.005f,30,0.015f,csv);
    cycle_test(0,5,5,30,0,csv);
    fclose(csv);
    printf("PASS %u checks; production C header + extracted production wrappers\n",checks);
    return 0;
}
