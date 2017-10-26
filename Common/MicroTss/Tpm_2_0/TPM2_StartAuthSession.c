/**
 *	@brief		Implements the TPM2_StartAuthSession method
 *	@file		TPM2_StartAuthSession.c
 *	@details	This file was auto-generated based on TPM2.0 specification revision 116.
 */
#include "TPM2_StartAuthSession.h"
#include "TPM2_Marshal.h"
#include "DeviceManagement.h"
#include "Platform.h"
#include "StdInclude.h"

/**
 *	@brief	Implementation of TPM2_StartAuthSession command.
 *
 *	@retval TPM_RC_ATTRIBUTES				tpmKey does not reference a decrypt key
 *	@retval TPM_RC_CONTEXT_GAP				the difference between the most recently created active context and the oldest active context is at the limits of the TPM
 *	@retval TPM_RC_HANDLE					input decrypt key handle only has public portion loaded
 *	@retval TPM_RC_MODE						symmetric specifies a block cipher but the mode is not TPM_ALG_CFB.
 *	@retval TPM_RC_SESSION_HANDLES			no session handle is available
 *	@retval TPM_RC_SESSION_MEMORY			no more slots for loading a session
 *	@retval TPM_RC_SIZE						nonce less than 16 octets or greater than the size of the digest produced by authHash
 *	@retval TPM_RC_VALUE					secret size does not match decrypt key type; or the recovered secret is larger than the digest size of the nameAlg of tpmKey; or, for an RSA decrypt key, if encryptedSecret is greater than the public exponent of tpmKey.
 */
_Check_return_
unsigned int
TSS_TPM2_StartAuthSession(
	_In_	TPMI_DH_OBJECT				tpmKey,
	_In_	TPMI_DH_ENTITY				bind,
	_In_	TPM2B_NONCE					nonceCaller,
	_In_	TPM2B_ENCRYPTED_SECRET		encryptedSalt,
	_In_	TPM_SE						sessionType,
	_In_	TPMT_SYM_DEF				symmetric,
	_In_	TPMI_ALG_HASH				authHash,
	_Out_	TPMI_SH_AUTH_SESSION*		pSessionHandle,
	_Out_	TPM2B_NONCE*				pNonceTPM
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
		TPM_CC commandCode = TPM_CC_StartAuthSession;
		// Response parameters
		UINT32 unResponseSize = 0;
		TPM_RC responseCode = TPM_RC_SUCCESS;

		// Initialize _Out_ parameters
		unReturnValue |= Platform_MemorySet(pSessionHandle, 0x00, sizeof(TPMI_SH_AUTH_SESSION));
		unReturnValue |= Platform_MemorySet(pNonceTPM, 0x00, sizeof(TPM2B_NONCE));
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
		unReturnValue = TSS_TPMI_DH_OBJECT_Marshal(&tpmKey, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_TPMI_DH_ENTITY_Marshal(&bind, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_TPM2B_NONCE_Marshal(&nonceCaller, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_TPM2B_ENCRYPTED_SECRET_Marshal(&encryptedSalt, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_TPM_SE_Marshal(&sessionType, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_TPMT_SYM_DEF_Marshal(&symmetric, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_TPMI_ALG_HASH_Marshal(&authHash, &pbBuffer, &nSizeRemaining);
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
		unReturnValue = TSS_TPMI_SH_AUTH_SESSION_Unmarshal(pSessionHandle, &pbBuffer, &nSizeRemaining);
		if (TPM_RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_TPM2B_NONCE_Unmarshal(pNonceTPM, &pbBuffer, &nSizeRemaining);
		if (TPM_RC_SUCCESS != unReturnValue)
			break;
	}
	WHILE_FALSE_END;
	return unReturnValue;
}