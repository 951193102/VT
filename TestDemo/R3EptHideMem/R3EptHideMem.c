#pragma once

#include "R3EptHideMem.h"



extern p_save_handlentry PmainList;
#ifndef __EPT_STRUCT
#define  __EPT_STRUCT
static LIST_ENTRY R3pageList;
static KSPIN_LOCK R3PageLock;

typedef struct _HOOK_CONTEXT
{
	BOOLEAN Hook;           // TRUE to hook page, FALSE to unhook
	ULONG64 DataPagePFN;    // Physical data page PFN
	ULONG64 CodePagePFN;    // Physical code page PFN
} HOOK_CONTEXT, *PHOOK_CONTEXT;

#endif // !__EPT_STRUCT
VOID InitialzeR3EPTHOOK() 
{
	KeInitializeSpinLock(&R3PageLock);
	InitializeListHead(&R3pageList);
}

//����R3��EPT�쳣
BOOLEAN R3_HideMEM_Violation(IN PGUEST_STATE GuestState) {
	PEPT_DATA pEPT = &GuestState->Vcpu->EPT;
	ULONG64 pfn = PFN(GuestState->PhysicalAddress.QuadPart);
	PR3EPT_HOOK Phook = NULL;
	p_save_handlentry Padd = NULL;
	PEPT_VIOLATION_DATA pViolationData = (PEPT_VIOLATION_DATA)&GuestState->ExitQualification;
	ULONG64 gva = GuestState->LinearAddress;
	Phook = Page_FindStructByGvaBase(gva);

	if (Phook)
	{
		//uanc	DbgPrint("R3 EPT���� \n");


		ULONG64 TargetPFN = Phook->Data_PAGE_PFN;
		EPT_ACCESS TargetAccess = EPT_ACCESS_ALL;

		// Executable page for writing
		if (pViolationData->Fields.Read)
		{

			Padd = QueryList(PmainList, PsGetCurrentProcessId(), PsGetCurrentProcess());
			if (Padd != NULL)
			{
				//DbgPrint("R3 EPT ������read���� \n");
				TargetPFN = Phook->Code_PAGE_PFN;//���Թ��߷����ڴ������ҳ
			}
			else
			{

				TargetPFN = Phook->Data_PAGE_PFN;

			}

			TargetAccess = EPT_ACCESS_RW;
		}
		else if (pViolationData->Fields.Write)
		{

			//	DbgPrint("R3 EPT Write���� \n");
			TargetPFN = Phook->Code_PAGE_PFN;


			TargetAccess = EPT_ACCESS_RW;
		}
		else if (pViolationData->Fields.Execute)
		{
			//DbgPrint("R3 EPT Execute���� \n");

			TargetPFN = Phook->Code_PAGE_PFN;


			TargetAccess = EPT_ACCESS_EXEC;
		}
		else
		{
			/* DPRINT(
			"HyperBone: CPU %d: %s: Impossible page 0x%p access 0x%X\n", CPU_IDX, __FUNCTION__,
			GuestState->PhysicalAddress.QuadPart, pViolationData->All
			);*/
		}


		EptUpdateTableRecursive(pEPT, pEPT->PML4Ptr, EPT_TOP_LEVEL, pfn, TargetAccess, TargetPFN, 1);
		EPT_CTX ctx = { 0 };
		__invept(INV_ALL_CONTEXTS, &ctx);
		GuestState->Vcpu->HookDispatch.pEntry = Phook;
		GuestState->Vcpu->HookDispatch.Rip = GuestState->GuestRip;
		ToggleMTF(TRUE);
		return TRUE;
	}
	else {


		//	EptUpdateTableRecursive(pEPT, pEPT->PML4Ptr, EPT_TOP_LEVEL, pfn, EPT_ACCESS_ALL, pfn, 1);
		return FALSE;

	}
}

//��ȡָ��GVA��HOOK����
PR3EPT_HOOK Page_FindStructByGvaBase(ULONG64 GVA) {

	KIRQL OldIrql;
	PLIST_ENTRY Entry;
	ULONG64 GVAbase = PAGE_ALIGN(GVA);
	R3EPT_HOOK *TempItem = NULL;
	R3EPT_HOOK* DFind = NULL;
	KeAcquireSpinLock(&R3PageLock, &OldIrql);
	Entry = R3pageList.Flink;
	while (Entry != &R3pageList)
	{
		TempItem = CONTAINING_RECORD(Entry, R3EPT_HOOK, PageList);
		Entry = Entry->Flink;
		if (GVA != NULL)
		{

			if (TempItem->Data_PAGE_VA == GVAbase)
			{
				DFind = TempItem;
				break;
			}
		}


	}
	KeReleaseSpinLock(&R3PageLock, OldIrql);
	return DFind;


}