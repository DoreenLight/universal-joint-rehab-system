#include <cmath>
#include <cstdint>
#include <iostream>
enum {ACTV_PAUSE_1, ACTV_MOVING_TO_2, ACTV_PAUSE_2, ACTV_MOVING_TO_1};
struct actv_rehab_t { unsigned active; int state; float pos, vel; uint16_t pause_ticks; };
float a=-30,b=30,v=30,p=2;
float motor1_get_actv_limit1(){return a;} float motor1_get_actv_limit2(){return b;}
float motor1_get_actv_speed(){return v;} float motor1_get_actv_pause(){return p;}
float motor2_get_actv_limit1(){return a;} float motor2_get_actv_limit2(){return b;}
float motor2_get_actv_speed(){return v;} float motor2_get_actv_pause(){return p;}static void actv_twave_tick(actv_rehab_t *ctx)
{
	float limit1 = motor1_get_actv_limit1();
	float limit2 = motor1_get_actv_limit2();
	float speed  = motor1_get_actv_speed() * 0.01f;  /* deg/s -> deg/cycle */
	float accel  = 0.1f;  /* deg/cycle^2, same as passive rehab */

	switch (ctx->state)
	{
	case ACTV_PAUSE_1:
		if (ctx->pause_ticks > 0u) { ctx->pause_ticks--; return; }
		ctx->state = ACTV_MOVING_TO_2;
		ctx->vel   = 0.0f;
		break;

	case ACTV_MOVING_TO_2:
		{
			float error = limit2 - ctx->pos;
			if (fabsf(error) < 0.01f) {
				ctx->pos = limit2;
				ctx->vel = 0.0f;
				ctx->state = ACTV_PAUSE_2;
				ctx->pause_ticks = (uint16_t)(motor1_get_actv_pause() * 100.0f);
			} else {
				float dir   = (error > 0.0f) ? 1.0f : -1.0f;
				float dist  = fabsf(error);
				float v_max = speed;
				float v_decel = sqrtf(2.0f * accel * dist);
				if (v_decel < v_max) v_max = v_decel;
				if (v_max < 0.01f)   v_max = 0.01f;
				ctx->vel += accel * dir;
				if (ctx->vel >  v_max) ctx->vel =  v_max;
				if (ctx->vel < -v_max) ctx->vel = -v_max;
				ctx->pos += ctx->vel;
				if (fabsf(ctx->pos - limit2) < 0.005f) ctx->pos = limit2;
			}
		}
		break;

	case ACTV_PAUSE_2:
		if (ctx->pause_ticks > 0u) { ctx->pause_ticks--; return; }
		ctx->state = ACTV_MOVING_TO_1;
		ctx->vel   = 0.0f;
		break;

	case ACTV_MOVING_TO_1:
		{
			float error = limit1 - ctx->pos;
			if (fabsf(error) < 0.01f) {
				ctx->pos = limit1;
				ctx->vel = 0.0f;
				ctx->state = ACTV_PAUSE_1;
				ctx->pause_ticks = (uint16_t)(motor1_get_actv_pause() * 100.0f);
			} else {
				float dir   = (error > 0.0f) ? 1.0f : -1.0f;
				float dist  = fabsf(error);
				float v_max = speed;
				float v_decel = sqrtf(2.0f * accel * dist);
				if (v_decel < v_max) v_max = v_decel;
				if (v_max < 0.01f)   v_max = 0.01f;
				ctx->vel += accel * dir;
				if (ctx->vel >  v_max) ctx->vel =  v_max;
				if (ctx->vel < -v_max) ctx->vel = -v_max;
				ctx->pos += ctx->vel;
				if (fabsf(ctx->pos - limit1) < 0.005f) ctx->pos = limit1;
			}
		}
		break;
	}
}

/* --- M2 T-wave virtual trajectory tick (100Hz) --- */
static void actv_twave_tick_m2(actv_rehab_t *ctx)
{
	float limit1 = motor2_get_actv_limit1();
	float limit2 = motor2_get_actv_limit2();
	float speed  = motor2_get_actv_speed() * 0.01f;
	float accel  = 0.1f;

	switch (ctx->state)
	{
	case ACTV_PAUSE_1:
		if (ctx->pause_ticks > 0u) { ctx->pause_ticks--; return; }
		ctx->state = ACTV_MOVING_TO_2;
		ctx->vel   = 0.0f;
		break;
	case ACTV_MOVING_TO_2:
		{
			float error = limit2 - ctx->pos;
			if (fabsf(error) < 0.01f) {
				ctx->pos = limit2; ctx->vel = 0.0f;
				ctx->state = ACTV_PAUSE_2;
				ctx->pause_ticks = (uint16_t)(motor2_get_actv_pause() * 100.0f);
			} else {
				float dir = (error > 0.0f) ? 1.0f : -1.0f;
				float v_max = (speed < sqrtf(2.0f*accel*fabsf(error))) ? speed : sqrtf(2.0f*accel*fabsf(error));
				if (v_max < 0.01f) v_max = 0.01f;
				ctx->vel += accel * dir;
				if (ctx->vel > v_max) ctx->vel = v_max;
				if (ctx->vel < -v_max) ctx->vel = -v_max;
				ctx->pos += ctx->vel;
				if (fabsf(ctx->pos - limit2) < 0.005f) ctx->pos = limit2;
			}
		}
		break;
	case ACTV_PAUSE_2:
		if (ctx->pause_ticks > 0u) { ctx->pause_ticks--; return; }
		ctx->state = ACTV_MOVING_TO_1;
		ctx->vel   = 0.0f;
		break;
	case ACTV_MOVING_TO_1:
		{
			float error = limit1 - ctx->pos;
			if (fabsf(error) < 0.01f) {
				ctx->pos = limit1; ctx->vel = 0.0f;
				ctx->state = ACTV_PAUSE_1;
				ctx->pause_ticks = (uint16_t)(motor2_get_actv_pause() * 100.0f);
			} else {
				float dir = (error > 0.0f) ? 1.0f : -1.0f;
				float v_max = (speed < sqrtf(2.0f*accel*fabsf(error))) ? speed : sqrtf(2.0f*accel*fabsf(error));
				if (v_max < 0.01f) v_max = 0.01f;
				ctx->vel += accel * dir;
				if (ctx->vel > v_max) ctx->vel = v_max;
				if (ctx->vel < -v_max) ctx->vel = -v_max;
				ctx->pos += ctx->vel;
				if (fabsf(ctx->pos - limit1) < 0.005f) ctx->pos = limit1;
			}
		}
		break;
	}
}

int main(){
for(int fixture=0;fixture<2;++fixture){
 if(fixture){a=-12;b=23;v=17;p=0.4f;}
 for(int motor=1;motor<=2;++motor){
  actv_rehab_t c={1,ACTV_PAUSE_1,a,0,(uint16_t)(p*100)};
  int last=0, transitions=0;
  std::cout<<"fixture="<<fixture<<" motor="<<motor<<" limits="<<a<<","<<b<<" speed="<<v<<" pause="<<p<<"\n";
  for(int tick=1;tick<100000 && transitions<4;++tick){
   int old=c.state;
   if(motor==1) actv_twave_tick(&c); else actv_twave_tick_m2(&c);
   if(old!=c.state){ std::cout<<"state="<<old<<" ticks="<<tick-last<<" end="<<c.pos<<"\n";last=tick;++transitions; }
  }
  if(transitions!=4) { std::cout<<"INCOMPLETE after 99999 ticks; pos="<<c.pos<<" state="<<c.state<<"\n"; continue; }
  std::cout<<"cycle_ticks="<<last<<" nominal_seconds="<<last*0.01<<"\n";
 }
}
}

