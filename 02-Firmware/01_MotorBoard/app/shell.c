#include "shell.h"
#include <lib_string.h>
#include "cmd.h"

static unsigned int ShellRegisterNum = 0;
static ShellCmdInput *current_cmd = NULL;
//解析
static ShellCmdParase CmdParase;
static char shell_temp_buf[SHELL_BUF_SIZE];

static ShellManage  shellManage;
static ShellManage *shellptr = NULL;

static void shell_parase_init(void)
{
	int i = 0;
	
	CmdParase.params_cnt = 0;
	for(i = 0; i < SHELL_CMD_PARAMS_MAX; i++)
	{
			raw_memset(CmdParase.params[i], 0, SHELL_CMD_PARAMS_SIZE);
	}
}

/*****************************ShellManage 父类操作*********************************/
static void shell_manage_clear(void)
{
	int i = 0;
 
	ShellRegisterNum = 0;
	shellManage.init = NULL;
	shellManage.prefix_parase = NULL;
	shellManage.params_parase = NULL;
	for(i = 0; i < SHELL_CMD_NUM_MAX; i++)
	{
			shellManage.CmdInput[i] = NULL;
	}
}

static ShellRet shell_prefix_parase(char *input)
{
	ShellRet ret = RET_PREFIX_FAIL;
	int i = 0;
	char *ptr = input;
	int len = raw_strlen(ptr);
	int prefix_len = 0;
	
	do
	{
		 if(raw_strncmp(ptr, shellManage.CmdInput[i]->cmd, raw_strlen(shellManage.CmdInput[i]->cmd)) == 0)
		 {
				 shell_parase_init();
				 current_cmd = shellManage.CmdInput[i];
				 if(current_cmd->NumOfParam == 0) //无参数
				 {
						 current_cmd->CmdInterpret(&CmdParase);
						 ret = RET_PARAMS_OK;
						 raw_memset(shell_temp_buf, 0, SHELL_BUF_SIZE);
						 current_cmd = NULL;
						 break;
				 }
				 else                            //有参数
				 {
						 prefix_len = raw_strlen(current_cmd->cmd);
						 if(prefix_len >= (len - 1))
						 {
								 printf("Error, No Param, Please input again!\r\n");
								 ret = RET_PREFIX_FAIL;
						 }
						 else
						 {
								 len -= ( prefix_len+ 1);  //有空格 要减去空格
								 ptr += (prefix_len + 1);
								 raw_memset(shell_temp_buf, 0, SHELL_BUF_SIZE);
								 raw_strncpy(shell_temp_buf, ptr, len);
								 ret = RET_PREFIX_OK;
						 }
						 break;
				 }
		 }
		 else
		 {
				 i++;
		 }
	}while(i < ShellRegisterNum);
	
	return ret;
}

static ShellRet shell_params_parase(void)
{
	ShellRet ret = RET_PARAMS_FAIL;
	char *ptb = shell_temp_buf;
	char *ptd = ptb;
	
	if(current_cmd == NULL) return ret;
	
	do
	{
			while((*ptb != 0x00) &&(*ptb != ' '))
			{
					ptb++;
			}
			raw_strncpy(CmdParase.params[CmdParase.params_cnt],ptd, ptb - ptd);
			CmdParase.params_cnt++;
			
			if(*ptb != 0x00)
			{
					if(CmdParase.params_cnt == current_cmd->NumOfParam)
					{
							ret = RET_PARAMS_OK;
							current_cmd->CmdInterpret(&CmdParase);
							ret = RET_PARAMS_OK;
							raw_memset(shell_temp_buf, 0, SHELL_BUF_SIZE);
							current_cmd = NULL;
							shell_parase_init();
							break;
					}
					else
					{
							ptb += 1;
							ptd = ptb;
					}
			}
			else
			{
					if(CmdParase.params_cnt < current_cmd->NumOfParam)
					{
							ret = RET_PARAMS_FAIL;
					}
					else
					{
							current_cmd->CmdInterpret(&CmdParase);
							ret = RET_PARAMS_OK;
					}
					raw_memset(shell_temp_buf, 0, SHELL_BUF_SIZE);
					current_cmd = NULL;
					shell_parase_init();
					break;
			}
	}while(CmdParase.params_cnt < current_cmd->NumOfParam);
	
	return ret;
}

static void shell_manage_init(cmd_registe all)
{
    shellManage.init = all;
    shellManage.prefix_parase = shell_prefix_parase;
    shellManage.params_parase = shell_params_parase;
}

ShellManage* shell_manage_get(void)
{
	if(shellptr == NULL)
	{
			shell_manage_clear();
			shell_manage_init(cmd_register_all);
			shellManage.init();
			shellptr = &shellManage;
	}
	return shellptr;
}

void shell_cmd_register(ShellCmdInput *cmd)
{
	if(ShellRegisterNum < SHELL_CMD_NUM_MAX)
	{
			shellManage.CmdInput[ShellRegisterNum] = cmd;
			ShellRegisterNum++;
	}
	else
	{
			printf("Shell Cmd List if full!\r\n");
	}
}

void shell_help_cmd(ShellCmdParase *parase)
{
    int i = 0;
    
    for(i = 0; i < ShellRegisterNum; i++)
    {
        printf("%s\r\n", shellManage.CmdInput[i]->HelpString);
    }
}

unsigned int HEXstringToNum(unsigned char *str, int length)
{  
	unsigned char  revstr[16]={0}; 
	unsigned int   num[16]={0};  
	unsigned int   count=1;  
	unsigned int   result=0; 
  int 	i;
	raw_memcpy(revstr,str,16);  
	for(i=length-1;i>=0;i--)  
	{  
			if ((revstr[i]>='0') && (revstr[i]<='9'))  
			num[i]=revstr[i]-48;
			else if ((revstr[i]>='a') && (revstr[i]<='f'))  
			num[i]=revstr[i]-'a'+10;  
			else if ((revstr[i]>='A') && (revstr[i]<='F'))  
			num[i]=revstr[i]-'A'+10;  
			else  
			num[i]=0;

			if('x' == revstr[1] || 'X' == revstr[1]) 
			{
					result=result+num[i]*count;  
					count=count*16;
			}
			else 
			{
					result=result+num[i]*count;  
					count=count*10;
			}
	}  
	return result;  
}


