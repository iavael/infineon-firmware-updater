/**
 *	@brief		Declares the TPM2_Shutdown method
 *	@file		TPM2_Shutdown.h
 *	@details	This file was auto-generated based on TPM2.0 specification revision 116.
 */
#pragma once
#include "TPM2_Types.h"

/**
 *	@brief	Implementation of TPM2_Shutdown command.
 *
 *	@retval TPM_RC_TYPE						if PCR bank has been re-configured, a CLEAR StateSave() is required
 */
_Check_return_
unsigned int
TSS_TPM2_Shutdown(
	_In_	TPM_SU		shutdownType
);