#ifndef BUILTIN_H
#define BUILTIN_H
#include "../include/command.h"


int searchBuiltInCommand(struct cmd_node *cmd);
int execBuiltInCommand(int status,struct cmd_node *cmd);
//const 是為了builtin_func 設定的
const int pwd(char **args);
const int help(char **args);
const int cd(char **args);
const int echo(char **args);
const int exit_shell(char **args);
const int record(char **args);

extern const char *builtin_str[];

extern const int (*builtin_func[]) (char **);

extern int num_builtins();


#endif
 