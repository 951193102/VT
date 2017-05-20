#pragma once 
#include <ntddk.h>

typedef struct _save_handlentry {
	struct _save_handlentry*head;		//ͷָ��
	HANDLE dbgProcessId;				//����ID
	PEPROCESS dbgProcessStruct;			//����PEPROCESS

	struct _save_handlentry*next;		//��һ���ڵ�

}_save_handlentry, *p_save_handlentry;

p_save_handlentry CreateList();
p_save_handlentry InsertList(HANDLE dbgProcessId,
	PEPROCESS dbgProcessStruct, p_save_handlentry phead);
p_save_handlentry QueryList(p_save_handlentry phead, HANDLE dbgProcessId, PEPROCESS dbgProcessStruct);
void DeleteList(p_save_handlentry pclid);