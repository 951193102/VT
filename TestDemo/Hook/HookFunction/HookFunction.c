#include "HookFunction.h"
#include "..\..\ProtectWindow\ProtectWindow.h"


typedef int(*LDE_DISASM)(void *p, int dw);
LDE_DISASM LDE;
#define kmalloc(_s)	ExAllocatePoolWithTag(NonPagedPool, _s, 'SYSQ')
//���룺��HOOK������ַ����������ַ������ԭʼ������ַ��ָ�룬���ղ������ȵ�ָ�룻���أ�ԭ��ͷN�ֽڵ�����
PVOID HookKernelApi(IN PVOID ApiAddress, IN PVOID Proxy_ApiAddress, OUT PVOID *Original_ApiAddress, OUT ULONG *PatchSize)
{
	KIRQL irql;
	UINT64 tmpv;
	PVOID head_n_byte, ori_func;
	UCHAR jmp_code[] = "\xFF\x25\x00\x00\x00\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF";
	UCHAR jmp_code_orifunc[] = "\xFF\x25\x00\x00\x00\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF";
	//How many bytes shoule be patch
	*PatchSize = GetPatchSize2((PUCHAR)ApiAddress);
	//step 1: Read current data
	head_n_byte = kmalloc(*PatchSize);
	irql = WPOFFx64();
	memcpy(head_n_byte, ApiAddress, *PatchSize);
	WPONx64(irql);
	//step 2: Create ori function
	ori_func = kmalloc(*PatchSize + 14);        //ԭʼ������+��ת������
	RtlFillMemory(ori_func, *PatchSize + 14, 0x90);
	tmpv = (ULONG64)ApiAddress + *PatchSize;        //��ת��û���򲹶����Ǹ��ֽ�
	memcpy(jmp_code_orifunc + 6, &tmpv, 8);
	memcpy((PUCHAR)ori_func, head_n_byte, *PatchSize);
	memcpy((PUCHAR)ori_func + *PatchSize, jmp_code_orifunc, 14);
	*Original_ApiAddress = ori_func;
	//step 3: fill jmp code
	tmpv = (UINT64)Proxy_ApiAddress;
	memcpy(jmp_code + 6, &tmpv, 8);
	//step 4: Fill NOP and hook
	irql = WPOFFx64();
	RtlFillMemory(ApiAddress, *PatchSize, 0x90);
	memcpy(ApiAddress, jmp_code, 14);
	WPONx64(irql);
	//return ori code
	return head_n_byte;
}
 



ULONG GetPatchSize2(PUCHAR Address)
{
	ULONG LenCount = 0, Len = 0;
	while (LenCount <= 14)        //������Ҫ14�ֽ�
	{
		Len = LDE(Address, 64);
		Address = Address + Len;
		LenCount = LenCount + Len;
	}
	return LenCount;
}