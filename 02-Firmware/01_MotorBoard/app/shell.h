#ifndef SHELL_H
#define SHELL_H

#include "sig.h"
#include "usart.h"

#define SHELL_CMD_NUM_MAX      35
#define SHELL_CMD_PARAMS_MAX   8
#define SHELL_CMD_PARAMS_SIZE  16
#define SHELL_BUF_SIZE         32

typedef enum _ShellRet
{
	RET_NULL,
	RET_PREFIX_OK,
	RET_PREFIX_FAIL,
	RET_PARAMS_OK,
	RET_PARAMS_FAIL,
}ShellRet;

typedef struct _ShellCmdParase
{
	//参数
	unsigned char params_cnt;
	char params[SHELL_CMD_PARAMS_MAX][SHELL_CMD_PARAMS_SIZE];
}ShellCmdParase;

typedef void (*shell_cmd_line_callback)(ShellCmdParase *parase);

typedef struct _ShellCmdInput
{
	const char* const cmd;
	const char* const HelpString;
	const shell_cmd_line_callback  CmdInterpret;
	unsigned char NumOfParam;
}ShellCmdInput;

typedef void (*cmd_registe)(void);   //负责把所有注册登记的函数统计进来

typedef struct _ShellManage
{
	void (*init)(void);
	ShellRet (*prefix_parase)(char *input);
	ShellRet (*params_parase)(void);
	ShellCmdInput *CmdInput[SHELL_CMD_NUM_MAX];
}ShellManage;

ShellManage* shell_manage_get(void);
void shell_cmd_register(ShellCmdInput *cmd);
void shell_help_cmd(ShellCmdParase *parase);
unsigned int HEXstringToNum(unsigned char *str, int length);

#endif
