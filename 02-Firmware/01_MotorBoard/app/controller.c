//#include "controller/controller.h"
//#include "dac/bsp_dac.h"
//#include "AdvancedTIM/bsp_AdvancedTIM.h"


#include "controller.h"
//#include "dac/bsp_dac.h"
//#include "AdvancedTIM/bsp_AdvancedTIM.h"		//����PWM�����õ�ͷ�ļ�
//#include "pwm.h"
#include "stdio.h"
#include "string.h"
#include <math.h>
//#include "board.h"
//dataspace	dataspace_1,dataspace_2;


Up_Data 	Up_Serial_Data[(UPDATA_NO-2)/2];
P_Up_Data 	P_Up_Serial_Data[(UPDATA_NO-2)/2];

//uint8_t up_data_test[UPDATA_NO*No_zhouqi];		//���ڲ��Դ����ϴ�,10msһ���������ڣ�5�����������ϴ�һ������
//uint8_t up_test_temp=0,up_data_cishu=0;

Order_mode my_Order_mode;			//ʵ����Ҫ��ָ�����ģʽ����λ��ͬ�򡢷��򣩣��ٶ�

volatile run_flag controller_flag = wait;		//���ƹ��̵ı�ʶ

Sensor Sensor_jiaodu_1, Sensor_I1, Sensor_jiaodu_2;	//�ֱ�������Ƕȡ�����ֵ

PSensor P_Sensor_jiaodu_1, P_Sensor_I1, P_Sensor_jiaodu_2;	//�ֱ�������Ƕȡ�����ֵ

float g_total_angle_1 = 0.0f;  // 电机1累计角度值
float g_prev_angle_1 = 0.0f;   // 电机1上一次绝对角度值

//float neibuyali[10]={0},waibuyali[10]={0};

volatile State_flag My_state = stop_flag;

PID 	Control_P_PID,		Control_jiao_PID;
P_PID 	P_Control_P_PID,	P_Control_jiao_PID;

LADRC 	Control_P_LADRC, 	Control_jiao_LADRC;
P_LADRC	P_Control_P_LADRC, 	P_Control_jiao_LADRC;

float 	yali_zhiling=0,		jiaodu_zhiling=0;
float 	yali_zhiling_0=0,	jiaodu_zhiling_0=0;

uint8_t uart_tx_buf[43];

uint8_t First_time_flag = 1;

//PWM_Set zhan_1,zhan_2;

volatile float zhan_1_temp = 0.0, zhan_2_temp = 0.0;

float Command_temp = 0.0;

//��ͨ����صĶ���

volatile unsigned char Rx_buffer[USART_RX_SIZE];	//���ڴ洢�·���ָ����ֽ�ֵ
volatile unsigned char Rx_Finished_flag = 0;				//����ָʾ�Ƿ�������

static volatile uint16_t rcv_cnt = 0;

volatile Rx_data data_received = {0};

void Sensor_Init()
{

//	
	Sensor_jiaodu_1.value_Filted = 0;	
	Sensor_jiaodu_1.First_flag = 1;
	Sensor_jiaodu_1.In_max = 360.0;						//�������������ֵ1.0MPa
	Sensor_jiaodu_1.In_min = 0.0;						//������������Сֵ0.0MPa
	Sensor_jiaodu_1.Value_High = 5;	//����ʵ�ʵ�·�����
	Sensor_jiaodu_1.Value_Low = 0;
	Sensor_jiaodu_1.Value_AD = 0;
	P_Sensor_jiaodu_1 = &Sensor_jiaodu_1;

	Sensor_I1.value_Filted = 0;
	Sensor_I1.First_flag = 1;
	Sensor_I1.In_max = 10.0;		//����λ��30mm//10MPa
	Sensor_I1.In_min = 0.0;
	Sensor_I1.Value_High = 4.0;	//20mA��Ӧ4V
	Sensor_I1.Value_Low = 0.8;	//4mA��Ӧ0.8V
	Sensor_I1.Value_AD = 0;
	P_Sensor_I1 = &Sensor_I1;

	//
	Sensor_jiaodu_2.value_Filted = 0;
	Sensor_jiaodu_2.First_flag = 1;
	Sensor_jiaodu_2.In_max = 60.0;	//60MPa//����λ��30mm
	Sensor_jiaodu_2.In_min = 0.0;
	Sensor_jiaodu_2.Value_High = 4.0;
	Sensor_jiaodu_2.Value_Low = 0.8;
	Sensor_jiaodu_2.Value_AD = 0;
	P_Sensor_jiaodu_2 = &Sensor_jiaodu_2;

}

//void Controller_Out_Set_Init()
//{
//	zhan_1.set_val = Set1_Value;
//	zhan_2.set_val = PWM_SetCompare1;	//��2��ͨ��
//}


void dataspace_Init()
{

	
	
}

void Sensor_Data_convert(Sensor *s_in)
{
	Sensor temp;
	temp=*s_in;
	
	//���������Сֵ
	temp.Value = (temp.Value_AD - temp.Value_Low) / (temp.Value_High - temp.Value_Low) * (temp.In_max - temp.In_min) + temp.In_min;	

	s_in->Value = temp.Value;	
	
	if(s_in->First_flag == 1) 
	{
		s_in->value_Filted = s_in->Value;
		s_in->First_flag = 0;
	}
	else
	{
		s_in->value_Filted = One_Order_Filter(s_in->Value,s_in->value_Filted,10.0,0.01);
	}
	
//	s_in->value_Filted = average_Filter(s_in->Value,s_in->value_1);
	
}

void Sensor_Data_convert_4_20mA(Sensor *s_in)
{
	
	Sensor temp;
	temp = *s_in;
	if (AD_BIT == 12)
	{
		//temp.Value=(double)temp.Value_AD/(4095-819)*(temp.Value_High-temp.Value_Low);			//���������Сֵ
//		temp.Value=(double)temp.Value_AD/(4095-0)*(temp.Value_High-temp.Value_Low);			//���������Сֵ
		
		temp.Value=((double)temp.Value_AD * 2.01416-400)/(2000-400)*(temp.Value_High-temp.Value_Low);			//��ʱ��ѹ����������
		
		//3300/4096/4,40-200mA	
	}
	
	
	s_in->Value=temp.Value;
	
	
	if(s_in->First_flag == 1) 
	{
		s_in->value_Filted = s_in->Value;
		s_in->First_flag = 0;
	}
	else
	{
		s_in->value_Filted = One_Order_Filter(s_in->Value,s_in->value_Filted,10.0,0.01);
	}
}

void Up_Data_cal(Up_Data *Data_cal)
{
	Up_Data temp;
	temp=*Data_cal;
	temp.Out_Value=(temp.Out_max-temp.Out_min)*(temp.In_Value-temp.In_min)/(temp.In_max-temp.In_min)+temp.Out_min;
	Data_cal->Out_Value=temp.Out_Value;
}


float limit(float value,float v_max,float v_min)
{

    float real_max = (v_max > v_min) ? v_max : v_min;
    float real_min = (v_max > v_min) ? v_min : v_max;
    
    return (value > real_max) ? real_max : (value < real_min) ? real_min : value;
	
//	float temp;
//	if(value>v_max)
//		temp=v_max;
//	else if(value<v_min)
//		temp=v_min;
//	else
//		temp=value;
//	return(temp);
}

float limit_convert(float value,float v_max,float v_min,float out_max,float out_min)
{
	float temp;
	temp=(value-v_min)/(v_max-v_min)*(out_max-out_min)+out_min;
	return(temp);
}

void PID_controller(PID *N_PID)
{
	float dT = SYS_PERIOD;		//�������ڣ���λs
	//float dT = 0.01;
	PID temp_PID = *N_PID;
	float val_kp,val_ki,val_kd;
	
	temp_PID.Now_error = temp_PID.In_val - temp_PID.Feedback_val;
	
//	if(temp_PID.Now_error<0.0005 && temp_PID.Now_error>-0.0005) temp_PID.Now_error = 0;
	
	val_kp = temp_PID.kp*temp_PID.Now_error;
	
	if (temp_PID.ki == 0)
		val_ki = 0;
	else
		val_ki = temp_PID.ki*temp_PID.Now_error*dT+temp_PID.integral;
	
	
	
	val_kd = temp_PID.kp*temp_PID.kd*(temp_PID.Now_error-temp_PID.previous_error)/dT;
	
	val_ki = limit(val_ki,temp_PID.integral_max,temp_PID.integral_min);
	
	temp_PID.u_out = limit(val_kp + val_ki + val_kd,1,-1);

	temp_PID.val_ki = val_ki;
	
	temp_PID.integral = val_ki;
	
	temp_PID.previous_error = temp_PID.Now_error;
	
	*N_PID = temp_PID;
	
}

void Controller_Init(void)
{
	Sensor_Init();		//
//	Controller_Out_Set_Init();
	dataspace_Init();
	
	my_Order_mode.mode = dingdian;
	
	my_Order_mode.start_or_stop = _stop;		//Ĭ����ֹͣ״̬
	
	my_Order_mode.motor1_sudu = 0.05f;
	
	my_Order_mode.nei_wai_xuanze = External_and_Internal;	
	
	//my_Order_mode.start_or_stop = _start;		//Ĭ����ֹͣ״̬
	
	Control_P_PID.kd = 0;
	Control_P_PID.ki = 0;
	Control_P_PID.kp = 0;
	Control_P_PID.integral = 0;
	Control_P_PID.integral_max =1;
	Control_P_PID.integral_min = -1;
	Control_P_PID.previous_error = 0;
	
	
	Control_jiao_PID.kd = 0;
	Control_jiao_PID.ki = 0;
	Control_jiao_PID.kp = 0;
	Control_jiao_PID.integral = 0;
	Control_jiao_PID.integral_max = 1;
	Control_jiao_PID.integral_min = -1;
	Control_jiao_PID.previous_error = 0;
	
	P_Control_P_PID = &Control_P_PID;
	P_Control_jiao_PID = &Control_jiao_PID;
	
	
	
	
//	Control_P_LADRC.r = 1.0f;
//	Control_P_LADRC.w0 = 1.0f;
//	Control_P_LADRC.wc = 3.0f;
//	Control_P_LADRC.b0 = 1000.0f;
		
	P_Control_P_LADRC = &Control_P_LADRC;
	P_Control_jiao_LADRC = &Control_jiao_LADRC;
	
	LADRC_Init(P_Control_P_LADRC, 0.1, 0.1, 0.5, 1000.0f);
	LADRC_Init(P_Control_jiao_LADRC, 0.1, 0.1, 0.5, 1000.0f);
	
}

float One_Order_Filter(float New_val,float Old_val,float pinlv,float dtt)
{
	//�������ݣ���ֹƵ�ʣ���������
	//In[0]�洢���µ���ֵ��In[1]�洢��һ�ε���ֵ

	float T =1/(2*3.14*pinlv);
	
	float Val_filted;
	
	Val_filted = (New_val + T/dtt*Old_val)/(T/dtt+1);
	
	
	//Val_filted = (New_val*dtt+Old_val*T)/(1+dtt);
	
	return Val_filted;

}

float average_Filter(float in_data,float data[100])
{
	float sum = 0;
	int i;
	for(i=0; i<99; i++)
	{
		data[i]=data[i+1];
		sum = sum + data[i];
	}
	data[99] = in_data;
	sum = sum + data[99];
	
	return(sum/100);

}

float Junzhi_Filter(float New_val)
{
	static float No_all = 0, Fist_shuzi=0, zongzhi = 0;
//	float temp_val;
	if (No_all < 100)
	{	
		zongzhi = (zongzhi*No_all+New_val)/(No_all+1);
		No_all++;
		if (No_all == 1)
			Fist_shuzi = New_val;
	}
	else
	{
		zongzhi = (zongzhi*100-Fist_shuzi+New_val)/100;
		
	}
	return zongzhi;
//	No_all++;
}

float Rate_limit(float New_val,float Old_val,float sudu_limit,float zhouqi)
{
	//zhouqi�ĵ�λ����
	float sudu,temp;
	sudu = fabsf(New_val-Old_val)/zhouqi;
	if(sudu>sudu_limit)
		if(New_val>Old_val)
			temp = Old_val+sudu_limit*zhouqi;
		else if(New_val<Old_val)
			temp = Old_val-sudu_limit*zhouqi;
		else
			temp = New_val;
	else
		temp = New_val;
	return temp;
}

//ͨ�Ŵ�������
void usart_callback(uint8_t ch)
{
//	printf("ch = %x \n", ch);
	int i = 0;
	
	for (i = 0;i <= (USART_RX_SIZE - 2); i++)
	{
		Rx_buffer[i] = Rx_buffer[i+1];
	}
	
	Rx_buffer[USART_RX_SIZE - 1] = ch;
	
	rcv_cnt++;
	
	if((Rx_buffer[USART_RX_SIZE - 1]) == '#' && (rcv_cnt >= USART_RX_SIZE))
	{
		rcv_cnt = 0;
		//������� ��������
		if (Rx_buffer[USART_RX_SIZE - 1] == '#')
		{
			Rx_Finished_flag = 1;
		}
	}
}

// ========== ���ߺ��� ==========
float bytes_to_float(const volatile uint8_t *p)
{
    float f;
//#if RX_BIG_ENDIAN
    // ���ģʽ���ֽ�˳��ת
    uint8_t buf[4];
    buf[0] = p[3];
    buf[1] = p[2];
    buf[2] = p[1];
    buf[3] = p[0];
    memcpy(&f, buf, 4);
//#else
//    // С��ģʽ��ֱ�ӿ�������
//    memcpy(&f, p, 4);
//#endif
    return f;
}

// ��������С��ģʽ��floatת��Ϊ���ģʽ���ֽ�����
void float_to_big_endian(uint8_t *dest, float value) {
    uint8_t *src = (uint8_t *)&value;
    // С��ģʽ�����ֽ���ǰ�����ֽ��ں�
    // ���ģʽ�����ֽ���ǰ�����ֽ��ں�������Ҫ��ת
    dest[0] = src[3];
    dest[1] = src[2];
    dest[2] = src[1];
    dest[3] = src[0];
}

// ��ȫ�� memcpy��֧�ִ� volatile Դ��ַ����
void safe_memcpy_from_volatile(void *dst, const volatile void *src, size_t len)
{
    const volatile uint8_t *s = (const volatile uint8_t*)src;
    uint8_t *d = (uint8_t*)dst;
    while (len--) {
        *d++ = *s++;
    }
}

