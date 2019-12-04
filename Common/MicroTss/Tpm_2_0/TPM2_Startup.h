/**
 *	@brief		Declares the TPM2_Startup method
 *	@file		TPM2_Startup.h
 *	@details	This file was auto-generated based on TPM2.0 specification revision 116.
 */
#pragma once
#include "TPM2_Types.h"

/**
 *	@brief	Implementation of TPM2_Startup command.
 *
 *	@retval TPM_RC_LOCALITY					a Startup(STATE) does not have the same H-CRTM state as the previous Startup() or the locality of the startup is not 0 pr 3
 *	@retval TPM_RC_NV_UNINITIALIZED			the saved state cannot be recovered and a Startup(CLEAR) is required.
 *	@retval TPM_RC_VALUE					start up type is not compatible with previous shutdown sequence
 */
_Check_return_
unsigned int
TSS_TPM2_Startup(
	_In_	TPM_SU		startupType
);