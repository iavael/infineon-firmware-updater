/**
 *	@brief		Declares the TPM2_StartAuthSession method
 *	@file		TPM2_StartAuthSession.h
 *	@details	This file was auto-generated based on TPM2.0 specification revision 116.
 */
#pragma once
#include "TPM2_Types.h"

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
);