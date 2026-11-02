#include "LADRC.h"
#include <math.h>

//void LADRC_Init(LTD *ltd, LESO *leso, LSEF *lsef, double r, double w0, double wc) 
//{
//	LADRC_LTD_Init(ltd);
//	LADRC_LESO_Init(leso);
//	
//	LADRC_LTD_Update_canshu(ltd, 0.0);
//	LADRC_LESO_Update_canshu(leso, 0.0, 0.001);
//	LADRC_LSEF_Update_canshu(lsef, 0.0);
//}

void LADRC_Init(LADRC *ladrc, double r, double w0, double wc, double b0) 
{
	
	LADRC_LTD_Init(ladrc);
	LADRC_LESO_Init(ladrc);
	
	LADRC_LTD_Update_canshu(ladrc, r);
	LADRC_LESO_Update_canshu(ladrc, w0);
	LADRC_LSEF_Update_canshu(ladrc, wc);
	
	ladrc->b0 = b0;
	if (ladrc->b0 <= 0.0) ladrc->b0 = 0.001;
}

void LADRC_Update_canshu(LADRC *ladrc, double r, double w0, double wc, double b0)
{
	ladrc->b0 = b0;
	
	if (ladrc->b0 <= 0.0) ladrc->b0 = 0.001;
	
	LADRC_LTD_Update_canshu(ladrc, r);
	LADRC_LESO_Update_canshu(ladrc, w0);
	LADRC_LSEF_Update_canshu(ladrc, wc);	
}

void LADRC_Cal(LADRC *ladrc, double setpoint, double measured_value, double dt)
{
//	LADRC_LTD_Cal(ladrc, setpoint, dt);
//=====================================	
	ladrc->ltd.x_in = setpoint;
	ladrc->ltd.dx2 = -2.0 * ladrc->ltd.r * ladrc->ltd.x2_out_last - ladrc->ltd.r * ladrc->ltd.r * (ladrc->ltd.x1_out_last - ladrc->ltd.x_in);
	ladrc->ltd.x2_out = ladrc->ltd.x2_out_last + ladrc->ltd.dx2 * dt;
	ladrc->ltd.dx1 = ladrc->ltd.x2_out;
	ladrc->ltd.x1_out = ladrc->ltd.x1_out_last + ladrc->ltd.dx1 * dt;
	ladrc->ltd.x1_out_last = ladrc->ltd.x1_out;
	ladrc->ltd.x2_out_last = ladrc->ltd.x2_out;
	
//	LADRC_LESO_Cal(ladrc, ladrc->u_last * ladrc->b0, measured_value, dt);
//=====================================	
	ladrc->leso.u = ladrc->u_last;
	ladrc->leso.y = measured_value;
	ladrc->leso.dt = 0.01;
	
//	ladrc->leso.error = ladrc->leso.y - ladrc->leso.x1_hat_last;
	
//	ladrc->leso.error = ladrc->leso.y - ladrc->leso.x1_hat_last;

	ladrc->leso.error = ladrc->leso.x1_hat_last - ladrc->leso.y;	
	
	ladrc->leso.dx1_hat = ladrc->leso.x2_hat_last - ladrc->leso.l1 * ladrc->leso.error;
	ladrc->leso.dx2_hat = ladrc->leso.x3_hat_last - ladrc->leso.l2 * ladrc->leso.error + ladrc->b0 * ladrc->u_last;
	ladrc->leso.dx3_hat = -1.0 * ladrc->leso.l3 * ladrc->leso.error;
	
	ladrc->leso.x1_hat = ladrc->leso.x1_hat_last + ladrc->leso.dt * ladrc->leso.dx1_hat;
	ladrc->leso.x2_hat = ladrc->leso.x2_hat_last + ladrc->leso.dt * ladrc->leso.dx2_hat;
	ladrc->leso.x3_hat = ladrc->leso.x3_hat_last + ladrc->leso.dt * ladrc->leso.dx3_hat;
	
	ladrc->leso.x1_hat_last = ladrc->leso.x1_hat;
	ladrc->leso.x2_hat_last = ladrc->leso.x2_hat;
	ladrc->leso.x3_hat_last = ladrc->leso.x3_hat;
	
//	LADRC_LSEF_Cal(ladrc);
//=====================================
	ladrc->lsef.x1 = ladrc->ltd.x1_out;
	ladrc->lsef.x2 = ladrc->ltd.x2_out;
	ladrc->lsef.y1 = ladrc->leso.x1_hat;
	ladrc->lsef.y2 = ladrc->leso.x2_hat;
	
	ladrc->lsef.error1 = ladrc->lsef.x1 - ladrc->lsef.y1;
	ladrc->lsef.error2 = ladrc->lsef.x2 - ladrc->lsef.y2;
	ladrc->lsef.u0 = ladrc->lsef.Kp * ladrc->lsef.error1 + ladrc->lsef.Kd * ladrc->lsef.error2;

//=====================================	

	ladrc->u0 = ladrc->lsef.u0;
	ladrc->u = (ladrc->u0 - ladrc->leso. x3_hat) / ladrc->b0;
	ladrc->u_last = ladrc->u;
	
}

void LADRC_LTD_Init(LADRC *ladrc)
{
	ladrc->ltd.x1_out_last = 0.0;
	ladrc->ltd.x2_out_last = 0.0;
}

void LADRC_LTD_Update_canshu(LADRC *ladrc, double r)
{
	ladrc->ltd.r = r;
}

void LADRC_LTD_Cal(LADRC *ladrc, double x_in, double dt)
{
	ladrc->ltd.x_in = x_in;
    ladrc->ltd.dx2 = -2.0 * ladrc->ltd.r * ladrc->ltd.x2_out_last - ladrc->ltd.r * ladrc->ltd.r * (ladrc->ltd.x1_out_last - ladrc->ltd.x_in);
	ladrc->ltd.x2_out = ladrc->ltd.x2_out_last + ladrc->ltd.dx2 * dt;
	ladrc->ltd.dx1 = ladrc->ltd.x2_out;
	ladrc->ltd.x1_out = ladrc->ltd.x1_out_last + ladrc->ltd.dx1 * dt;
	ladrc->ltd.x1_out_last = ladrc->ltd.x1_out;
	ladrc->ltd.x2_out_last = ladrc->ltd.x2_out;
}

void LADRC_LESO_Init(LADRC *ladrc)
{
	ladrc->leso.x1_hat_last = 0.0;
	ladrc->leso.x2_hat_last = 0.0;
	ladrc->leso.x3_hat_last = 0.0;
}

void LADRC_LESO_Update_canshu(LADRC *ladrc, double w0)
{
	ladrc->leso.w0 = w0;
	ladrc->leso.l1 = 3.0 * ladrc->leso.w0;
	ladrc->leso.l2 = ladrc->leso.l1 * ladrc->leso.w0;
	ladrc->leso.l3 = powl(ladrc->leso.w0, 3);
//	ladrc->leso.b0 = b0;
}

void LADRC_LESO_Cal(LADRC *ladrc, double u, double y, double dt)
{
	ladrc->leso.u = u;
	ladrc->leso.y = y;
	ladrc->leso.dt = dt;
//	leso->b0 = b0; 

	ladrc->leso.error = ladrc->leso.y - ladrc->leso.x1_hat;
	ladrc->leso.dx1_hat = ladrc->leso.x2_hat_last + ladrc->leso.l1 * ladrc->leso.error;
	ladrc->leso.dx2_hat = ladrc->leso.x3_hat_last + ladrc->leso.l2 * ladrc->leso.error + ladrc->b0 * ladrc->u_last;
	ladrc->leso.dx3_hat = ladrc->leso.l3 * ladrc->leso.error;
	
	ladrc->leso.x1_hat = ladrc->leso.x1_hat_last + ladrc->leso.dt * ladrc->leso.dx1_hat;
	ladrc->leso.x2_hat = ladrc->leso.x2_hat_last + ladrc->leso.dt * ladrc->leso.dx2_hat;
	ladrc->leso.x3_hat = ladrc->leso.x3_hat_last + ladrc->leso.dt * ladrc->leso.dx3_hat;
	
	ladrc->leso.x1_hat_last = ladrc->leso.x1_hat;
	ladrc->leso.x2_hat_last = ladrc->leso.x2_hat;
	ladrc->leso.x3_hat_last = ladrc->leso.x3_hat;
}

void LADRC_LSEF_Update_canshu(LADRC *ladrc, double wc)
{
	ladrc->lsef.wc = wc;
	ladrc->lsef.Kp = ladrc->lsef.wc * ladrc->lsef.wc;
	ladrc->lsef.Kd = 2.0 * ladrc->lsef.wc;
}

void LADRC_LSEF_Cal(LADRC *ladrc)
{
	ladrc->lsef.x1 = ladrc->ltd.x1_out;
	ladrc->lsef.x2 = ladrc->ltd.x2_out;
	ladrc->lsef.y1 = ladrc->leso.x1_hat;
	ladrc->lsef.y2 = ladrc->leso.x2_hat;
	
	ladrc->lsef.error1 = ladrc->lsef.x1 - ladrc->lsef.y1;
	ladrc->lsef.error2 = ladrc->lsef.x2 - ladrc->lsef.y2;
	ladrc->lsef.u0 = ladrc->lsef.Kp * ladrc->lsef.error1 + ladrc->lsef.Kd * ladrc->lsef.error2;
}

//void LADRC_Update(LADRC *ladrc, double setpoint, double measured_value, double dt) 
//{
//    // Calculate error and its derivative
//    ladrc->e = setpoint - measured_value;
//    ladrc->edot = (ladrc->e - ladrc->e_last) / dt;

//    // Update the extended state observer
//    ladrc->zdot = -ladrc->beta * ladrc->z - ladrc->Kp * ladrc->e - ladrc->Kd * ladrc->edot;
//    ladrc->z += ladrc->zdot * dt;

//    // Calculate the control input
//    ladrc->u = ladrc->Kp * ladrc->e + ladrc->Ki * ladrc->e * dt + ladrc->Kd * ladrc->edot + ladrc->z;

//    // Update the last values for the next iteration
//    ladrc->u_last = ladrc->u;
//    ladrc->e_last = ladrc->e;
//    ladrc->edot_last = ladrc->edot;
//}

//void LADRC_LTD_Init(LTD *ltd)
//{
//	ltd->x1_out_last = 0.0;
//	ltd->x2_out_last = 0.0;
//}

//void LADRC_LTD_Update_canshu(LTD *ltd, double r)
//{
//	ltd->r = r;
//}

//void LADRC_LTD_Cal(LTD *ltd, double x_in, double dt)
//{
//	ltd->x_in = x_in;
//    ltd->dx2 = -2.0 * ltd->r * ltd->x2_out_last - ltd->r * ltd->r * (ltd->x1_out_last - ltd->x_in);
//	ltd->x2_out = ltd->x2_out_last + ltd->dx2 * dt;
//	ltd->dx1 = ltd->x2_out;
//	ltd->x1_out = ltd->x1_out_last + ltd->dx1 * dt;
//	ltd->x1_out_last = ltd->x1_out;
//	ltd->x2_out_last = ltd->x2_out;
//}

//void LADRC_LESO_Init(LESO *leso)
//{
//	leso->x1_hat_last = 0.0;
//	leso->x2_hat_last = 0.0;
//	leso->x3_hat_last = 0.0;
//}

//void LADRC_LESO_Update_canshu(LESO *leso, double w0, double b0)
//{
//	leso->l1 = 3.0 * w0;
//	leso->l2 = leso->l1 * w0;
//	leso->l3 = powl(w0, 3);
//	
//	leso->b0 = b0;
//}

//void LADRC_LESO_Cal(LESO *leso, double u, double y, double dt)
//{
//	leso->u = u;
//	leso->y = y;
//	leso->dt = dt;
////	leso->b0 = b0; 

//	leso->error = leso->y - leso->x1_hat;
//	leso->dx1_hat = leso->x2_hat_last + leso->l1 * leso->error;
//	leso->dx2_hat = leso->x3_hat_last + leso->l2 * leso->error + leso->b0 * leso->u;
//	leso->dx3_hat = leso->l3 * leso->error;
//	
//	leso->x1_hat = leso->x1_hat_last + leso->dt * leso->dx1_hat;
//	leso->x2_hat = leso->x2_hat_last + leso->dt * leso->dx2_hat;
//	leso->x3_hat = leso->x3_hat_last + leso->dt * leso->dx3_hat;
//	
//	leso->x1_hat_last = leso->x1_hat;
//	leso->x2_hat_last = leso->x2_hat;
//	leso->x3_hat_last = leso->x3_hat;
//}

//void LADRC_LSEF_Update_canshu(LSEF *lesf, double wc)
//{
//	lesf->Kp = wc * wc;
//	lesf->Kd = 2.0 * wc;
//}

//void LADRC_LSEF_Cal(LSEF *lsef, double x1, double x2, double y1, double y2)
//{
//	lsef->x1 = x1;
//	lsef->x2 = x2;
//	lsef->y1 = y1;
//	lsef->y2 = y2;
//	lsef->error1 = lsef->x1 - lsef->y1;
//	lsef->error2 = lsef->x2 - lsef->y2;
//	lsef->u0 = lsef->Kp * lsef->error1 + lsef->Kd * lsef->error2;
//}
