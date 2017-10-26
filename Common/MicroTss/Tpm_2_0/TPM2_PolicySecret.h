/**
 *	@brief		Declares the TPM2_PolicySecret method
 *	@file		TPM2_PolicySecret.h
 *	@details	This file was auto-generated based on TPM2.0 specification revision 116.
 */
#pragma once
#include "TPM2_Types.h"

/**
 *	@brief	Implementation of TPM2_PolicySecret command.
 *
 *	@retval TPM_RC_CPHASH					cpHash for policy was previously set to a value that is not the same as cpHashA
 *	@retval TPM_RC_EXPIRED					expiration indicates a time in the past
 *	@retval TPM_RC_NONCE					nonceTPM does not match the nonce associated with policySession
 *	@retval TPM_RC_SIZE						cpHashA is not the size of a digest for the hash associated with policySession
 *	@retval TPM_RC_VALUE					input policyID or expiration does not match the internal data in policy session
 */
_Check_return_
unsigned int
TSS_TPM2_PolicySecret(
	_In_	TPMI_DH_ENTITY					authHandle,
	_In_	AuthorizationCommandData		authHandleSessionRequestData,
	_In_	TPMI_SH_POLICY					policySession,
	_In_	TPM2B_NONCE						nonceTPM,
	_In_	TPM2B_DIGEST					cpHashA,
	_In_	TPM2B_NONCE						policyRef,
	_In_	INT32							expiration,
	_Out_	TPM2B_TIMEOUT*					pTimeout,
	_Out_	TPMT_TK_AUTH*					pPolicyTicket,
	_Out_	AcknowledgmentResponseData*		pAuthHandleSessionResponseData
);