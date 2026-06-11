#ifndef INC_SCRIPT_INTERPRETER_H_
#define INC_SCRIPT_INTERPRETER_H_

int ScriptInterpreter_ExecuteLine(char *line);
int ScriptInterpreter_RunScript(const char *path);

#endif /* INC_SCRIPT_INTERPRETER_H_ */