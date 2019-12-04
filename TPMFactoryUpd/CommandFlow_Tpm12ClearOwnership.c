/**
 *	@brief		Implements the command flow to clear the TPM1.2 ownership.
 *	@details	This module removes the TPM owner that was temporarily created during an update from TPM1.2 to TPM1.2.
 *	@file		CommandFlow_Tpm12ClearOwnership.c
 *	@copyright	Copyright 2014 - 2017 Infineon Technologies AG ( www.infineon.com )
 *
 *	@copyright	All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "CommandFlow_Tpm12ClearOwnership.h"
#include "FirmwareUpdate.h"
#include "Crypt.h"
#include "TPM_OIAP.h"
#include "TPM_OSAP.h"
#include "TPM_OwnerClear.h"

/**
 *	@brief		Processes a sequence of TPM commands to clear the TPM1.2 ownership.
 *	@details	This function removes the TPM owner that was temporarily created during an update from TPM1.2 to TPM1.2.
 *				The function utilizes the MicroTss library.
 *
 *	@param		PpTpmClearOwnership		Pointer to an initialized IfxTpm12ClearOwnership structure to be filled in
 *
 *	@retval		RC_SUCCESS						The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER				An invalid parameter was passed to the function. PpTpmClearOwnership was invalid.
 *	@retval		RC_E_NOT_SUPPORTED_FEATURE		In case of the TPM is a TPM2.0
 *	@retval		RC_E_TPM12_NO_OWNER				The TPM1.2 does not have an owner.
 *	@retval		RC_E_NO_IFX_TPM					The underlying TPM is not an Infineon TPM.
 *	@retval		RC_E_UNSUPPORTED_CHIP			In case of the underlying TPM does not support that functionality
 *	@retval		RC_E_TPM12_INVALID_OWNERAUTH	In case of the expected owner authorization can not be verified
 *	@retval		RC_E_FAIL						An unexpected error occurred.
 *	@retval		...								Error codes from called functions.
 */
_Check_return_
unsigned int
CommandFlow_Tpm12ClearOwnership_Execute(
	_Inout_ IfxTpm12ClearOwnership* PpTpmClearOwnership)
{
	unsigned int unReturnValue = RC_E_FAIL;

	do
	{
		// TPM operation mode
		TPM_STATE sTpmState = {{0}};
		// SHA1 Hash of the default owner password
		TPM_AUTHDATA ownerAuthData = {{
				0x67, 0x68, 0x03, 0x3e, 0x21,
				0x64, 0x68, 0x24, 0x7b, 0xd0,
				0x31, 0xa0, 0xa2, 0xd9, 0x87,
				0x6d, 0x79, 0x81, 0x8f, 0x8f
			}
		};

		// Parameter check
		if (NULL == PpTpmClearOwnership)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			ERROR_STORE(unReturnValue, L"Bad parameter detected.");
			break;
		}

		// Calculate TPM operational mode
		unReturnValue = FirmwareUpdate_CalculateState(&sTpmState);
		if (RC_SUCCESS != unReturnValue)
		{
			ERROR_STORE(unReturnValue, L"FirmwareUpdate_CalculateState returned an unexpected value.");
			break;
		}

		// Check TPM operation mode
		if (sTpmState.attribs.tpm12 && sTpmState.attribs.tpm12owner)
		{
			// Continue...
		}
		else if (sTpmState.attribs.tpm20)
		{
			// TPM2.0
			unReturnValue = RC_E_TPM_NOT_SUPPORTED_FEATURE;
			ERROR_STORE(unReturnValue, L"Detected TPM is a TPM2.0.");
			break;
		}
		else if (sTpmState.attribs.tpm12)
		{
			// Unowned TPM1.2
			unReturnValue = RC_E_TPM12_NO_OWNER;
			ERROR_STORE(unReturnValue, L"Detected TPM1.2 has no owner.");
			break;
		}
		else if (!sTpmState.attribs.infineon)
		{
			// Unsupported vendor
			unReturnValue = RC_E_NO_IFX_TPM;
			ERROR_STORE(unReturnValue, L"Detected TPM is not an Infineon TPM.");
			break;
		}
		else if (sTpmState.attribs.unsupportedChip)
		{
			// Unsupported TPM1.2 chip
			unReturnValue = RC_E_UNSUPPORTED_CHIP;
			ERROR_STORE(unReturnValue, L"Detected TPM1.2 is not supported.");
			break;
		}
		else
		{
			unReturnValue = RC_E_UNSUPPORTED_CHIP;
			ERROR_STORE(unReturnValue, L"Detected TPM is not in the correct mode.");
			break;
		}

		// Check if owner authorization password is the default value as expected
		unReturnValue = FirmwareUpdate_CheckOwnerAuthorization(ownerAuthData.authdata);
		if (RC_SUCCESS != unReturnValue)
		{
			ERROR_STORE(unReturnValue, L"FirmwareUpdate_CheckOwnerAuthorization returned an unexpected value.");
			if (TPM_AUTHFAIL == (unReturnValue ^ RC_TPM_MASK))
			{
				unReturnValue = RC_E_TPM12_INVALID_OWNERAUTH;
				ERROR_STORE(unReturnValue, L"The owner password is not default. Owner authentication check failed.");
			}
			break;
		}

		// Clear ownership
		{
			TPM_AUTHHANDLE unAuthHandle = 0;
			TPM_NONCE sNonceEven = {{0}};

			// Create OIAP Session
			unReturnValue = TSS_TPM_OIAP(&unAuthHandle, &sNonceEven);
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"TPM_OIAP command returned an unexpected value");
				break;
			}

			// Clear TPM1.2 Ownership
			unReturnValue = TSS_TPM_OwnerClear(unAuthHandle, &sNonceEven, FALSE, &ownerAuthData);
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"TPMOwnerClear returned an unexpected value");
				break;
			}
		}
	}
	WHILE_FALSE_END;

	PpTpmClearOwnership->unReturnCode = unReturnValue;
	unReturnValue = RC_SUCCESS;

	return unReturnValue;
}
