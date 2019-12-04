/**
 *	@brief		Declares the TPM2_PolicyCommandCode method
 *	@file		TPM2_PolicyCommandCode.h
 *	@details	This file was auto-generated based on TPM2.0 specification revision 116.
 */
#pragma once
#include "TPM2_Types.h"

/**
 *	@brief	Implementation of TPM2_PolicyCommandCode command.
 *
 *	@retval TPM_RC_VALUE					commandCode of policySession previously set to a different value
 */
_Check_return_
unsigned int
TSS_TPM2_PolicyCommandCode(
	_In_	TPMI_SH_POLICY		policySession,
	_In_	TPM_CC				code
);