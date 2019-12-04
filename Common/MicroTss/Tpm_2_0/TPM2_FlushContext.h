/**
 *	@brief		Declares the TPM2_FlushContext method
 *	@file		TPM2_FlushContext.h
 *	@details	This file was auto-generated based on TPM2.0 specification revision 116.
 */
#pragma once
#include "TPM2_Types.h"

/**
 *	@brief	Implementation of TPM2_FlushContext command.
 *
 *	@retval TPM_RC_HANDLE					flushHandle does not reference a loaded object or session
 */
_Check_return_
unsigned int
TSS_TPM2_FlushContext(
	_In_	TPMI_DH_CONTEXT		flushHandle
);