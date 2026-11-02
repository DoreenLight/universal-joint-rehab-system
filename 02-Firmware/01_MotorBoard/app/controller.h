#ifndef		 	_controller_H
#define			_controller_H

#include "stdint.h"
#include "LADRC.h"

#define SYS_PERIOD      0.01f		//��λ��
#define AD_BIT	12
#define DATASPACE_SIZE 2000
#define RX_BIG_ENDIAN   1

//���������Ϣ���壬9�������ͣ�5�����ͣ�
#define USART_RX_Float_No	12
#define USART_RX_UChar_No	5
#define USART_RX_SIZE    	54	//USART_RX_Float_No * 4 +  USART_RX_UChar_No + 1

#define USART_TX_SIZE    	20

////������λ���·���ָ��
//#define Order_No 		9					//??????��???????
//#define Order_NO_zijie	USART_RX_SIZE

#define UPDATA_Count 	9				//�ϴ����������ܵ�����
#define UPDATA_NO 		38				//�ϴ����ݵ����ֽ�������������ĸ$#��9 * 4 + 2

#define dt 0.01							//���Ƽ������ϴ�����Ϊ0.01s			
//#define JR_jiaodu_Init 0		//����ƽ̨��ʼ�ĽǶ�ֵ



//TIM1��TIM8TIM11��ʱ��ΪAPB2ʱ�ӵ�������168M��TIM2TIM7��TIM12~TIM14��ʱ��ΪAPB1��ʱ�ӵ�������84M��

typedef struct 
{
	uint8_t		First_flag;
	float 		value_Filted;			//�洢��һ�ε�����
	float 		Value;
	float		Value_Low;		//��Ӧ����͵�ѹֵ��Χ
	float		Value_High;
	float 		Value_AD;			//AD������������ֵ	
	float  		In_max;
	float  		In_min;
//	float			value_1[100];
}Sensor, *PSensor;

typedef struct						//�ϴ����ݵ����ͼ�ת������
{
	float In_Value;
	float In_max;
	float In_min;
	uint16_t Out_Value;
	uint16_t Out_max;
	uint16_t Out_min;
}Up_Data, *P_Up_Data;

typedef struct
{
	float kp;
	float ki;
	float kd;
	float In_val;
	float Feedback_val;
	float Now_error;
	float previous_error;
	float integral;
	float integral_max;
	float integral_min;
	float u_out;						//��1֮�����ֵ
	float val_ki;
}PID,*P_PID;

typedef struct
{
	uint16_t val;
	void (*set_val)(uint16_t);
}PWM_Set;

typedef enum {
    dingdian,
    zhengxian,
    fangbo,
    PWM_test,
    gengxin_flash
} ModeEnum;

typedef enum {
    _start = 1,
    _stop = 2
} StartStopEnum;

typedef enum {
    PWM_mode,
    Internal,
    External_and_Internal,
    External
} NeiWaiEnum;

typedef enum {
    no,
    yes
} KzqGenxinEnum;

typedef struct
{
//	uint8_t mode;
	ModeEnum mode;
	StartStopEnum start_or_stop;
	float motor1_jiaodu;
	float motor1_sudu;
	float motor1_dianliu;		//����������ٶ�ָ���λ��/s
	float motor1_kp;
	float motor1_ki;
	float motor1_beiyong1;
	float motor1_beiyong2;
	float motor2_jiaodu;
	float motor2_sudu;
	float motor2_liju;		//����������ٶ�ָ���λ��/s
	float motor2_kp;
	float motor2_kd;
	float motor2_dianliu_limit;
	
	NeiWaiEnum nei_wai_xuanze;
	KzqGenxinEnum kzq_genxin;
	uint8_t BeiYong;
}Order_mode;


typedef struct __attribute__((packed))
{
	uint8_t _Mode;
	uint8_t _Start_stop;
	uint8_t _nei_wai_select;
	uint8_t _update_para;	
	uint8_t _deviceID;	
	
	float _float_para1[USART_RX_Float_No];
}Rx_data;

//״̬��Ϊ�ȴ����ɼ����ݣ�����ָ����������㼰�����ˢ���ϴ�����
typedef enum{wait,creat_order,caiji,controller_cal,fresh_data} run_flag;

//�����ж��ϴ��������Ƿ���ʣ���ʢ�������ĳ����У��ƺ��ò��������ʽ
typedef enum{idle,work,over} data_state;

//��ʱ���Ƿ��ڹ���״̬�����ڽ���ʱ���еĹ���״̬������whileѭ����
typedef enum{work_flag = 1, stop_flag = 2} State_flag;

typedef struct
{
	data_state	state;			//0��ʾ���У�1��ʾ����д�룬2��ʾ�Ѿ�д��
	uint16_t 	count;					//��ǰ��Ҫд���ָ����ֵ
	uint8_t 	data[DATASPACE_SIZE];
}dataspace;		//��Ϊ˫���棬��������λ����������

extern unsigned char rs485_send_test[];

//extern Sensor weizhi_left,weizhi_right;
extern Sensor Sensor_jiaodu_1, Sensor_I1, Sensor_jiaodu_2;	//�ֱ�������Ƕȡ�����ֵ

extern PSensor P_Sensor_jiaodu_1, P_Sensor_I1, P_Sensor_jiaodu_2;	//�ֱ�������Ƕȡ�����ֵ

extern float g_total_angle_1;  // 电机1累计角度值
extern float g_prev_angle_1;   // 电机1上一次绝对角度值

extern Order_mode my_Order_mode;			//ʵ����Ҫ��ָ�����ģʽ����λ��ͬ�򡢷��򣩣��ٶ�

//extern Order My_Order[Order_No];			//������յ���ָ������

//extern P_Order My_P_Order[Order_No];	//������յ���ָ������ָ��

extern volatile run_flag controller_flag;		//���ƹ��̵ı�ʶ

extern volatile State_flag My_state;				//�ڶ�ʱ����while֮�佻�������ڱ�ʶÿһ�ζ�ʱ�ڵĿ��������Ƿ����

extern PID Control_P_PID,Control_jiao_PID;
extern P_PID P_Control_P_PID,P_Control_jiao_PID;

extern LADRC 	Control_P_LADRC, 	Control_jiao_LADRC;
extern P_LADRC 	P_Control_P_LADRC, 	P_Control_jiao_LADRC;

extern float yali_zhiling,jiaodu_zhiling;
extern float yali_zhiling_0,jiaodu_zhiling_0;
extern uint8_t First_time_flag;

//extern PWM_Set zhan_1,zhan_2;
extern volatile float zhan_1_temp, zhan_2_temp;
//volatile extern dataspace	dataspace_1,dataspace_2;

extern Up_Data Up_Serial_Data[(UPDATA_NO-2)/2];			//��Ҫ�ϴ������ݵ�����������һ��������20���ֽڣ�������ϴ�8������
extern P_Up_Data P_Up_Serial_Data[(UPDATA_NO-2)/2];

extern float P_siqu ,	N_siqu;

extern float Command_temp;			//20231127����������ʱ������������ʱ���¼��һ�ε�ѹ��ָ��Ӷ����

volatile extern run_flag controller_flag;

extern volatile Rx_data data_received;

extern uint8_t uart_tx_buf[43];

void Sensor_Init(void);

void dataspace_Init(void);

void Controller_Init(void);

void PID_controller(PID *N_PID);

void Sensor_Data_convert(Sensor *s_in);

void Sensor_Data_convert_4_20mA(Sensor *s_in);

void Up_Data_cal(Up_Data *Data_cal);

//����ͨ����ز���
volatile extern unsigned char Rx_Finished_flag;
volatile extern unsigned char Rx_buffer[USART_RX_SIZE];	//���ڴ洢�·���ָ����ֽ�ֵ


//�����������صĲ���

float limit(float value,float v_max,float v_min);

float limit_convert(float value,float v_max,float v_min,float out_max,float out_min);

short DA_out(int ch_no,float data_in);	//�����ͨ������1��2�������data_in �ķ�Χ�ǡ�1

float One_Order_Filter(float New_val,float Old_val,float pinlv,float zhouqi);

float Rate_limit(float New_val,float Old_val,float sudu_limit,float zhouqi);

//ͨ����غ���
void usart_callback(uint8_t ch);

float bytes_to_float(const volatile uint8_t *p);
void float_to_big_endian(uint8_t *dest, float value);


//float Sensor_Data_convte(uint16_t in_min,uint16_t in_max,float out_min,float out_max);

#endif

