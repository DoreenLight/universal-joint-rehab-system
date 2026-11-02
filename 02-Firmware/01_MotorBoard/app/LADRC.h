#ifndef		 	_LADRC_H
#define			_LADRC_H

typedef struct{
	double r;
    double x_in;
    double x1_out;
	double x1_out_last;
    double x2_out;
	double x2_out_last;
    double dx1;
    double dx2;
	double dt;
}LTD, *P_LTD;

typedef struct{
	double u;
	double y;
	double dt;
//	double b0;
	double w0;
	double l1;
	double l2;
	double l3;
	double error;	
	double x1_hat;
	double x2_hat;
	double x3_hat;
	double x1_hat_last;
	double x2_hat_last;
	double x3_hat_last;
	double dx1_hat;
	double dx2_hat;
	double dx3_hat;
	
}LESO, *P_LESO;

typedef struct{
	
	double u0;
	double wc;
	double Kp;
	double Kd;
	double error1;
	double error2;
	double x1;
	double x2;
	double y1;
	double y2;

}LSEF, *P_LSEF;

typedef struct {
	LTD ltd;
	LESO leso;
	LSEF lsef;
	double b0;
	double u0;
	double u;
	double u_last;
}LADRC, *P_LADRC;

//void LADRC_Init(LTD *ltd, LESO *leso, LSEF *lsef, double r, double w0, double wc);
void LADRC_Init(LADRC *ladrc, double r, double w0, double wc, double b0);
void LADRC_Update_canshu(LADRC *ladrc, double r, double w0, double wc, double b0);
void LADRC_Cal(LADRC *ladrc, double setpoint, double measured_value, double dt);

void LADRC_LTD_Init(LADRC *ladrc);
void LADRC_LTD_Update_canshu(LADRC *ladrc, double r);
void LADRC_LTD_Cal(LADRC *ladrc, double x_in, double dt);

void LADRC_LESO_Init(LADRC *ladrc);
void LADRC_LESO_Update_canshu(LADRC *ladrc, double w0);
void LADRC_LESO_Cal(LADRC *ladrc, double u, double y, double dt);

void LADRC_LSEF_Update_canshu(LADRC *ladrc, double wc);
void LADRC_LSEF_Cal(LADRC *ladrc);


//void LADRC_LTD_Init(LTD *ltd);
//void LADRC_LTD_Update_canshu(LTD *ltd, double r);
//void LADRC_LTD_Cal(LTD *ltd, double x_in, double dt);

//void LADRC_LESO_Init(LESO *leso);
//void LADRC_LESO_Update_canshu(LESO *leso, double w0, double b0);
//void LADRC_LESO_Cal(LESO *leso, double u, double y, double dt);

//void LADRC_LSEF_Update_canshu(LSEF *lsef, double wc);
//void LADRC_LSEF_Cal(LSEF *lsef, double x1, double x2, double y1, double y2);

#endif
