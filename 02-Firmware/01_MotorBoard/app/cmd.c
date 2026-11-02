#include "cmd.h"
#include <lib_string.h>
#include "usart.h"
#include "common.h"
#include "delay.h"
#include "flash_param.h"
#include "iwdg.h"
#include "AD7606.h"

static ShellCmdInput help_command = 
{
	"help",
	"help : List all command in system!",
	shell_help_cmd,
	0
};


void cmd_register_all(void)
{
	shell_cmd_register(&help_command);
}


