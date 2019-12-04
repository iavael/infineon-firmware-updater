/**
 *	@brief		Declares the TPM2_GetTestResult method
 *	@file		TPM2_GetTestResult.h
 *	@details	This file was auto-generated based on TPM2.0 specification revision 116.
 */
#pragma once
#include "TPM2_Types.h"

/**
 *	@brief	Implementation of TPM2_GetTestResult command.
 */
_Check_return_
unsigned int
TSS_TPM2_GetTestResult(
	_Out_	TPM2B_MAX_BUFFER*		pOutData,
	_Out_	TPM_RC*					pTestResult
);