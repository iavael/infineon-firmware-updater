/**
 *	@brief		Implements the TPM2_Startup method
 *	@file		TPM2_Startup.c
 *	@details	This file was auto-generated based on TPM2.0 specification revision 116.
 */
#include "TPM2_Startup.h"
#include "TPM2_Marshal.h"
#include "DeviceManagement.h"
#include "Platform.h"
#include "StdInclude.h"

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
)
{
	unsigned int unReturnValue = RC_SUCCESS;
	do
	{
		BYTE rgbRequest[MAX_COMMAND_SIZE] = {0};
		BYTE rgbResponse[MAX_RESPONSE_SIZE] = {0};
		BYTE* pbBuffer = NULL;
		INT32 nSizeRemaining = sizeof(rgbRequest);
		INT32 nSizeResponse = sizeof(rgbResponse);
		// Request parameters
		TPM_ST tag = TPM_ST_NO_SESSIONS;
		UINT32 unCommandSize = 0;
		TPM_CC commandCode = TPM_CC_Startup;
		// Response parameters
		UINT32 unResponseSize = 0;
		TPM_RC responseCode = TPM_RC_SUCCESS;

		// Marshal the request
		pbBuffer = rgbRequest;
		unReturnValue = TSS_TPMI_ST_COMMAND_TAG_Marshal(&tag, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_UINT32_Marshal(&unCommandSize, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_TPM_CC_Marshal(&commandCode, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_TPM_SU_Marshal(&startupType, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;

		// Overwrite unCommandSize
		unCommandSize = sizeof(rgbRequest) - nSizeRemaining;
		pbBuffer = rgbRequest + 2;
		nSizeRemaining = 4;
		unReturnValue = TSS_UINT32_Marshal(&unCommandSize, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;

		// Transmit the command over TDDL
		unReturnValue = DeviceManagement_Transmit(rgbRequest, unCommandSize, rgbResponse, (unsigned int*)&nSizeResponse);
		if (TPM_RC_SUCCESS != unReturnValue)
			break;

		// Unmarshal the response
		pbBuffer = rgbResponse;
		nSizeRemaining = nSizeResponse;
		unReturnValue = TSS_TPM_ST_Unmarshal(&tag, &pbBuffer, &nSizeRemaining);
		if (TPM_RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_UINT32_Unmarshal(&unResponseSize, &pbBuffer, &nSizeRemaining);
		if (TPM_RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_TPM_RC_Unmarshal(&responseCode, &pbBuffer, &nSizeRemaining);
		if (TPM_RC_SUCCESS != unReturnValue)
			break;
		if (responseCode != TPM_RC_SUCCESS)
		{
			unReturnValue = RC_TPM_MASK | responseCode;
			break;
		}
	}
	WHILE_FALSE_END;
	return unReturnValue;
}