/**
 *	@brief		Declares the TPM2_GetCapability method
 *	@file		TPM2_GetCapability.h
 *	@details	This file was auto-generated based on TPM2.0 specification revision 116.
 */
#pragma once
#include "TPM2_Types.h"

/**
 *	@brief	Implementation of TPM2_GetCapability command.
 *
 *	@retval TPM_RC_HANDLE					value of property is in an unsupported handle range for the TPM_CAP_HANDLES capability value
 *	@retval TPM_RC_VALUE					invalid capability; or property is not 0 for the TPM_CAP_PCRS capability value
 */
_Check_return_
unsigned int
TSS_TPM2_GetCapability(
	_In_	TPM_CAP						capability,
	_In_	UINT32						property,
	_In_	UINT32						propertyCount,
	_Out_	TPMI_YES_NO*				pMoreData,
	_Out_	TPMS_CAPABILITY_DATA*		pCapabilityData
);