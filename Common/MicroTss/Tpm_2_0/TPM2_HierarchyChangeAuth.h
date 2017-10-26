/**
 *	@brief		Declares the TPM2_HierarchyChangeAuth method
 *	@file		TPM2_HierarchyChangeAuth.h
 *	@details	This file was auto-generated based on TPM2.0 specification revision 116.
 */
#pragma once
#include "TPM2_Types.h"

/**
 *	@brief	Implementation of TPM2_HierarchyChangeAuth command.
 *
 *	@retval TPM_RC_SIZE						newAuth size is greater than that of integrity hash digest
 */
_Check_return_
unsigned int
TSS_TPM2_HierarchyChangeAuth(
	_In_	TPMI_RH_HIERARCHY_AUTH			authHandle,
	_In_	AuthorizationCommandData		authHandleSessionRequestData,
	_In_	TPM2B_AUTH						newAuth,
	_Out_	AcknowledgmentResponseData*		pAuthHandleSessionResponseData
);