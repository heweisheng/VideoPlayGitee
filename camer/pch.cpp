// pch.cpp: 与预编译标头对应的源文件；编译成功所必需的

#include "pch.h"

char g_sError_ptr[64] = {0};
char g_sError_where[1024] = { 0 };
// 一般情况下，忽略此文件，但如果你使用的是预编译标头，请保留它。
