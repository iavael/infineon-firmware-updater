/**
 *	@brief		Implements the TPM2_GetCapability method
 *	@file		TPM2_GetCapability.c
 *	@details	This file was auto-generated based on TPM2.0 specification revision 116.
 */
#include "TPM2_GetCapability.h"
#include "TPM2_Marshal.h"
#include "DeviceManagement.h"
#include "Platform.h"
#include "StdInclude.h"

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
		TPM_CC commandCode = TPM_CC_GetCapability;
		// Response parameters
		UINT32 unResponseSize = 0;
		TPM_RC responseCode = TPM_RC_SUCCESS;

		// Initialize _Out_ parameters
		unReturnValue |= Platform_MemorySet(pMoreData, 0x00, sizeof(TPMI_YES_NO));
		unReturnValue |= Platform_MemorySet(pCapabilityData, 0x00, sizeof(TPMS_CAPABILITY_DATA));
		if (RC_SUCCESS != unReturnValue)
			break;
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
		unReturnValue = TSS_TPM_CAP_Marshal(&capability, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_UINT32_Marshal(&property, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_UINT32_Marshal(&propertyCount, &pbBuffer, &nSizeRemaining);
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
		unReturnValue = TSS_TPMI_YES_NO_Unmarshal(pMoreData, &pbBuffer, &nSizeRemaining);
		if (TPM_RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_TPMS_CAPABILITY_DATA_Unmarshal(pCapabilityData, &pbBuffer, &nSizeRemaining);
		if (TPM_RC_SUCCESS != unReturnValue)
			break;
	}
	WHILE_FALSE_END;
	return unReturnValue;
}