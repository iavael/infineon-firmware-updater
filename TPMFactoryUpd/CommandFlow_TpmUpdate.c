/**
 *	@brief		Implements the command flow to update the TPM firmware.
 *	@details	This module processes the firmware update. Afterwards the result is returned to the calling module.
 *	@file		CommandFlow_TpmUpdate.c
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

#include "CommandFlow_TpmUpdate.h"
#include "CommandFlow_TpmInfo.h"
#include "FirmwareImage.h"
#include "FirmwareUpdate.h"
#include "Response.h"
#include "Crypt.h"
#include "Config.h"
#include "ConfigSettings.h"
#include "TPMFactoryUpdStruct.h"
#include "Resource.h"
#include "FileIO.h"

#include <TPM2_FlushContext.h>
#include <TPM2_StartAuthSession.h>
#include <TPM2_FieldUpgradeTypes.h>
#include <TPM_OIAP.h>
#include <TPM_ReadPubEK.h>
#include <TPM_SetCapability.h>
#include <TPM_TakeOwnership.h>
#include <TSC_PhysicalPresence.h>
#include <TPM_FieldUpgradeInfoRequest.h>

// Storage Root Key well known authentication value
#define SRK_WELL_KNOWN_AUTH	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }

#define TPM12_FAMILY_STRING L"TPM12"
#define TPM20_FAMILY_STRING L"TPM20"

#define TPM_FACTORY_UPD_RUNDATA_FILE L"TPMFactoryUpd_RunData.txt"

// Exemplary SHA-1 hash value of 20 zero bytes (assumes that TPM Ownership has been taken with this string as TPM Owner authentication)

TPM_AUTHDATA s_ownerAuthData = {{
		0x67, 0x68, 0x03, 0x3e, 0x21,
		0x64, 0x68, 0x24, 0x7b, 0xd0,
		0x31, 0xa0, 0xa2, 0xd9, 0x87,
		0x6d, 0x79, 0x81, 0x8f, 0x8f
	}
};

// Flag to remember that firmware update is done through config file option
BOOL s_fUpdateThroughConfigFile = FALSE;

/**
 *	@brief		Callback function to save the used firmware image path to TPM_FACTORY_UPD_RUNDATA_FILE (once an update has been started successfully)
 *	@details	The function is called by FirmwareUpdate_UpdateImage() to create the TPM_FACTORY_UPD_RUNDATA_FILE.
 */
void
CommandFlow_TpmUpdate_UpdateStartedCallback()
{
	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);
	// Save the firmware image path in TPM_FACTORY_UPD_RUNDATA_FILE if firmware update was initiated through "-update config-file" option.
	// If the firmware update should fail unexpectedly and leave TPM in invalid firmware mode, the user can restart the system and run TPMFactoryUpd
	// to resume the firmware update with the saved firmware image path.
	// Continue on any errors if for example the saving of the file fails, etc.
	if (s_fUpdateThroughConfigFile)
	{
		void* pFile = NULL;
		if (0 == FileIO_Open(TPM_FACTORY_UPD_RUNDATA_FILE, &pFile, FILE_WRITE) && NULL != pFile)
		{
			wchar_t wszFirmwareImagePath[MAX_PATH] = {0};
			unsigned int unFirmwareImagePathSize = RG_LEN(wszFirmwareImagePath);
			if (FALSE == PropertyStorage_GetValueByKey(PROPERTY_FIRMWARE_PATH, wszFirmwareImagePath, &unFirmwareImagePathSize))
			{
				ERROR_STORE_FMT(RC_E_FAIL, L"PropertyStorage_GetValueByKey failed to get property '%ls'.", PROPERTY_FIRMWARE_PATH);
			}
			else
			{
				IGNORE_RETURN_VALUE(FileIO_WriteString(pFile, wszFirmwareImagePath));
			}
			IGNORE_RETURN_VALUE(FileIO_Close(&pFile));
		}
	}
	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_EXIT_STRING);
	return;
}

/**
 *	@brief		Checks if the given firmware package can be used to update the TPM.
 *	@details	The function calls FirmwareUpdate_CheckImage() to check whether the TPM can be updated with the given firmware package.
 *
 *	@param		PpTpmUpdate				Pointer to a IfxUpdate structure where only firmware image is accessed.
 *
 *	@retval		RC_SUCCESS					The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER			An invalid parameter was passed to the function.
 *	@retval		RC_E_CORRUPT_FW_IMAGE		In case of a corrupt firmware image.
 *	@retval		RC_E_FAIL					An unexpected error occurred.
 *	@retval		RC_E_NEWER_TOOL_REQUIRED	The firmware image provided requires a newer version of this tool.
 *	@retval		RC_E_WRONG_FW_IMAGE			In case of a wrong firmware image.
 *	@retval		RC_E_WRONG_DECRYPT_KEYS		In case the TPM2.0 does not have decrypt keys matching to the firmware image.
 *	@retval		...							Error codes from called functions.
 */
_Check_return_
unsigned int
CommandFlow_TpmUpdate_IsTpmUpdatableWithFirmware(
	_Inout_ IfxUpdate* PpTpmUpdate)
{
	unsigned int unReturnValue = RC_E_FAIL;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	do
	{
		// Check input parameters
		if (NULL == PpTpmUpdate ||
				STRUCT_TYPE_TpmUpdate != PpTpmUpdate->unType ||
				sizeof(IfxUpdate) != PpTpmUpdate->unSize)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			ERROR_STORE(unReturnValue, L"Bad parameter detected. TpmUpdate structure is not in the correct state.");
			break;
		}

		// Call CheckImage
		unReturnValue = FirmwareUpdate_CheckImage(PpTpmUpdate->rgbFirmwareImage, PpTpmUpdate->unFirmwareImageSize, &PpTpmUpdate->fValid, &PpTpmUpdate->bfNewTpmFirmwareInfo, &PpTpmUpdate->unErrorDetails);
		if (RC_SUCCESS != unReturnValue)
			break;

		if (!PpTpmUpdate->fValid)
		{
			switch (PpTpmUpdate->unErrorDetails)
			{
				case RC_E_CORRUPT_FW_IMAGE:
					unReturnValue = PpTpmUpdate->unErrorDetails;
					ERROR_STORE_FMT(unReturnValue, L"The provided firmware image is corrupt. (0x%.8X)", PpTpmUpdate->unErrorDetails);
					break;
				case RC_E_WRONG_FW_IMAGE:
					unReturnValue = PpTpmUpdate->unErrorDetails;
					ERROR_STORE_FMT(unReturnValue, L"The provided firmware image is not valid for the TPM. (0x%.8X)", PpTpmUpdate->unErrorDetails);
					break;
				case RC_E_NEWER_TOOL_REQUIRED:
					unReturnValue = PpTpmUpdate->unErrorDetails;
					ERROR_STORE_FMT(unReturnValue, L"A newer version of the tool is required to process the provided firmware image. (0x%.8X)", PpTpmUpdate->unErrorDetails);
					break;
				case RC_E_WRONG_DECRYPT_KEYS:
					unReturnValue = PpTpmUpdate->unErrorDetails;
					ERROR_STORE_FMT(unReturnValue, L"The provided firmware image is not valid for the TPM. (0x%.8X)", PpTpmUpdate->unErrorDetails);
					break;
				default:
					unReturnValue = RC_E_TPM_FIRMWARE_UPDATE;
					ERROR_STORE_FMT(unReturnValue, L"The firmware image check returned an unexpected value. (0x%.8X)", PpTpmUpdate->unErrorDetails);
					break;
			}
			break;
		}

		// Parse the new image and get the target version and the target family
		{
			BYTE* rgbIfxFirmwareImageStream = PpTpmUpdate->rgbFirmwareImage;
			IfxFirmwareImage sIfxFirmwareImage = {{0}};
			INT32 nIfxFirmwareImageSize = (INT32)PpTpmUpdate->unFirmwareImageSize;
			unsigned int unNewFirmwareVersionSize = RG_LEN(PpTpmUpdate->wszNewFirmwareVersion);

			unReturnValue = FirmwareImage_Unmarshal(&sIfxFirmwareImage, &rgbIfxFirmwareImageStream, &nIfxFirmwareImageSize);
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"Firmware image cannot be parsed.");
				break;
			}

			unReturnValue = Platform_StringCopy(PpTpmUpdate->wszNewFirmwareVersion, &unNewFirmwareVersionSize, sIfxFirmwareImage.wszTargetVersion);
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"Platform_StringCopy returned an unexpected value while copying the target firmware version.");
				break;
			}

			PpTpmUpdate->bTargetFamily = sIfxFirmwareImage.bTargetTpmFamily;
		}
	}
	WHILE_FALSE_END;

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

	return unReturnValue;
}

/**
 *	@brief		Prepare a firmware update for a TPM1.2 with (Deferred) Physical Presence.
 *	@details	This function will prepare the TPM1.2 to do a firmware update.
 *
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_TPM12_DEFERREDPP_REQUIRED	Physical Presence is locked and Deferred Physical Presence is not set
 *	@retval		RC_E_FAIL						An unexpected error occurred.
 *	@retval		...								Error codes from called functions.
 */
_Check_return_
unsigned int
CommandFlow_TpmUpdate_PrepareTPM12PhysicalPresence()
{
	unsigned int unReturnValue = RC_E_FAIL;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	do
	{
		// Deferred Physical Presence is not set so try to enable Physical Presence and set Deferred Physical Presence with following command sequence.
		// First try to enable the Physical Presence command, it may already be enabled
		unReturnValue = TSS_TSC_PhysicalPresence(TPM_PHYSICAL_PRESENCE_CMD_ENABLE);
		// In case this has already been done and lifetime was locked in TPM factory, the command above will fail with TPM_BAD_PARAMETER.
		// But in case Physical Presence is not locked yet, we can still perform all required actions, therefore this is not necessarily an error and we
		// should continue.
		if (RC_SUCCESS != unReturnValue && TPM_BAD_PARAMETER != (unReturnValue ^ RC_TPM_MASK))
		{
			ERROR_STORE(unReturnValue, L"Error calling TSS_TSC_PhysicalPresence(TPM_PHYSICAL_PRESENCE_CMD_ENABLE)");
			break;
		}

		// Try to set Physical Presence, may be locked
		unReturnValue = TSS_TSC_PhysicalPresence(TPM_PHYSICAL_PRESENCE_PRESENT);
		// In case Physical Presence is locked, the command above will fail with TPM_BAD_PARAMETER.
		// Since Deferred Physical Presence is also not set we must stop the update execution and return to the caller
		if (RC_SUCCESS != unReturnValue)
		{
			if (TPM_BAD_PARAMETER == (unReturnValue ^ RC_TPM_MASK))
				unReturnValue = RC_E_TPM12_DEFERREDPP_REQUIRED;

			ERROR_STORE(unReturnValue, L"Error calling TSS_TSC_PhysicalPresence(TPM_PHYSICAL_PRESENCE_PRESENT)");
			break;
		}

		{
			// Set Deferred Physical Presence bit
			UINT32 unSubCapSwapped = Platform_SwapBytes32(TPM_SD_DEFERREDPHYSICALPRESENCE);
			BYTE setValue[] = { 0x00, 0x00, 0x00, 0x01}; // TRUE
			unReturnValue = TSS_TPM_SetCapability(TPM_SET_STCLEAR_DATA, sizeof(unSubCapSwapped), (BYTE*)&unSubCapSwapped, sizeof(setValue), setValue);
			// If we manage to come to this call, the command should succeed. Therefore any error is really an error and should be logged and handled properly.
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"Error calling TSS_TPM_SetCapability(TPM_SET_STCLEAR_DATA)");
				break;
			}
		}
		unReturnValue = RC_SUCCESS;
	}
	WHILE_FALSE_END;

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

	return unReturnValue;
}

/**
 *	@brief		Processes a sequence of TPM update related commands to update the firmware.
 *	@details	This module processes the firmware update. Afterwards the result is returned to the calling module.
 *				The function utilizes the MicroTSS library.
 *
 *	@param		PpTpmUpdate			Pointer to an initialized IfxUpdate structure to be filled in
 *
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function. PpTpmUpdate is NULL or invalid.
 *	@retval		RC_E_FAIL			An unexpected error occurred.
 *	@retval		...					Error codes from called functions.
 */
_Check_return_
unsigned int
CommandFlow_TpmUpdate_UpdateFirmware(
	_Inout_ IfxUpdate* PpTpmUpdate)
{
	unsigned int unReturnValue = RC_E_FAIL;
	BOOL fValue = FALSE;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	do
	{
		ENUM_UPDATE_TYPES unUpdateType = UPDATE_TYPE_NONE;
		IfxFirmwareUpdateData sFirmwareUpdateData = {0};

		// Check parameters
		if (NULL == PpTpmUpdate ||
				STRUCT_TYPE_TpmUpdate != PpTpmUpdate->unType ||
				PpTpmUpdate->unSize != sizeof(IfxUpdate) ||
				STRUCT_SUBTYPE_PREPARE != PpTpmUpdate->unSubType)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			ERROR_STORE(unReturnValue, L"Bad parameter detected. TpmUpdate structure is not in the correct state.");
			break;
		}

		// Set TpmUpdate structure sub type and return value
		PpTpmUpdate->unSubType = STRUCT_SUBTYPE_UPDATE;
		PpTpmUpdate->unReturnCode = RC_E_FAIL;

		// Set the session handle for a TPM2.0 update flow if necessary
		if (PpTpmUpdate->sTpmState.attribs.tpm20)
			sFirmwareUpdateData.unSessionHandle = PpTpmUpdate->hPolicySession;

		// Try to set TPM Owner authentication hash for update with taking ownership if necessary
		if (PpTpmUpdate->sTpmState.attribs.tpm12)
		{
			// Get update type
			if (FALSE == PropertyStorage_GetUIntegerValueByKey(PROPERTY_UPDATE_TYPE, (unsigned int*)&unUpdateType))
			{
				unReturnValue = RC_E_FAIL;
				ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_GetUIntegerValueByKey failed to get property '%ls'.", PROPERTY_UPDATE_TYPE);
				break;
			}

			// Set TPM Owner authentication hash only in case of corresponding update type
			if (UPDATE_TYPE_TPM12_TAKEOWNERSHIP == unUpdateType)
			{
				unReturnValue = Platform_MemoryCopy(sFirmwareUpdateData.rgbOwnerAuthHash, sizeof(sFirmwareUpdateData.rgbOwnerAuthHash), s_ownerAuthData.authdata, sizeof(s_ownerAuthData.authdata));
				if (RC_SUCCESS != unReturnValue)
				{
					ERROR_STORE(unReturnValue, L"Platform_MemoryCopy returned an unexpected value while copying TPM Owner authentication hash.");
					break;
				}
			}
		}

		// Update firmware
		sFirmwareUpdateData.fnProgressCallback = &Response_ProgressCallback;
		sFirmwareUpdateData.fnUpdateStartedCallback = &CommandFlow_TpmUpdate_UpdateStartedCallback;
		sFirmwareUpdateData.rgbFirmwareImage = PpTpmUpdate->rgbFirmwareImage;
		sFirmwareUpdateData.unFirmwareImageSize = PpTpmUpdate->unFirmwareImageSize;
		if (TRUE == PropertyStorage_GetBooleanValueByKey(PROPERTY_DRY_RUN, &fValue) && TRUE == fValue)
		{
			PpTpmUpdate->unReturnCode = RC_SUCCESS;
			for (unsigned long long ullProgress = 25; ullProgress <= 100; ullProgress += 25)
			{
				Platform_SleepMicroSeconds(2*1000*1000);
				Response_ProgressCallback(ullProgress);
			}
		}
		else
			PpTpmUpdate->unReturnCode = FirmwareUpdate_UpdateImage(&sFirmwareUpdateData);
		unReturnValue = RC_SUCCESS;
		if (RC_SUCCESS != PpTpmUpdate->unReturnCode)
			break;

		// The firmware update completed successfully. Remove run data. Ignore errors: For example the tool might not have
		// access rights to remove the file.
		if (FileIO_Exists(TPM_FACTORY_UPD_RUNDATA_FILE))
		{
			IGNORE_RETURN_VALUE(FileIO_Remove(TPM_FACTORY_UPD_RUNDATA_FILE));
		}
	}
	WHILE_FALSE_END;

	// Try to close policy session in case of errors (only if session has already been started)
	if ((RC_SUCCESS != unReturnValue || RC_SUCCESS != PpTpmUpdate->unReturnCode) && 0 != PpTpmUpdate->hPolicySession)
	{
		IGNORE_RETURN_VALUE(TSS_TPM2_FlushContext(PpTpmUpdate->hPolicySession));
		PpTpmUpdate->hPolicySession = 0;
	}

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

	return unReturnValue;
}

/**
 *	@brief		Prepare a firmware update.
 *	@details	This function will prepare the TPM to do a firmware update.
 *
 *	@param		PpTpmUpdate			Pointer to an initialized IfxUpdate structure to be filled in
 *
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function.
 *	@retval		RC_E_FAIL			An unexpected error occurred.
 *	@retval		...					Error codes from called functions.
 */
_Check_return_
unsigned int
CommandFlow_TpmUpdate_PrepareFirmwareUpdate(
	_Inout_ IfxUpdate* PpTpmUpdate)
{
	unsigned int unReturnValue = RC_E_FAIL;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	do
	{
		// Check parameters
		if (NULL == PpTpmUpdate ||
				STRUCT_TYPE_TpmUpdate != PpTpmUpdate->unType ||
				PpTpmUpdate->unSize != sizeof(IfxUpdate) ||
				STRUCT_SUBTYPE_IS_UPDATABLE != PpTpmUpdate->unSubType)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			ERROR_STORE(unReturnValue, L"Bad parameter detected. TpmUpdate structure is not in the correct state.");
			break;
		}

		// Set TpmUpdate structure sub type and return value
		PpTpmUpdate->unSubType = STRUCT_SUBTYPE_PREPARE;
		PpTpmUpdate->unReturnCode = RC_E_FAIL;

		// Check which type of TPM is present or in which state
		if (PpTpmUpdate->sTpmState.attribs.bootLoader)
		{
			// No preparation needed
			PpTpmUpdate->unReturnCode = RC_SUCCESS;
			unReturnValue = RC_SUCCESS;
		}
		else if (PpTpmUpdate->sTpmState.attribs.tpm20)
		{
			// Prepare TPM2.0 update
			PpTpmUpdate->unReturnCode = FirmwareUpdate_PrepareTPM20Policy(&PpTpmUpdate->hPolicySession);
			unReturnValue = RC_SUCCESS;
		}
		else if (PpTpmUpdate->sTpmState.attribs.tpm12)
		{
			unsigned int unUpdateType = UPDATE_TYPE_NONE;

			// Check which type is given
			if (TRUE == PropertyStorage_GetUIntegerValueByKey(PROPERTY_UPDATE_TYPE, &unUpdateType))
			{
				if (UPDATE_TYPE_TPM12_DEFERREDPP == unUpdateType)
				{
					// Check if Deferred Physical Presence is set. If so we do not need to set it and can jump out.
					if (PpTpmUpdate->sTpmState.attribs.tpm12DeferredPhysicalPresence)
					{
						PpTpmUpdate->unReturnCode = RC_SUCCESS;
						unReturnValue = RC_SUCCESS;
					}
					else
					{
						// Prepare (deferred) physical presence based TPM1.2 update
						PpTpmUpdate->unReturnCode = CommandFlow_TpmUpdate_PrepareTPM12PhysicalPresence();
						unReturnValue = RC_SUCCESS;
					}
				}
				else if (UPDATE_TYPE_TPM12_TAKEOWNERSHIP == unUpdateType)
				{
					// Prepare owner based TPM1.2 update
					PpTpmUpdate->unReturnCode = CommandFlow_TpmUpdate_PrepareTPM12Ownership();
					unReturnValue = RC_SUCCESS;
				}
				else
				{
					unReturnValue = RC_E_FAIL;
					ERROR_STORE(unReturnValue, L"Unsupported Update type detected");
				}
			}
			else
			{
				unReturnValue = RC_E_FAIL;
				ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_GetUIntegerValueByKey failed to get property '%ls'.", PROPERTY_UPDATE_TYPE);
			}
		}
		else
		{
			unReturnValue = RC_E_FAIL;
			ERROR_STORE(unReturnValue, L"Unsupported TPM type detected");
		}
	}
	WHILE_FALSE_END;

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

	return unReturnValue;
}

/**
 *	@brief		Check if a firmware update is possible.
 *	@details	This function will check if the firmware is updatable with the given firmware image
 *
 *	@param		PpTpmUpdate				Pointer to an initialized IfxUpdate structure to be filled in
 *
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function.
 *	@retval		RC_E_INVALID_FW_OPTION	In case of an invalid firmware option argument.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval		...						Error codes from Micro TSS functions
 */
_Check_return_
unsigned int
CommandFlow_TpmUpdate_IsFirmwareUpdatable(
	_Inout_ IfxUpdate* PpTpmUpdate)
{
	unsigned int unReturnValue = RC_E_FAIL;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	do
	{
		// Check parameters
		if (NULL == PpTpmUpdate || PpTpmUpdate->unType != STRUCT_TYPE_TpmUpdate || PpTpmUpdate->unSize != sizeof(IfxUpdate))
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			ERROR_STORE(unReturnValue, L"Bad parameter detected. TpmUpdate structure is not in the correct state.");
			break;
		}

		// Set TpmUpdate structure sub type and return value
		PpTpmUpdate->unSubType = STRUCT_SUBTYPE_IS_UPDATABLE;
		PpTpmUpdate->unNewFirmwareValid = GENERIC_TRISTATE_STATE_NA;
		PpTpmUpdate->unReturnCode = RC_E_FAIL;

		// Check if TPM is updatable regarding the count
		{
			ENUM_UPDATE_TYPES unUpdateType = UPDATE_TYPE_NONE;

			// Get update type
			if (FALSE == PropertyStorage_GetUIntegerValueByKey(PROPERTY_UPDATE_TYPE, (unsigned int*)&unUpdateType))
			{
				unReturnValue = RC_E_FAIL;
				break;
			}

			// Check if TPM1.2 is detected
			if (PpTpmUpdate->sTpmState.attribs.tpm12)
			{
				// Check if the correct update type is set
				if (UPDATE_TYPE_TPM12_DEFERREDPP != unUpdateType && UPDATE_TYPE_TPM12_TAKEOWNERSHIP != unUpdateType)
				{
					PpTpmUpdate->unReturnCode = RC_E_INVALID_UPDATE_OPTION;
					ERROR_STORE(PpTpmUpdate->unReturnCode, L"Wrong update type detected. The underlying TPM is a TPM1.2.");
					unReturnValue = RC_SUCCESS;
					break;
				}

				// Check if TPM already has an owner
				if (PpTpmUpdate->sTpmState.attribs.tpm12owner)
				{
					PpTpmUpdate->unReturnCode = RC_E_TPM12_OWNED;
					ERROR_STORE(PpTpmUpdate->unReturnCode, L"TPM1.2 Owner detected. Update cannot be done.");
					unReturnValue = RC_SUCCESS;
					break;
				}
			}

			// Check if TPM2.0 is detected and correct update type is set
			if (PpTpmUpdate->sTpmState.attribs.tpm20 && UPDATE_TYPE_TPM20_EMPTYPLATFORMAUTH != unUpdateType)
			{
				PpTpmUpdate->unReturnCode = RC_E_INVALID_UPDATE_OPTION;
				ERROR_STORE(PpTpmUpdate->unReturnCode, L"Wrong update type detected. The underlying TPM is a TPM2.0.");
				unReturnValue = RC_SUCCESS;
				break;
			}

			// Check if restart is required
			if (PpTpmUpdate->sTpmState.attribs.tpm20restartRequired)
			{
				PpTpmUpdate->unReturnCode = RC_E_RESTART_REQUIRED;
				ERROR_STORE_FMT(PpTpmUpdate->unReturnCode, L"System must be restarted. (0x%.8lX)", PpTpmUpdate->sTpmState.attribs);
				unReturnValue = RC_SUCCESS;
				break;
			}

			// Check if TPM is in failure mode
			if (PpTpmUpdate->sTpmState.attribs.tpm20InFailureMode)
			{
				PpTpmUpdate->unReturnCode = RC_E_TPM20_FAILURE_MODE;
				ERROR_STORE_FMT(PpTpmUpdate->unReturnCode, L"TPM2.0 is in failure mode. (0x%.8lX)", PpTpmUpdate->sTpmState.attribs);
				unReturnValue = RC_SUCCESS;
				break;
			}

			// Check if updatable
			if (PpTpmUpdate->unRemainingUpdates == 0)
			{
				PpTpmUpdate->unReturnCode = RC_E_FW_UPDATE_BLOCKED;
				ERROR_STORE_FMT(PpTpmUpdate->unReturnCode, L"Image is not updatable. (0x%.8lX | 0x%.8X)", PpTpmUpdate->sTpmState.attribs, PpTpmUpdate->unRemainingUpdates);
				unReturnValue = RC_SUCCESS;
				break;
			}
		}

		// Get firmware path from property storage and load file
		{
			wchar_t wszFirmwareImagePath[MAX_PATH] = {0};
			unsigned int unFirmwareImagePathSize = RG_LEN(wszFirmwareImagePath);

			if (FALSE == PropertyStorage_GetValueByKey(PROPERTY_FIRMWARE_PATH, wszFirmwareImagePath, &unFirmwareImagePathSize))
			{
				unReturnValue = RC_E_FAIL;
				ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_GetValueByKey failed to get property '%ls'.", PROPERTY_FIRMWARE_PATH);
				break;
			}

			unReturnValue = FileIO_ReadFileToBuffer(wszFirmwareImagePath, &PpTpmUpdate->rgbFirmwareImage, &PpTpmUpdate->unFirmwareImageSize);
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE_FMT(RC_E_INVALID_FW_OPTION, L"Failed to load the firmware image (%ls). (0x%.8X)", wszFirmwareImagePath, unReturnValue);
				unReturnValue = RC_E_INVALID_FW_OPTION;
				break;
			}
		}

		unReturnValue = CommandFlow_TpmUpdate_IsTpmUpdatableWithFirmware(PpTpmUpdate);
		if (RC_SUCCESS != unReturnValue)
		{
			PpTpmUpdate->unNewFirmwareValid = GENERIC_TRISTATE_STATE_NO;
			PpTpmUpdate->unReturnCode = unReturnValue;
			unReturnValue = RC_SUCCESS;
			break;
		}

		PpTpmUpdate->unNewFirmwareValid = GENERIC_TRISTATE_STATE_YES;
		PpTpmUpdate->unReturnCode = RC_SUCCESS;
		unReturnValue = RC_SUCCESS;
	}
	WHILE_FALSE_END;

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

	return unReturnValue;
}

/**
 *	@brief		Take TPM Ownership with hard coded hash value.
 *	@details	The corresponding TPM Owner authentication is described in the user manual.
 *
 *	@retval		RC_SUCCESS						TPM Ownership was taken successfully.
 *	@retval		RC_E_FAIL						An unexpected error occurred.
 *	@retval		RC_E_TPM12_DISABLED_DEACTIVATED	In case the TPM is disabled and deactivated.
 *	@retval		...								Error codes from Micro TSS functions
 */
_Check_return_
unsigned int
CommandFlow_TpmUpdate_PrepareTPM12Ownership()
{
	unsigned int unReturnValue = RC_SUCCESS;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	do
	{
		TPM_PUBKEY sTpmPubKey = {{0}};
		BYTE rgbPublicExponent[] = {0x01, 0x00, 0x01};
		// Encrypted TPM Owner authentication hash buffer
		BYTE rgbEncryptedOwnerAuthHash[MAX_RSA_KEY_BYTES] = {0};
		unsigned int unEncryptedOwnerHashSize = RG_LEN(rgbEncryptedOwnerAuthHash);
		// Storage Root Key hash value = SRK authentication value
		BYTE rgbSrkWellKnownAuth[] = SRK_WELL_KNOWN_AUTH;
		// Encrypted SRK hash buffer
		BYTE rgbEncryptedSrkHash[MAX_RSA_KEY_BYTES] = {0};
		unsigned int unEncryptedSrkHashSize = RG_LEN(rgbEncryptedSrkHash);
		// OIAP Session parameters
		// Authorization session handle
		TPM_AUTHHANDLE unAuthHandle = 0;
		// Even nonce to be generated by TPM prior TPM_TakeOwnership
		TPM_NONCE sAuthLastNonceEven = {{0}};
		// Take ownership parameters
		TPM_KEY sSrkParams = {{0}};
		TPM_KEY sSrkKey = {{0}};

		// Get Public Endorsement Key
		unReturnValue = TSS_TPM_ReadPubEK(&sTpmPubKey);
		if (unReturnValue != RC_SUCCESS)
		{
			ERROR_STORE(unReturnValue, L"Read Public Endorsement Key failed!");
			break;
		}

		// Encrypt TPM Owner authentication hash
		unReturnValue = Crypt_EncryptRSA(
							CRYPT_ES_RSAESOAEP_SHA1_MGF1,
							SHA1_DIGEST_SIZE, s_ownerAuthData.authdata, // Decrypted buffer
							sTpmPubKey.pubKey.keyLength, sTpmPubKey.pubKey.key, // Public modulus
							sizeof(rgbPublicExponent), rgbPublicExponent, // Public exponent
							&unEncryptedOwnerHashSize, rgbEncryptedOwnerAuthHash);
		if (RC_SUCCESS != unReturnValue)
		{
			ERROR_STORE(unReturnValue, L"TPM Owner authentication hash encryption failed!");
			break;
		}

		// Encrypt TPM SRK authentication hash (well-known authentication)
		unReturnValue = Crypt_EncryptRSA(
							CRYPT_ES_RSAESOAEP_SHA1_MGF1,
							SHA1_DIGEST_SIZE, rgbSrkWellKnownAuth, // Decrypted buffer
							sTpmPubKey.pubKey.keyLength, sTpmPubKey.pubKey.key, // Public modulus
							sizeof(rgbPublicExponent), rgbPublicExponent, // Public exponent
							&unEncryptedSrkHashSize, rgbEncryptedSrkHash);
		if (RC_SUCCESS != unReturnValue)
		{
			ERROR_STORE(unReturnValue, L"SRK authentication hash encryption failed!");
			break;
		}

		// Get Authorization Session handle and even nonce
		unReturnValue = TSS_TPM_OIAP(&unAuthHandle, &sAuthLastNonceEven);
		if (unReturnValue != RC_SUCCESS)
		{
			ERROR_STORE(unReturnValue, L"Get Authorization Session handle failed!");
			break;
		}

		// Initialize SRK parameters
		sSrkParams.ver.major = 1;
		sSrkParams.ver.minor = 1;
		sSrkParams.ver.revMajor = 0;
		sSrkParams.ver.revMinor = 0;
		sSrkParams.keyUsage = TPM_KEY_STORAGE;
		sSrkParams.keyFlags = 0;
		sSrkParams.authDataUsage = TPM_AUTH_ALWAYS;
		sSrkParams.algorithmParms.algorithmID = 0x00000001;
		sSrkParams.algorithmParms.encScheme = CRYPT_ES_RSAESOAEP_SHA1_MGF1;
		sSrkParams.algorithmParms.sigScheme = TPM_SS_NONE;
		sSrkParams.algorithmParms.parmSize = sizeof(TPM_RSA_KEY_PARMS);
		sSrkParams.algorithmParms.parms.keyLength = 0x800;
		sSrkParams.algorithmParms.parms.numPrimes = 2;
		sSrkParams.algorithmParms.parms.exponentSize = 0;
		sSrkParams.PCRInfoSize = 0;
		sSrkParams.pubKey.keyLength = 0;
		sSrkParams.encSize = 0;

		unReturnValue = TSS_TPM_TakeOwnership(
							rgbEncryptedOwnerAuthHash, unEncryptedSrkHashSize, // Encrypted TPM Owner authentication hash
							rgbEncryptedSrkHash, unEncryptedSrkHashSize, // Encrypted SRK authentication hash
							&sSrkParams, unAuthHandle, &s_ownerAuthData,
							&sAuthLastNonceEven, &sSrkKey);

		if (RC_SUCCESS != unReturnValue || 0 == sSrkKey.pubKey.keyLength)
		{
			ERROR_STORE(unReturnValue, L"Take Ownership failed!");
			break;
		}
	}
	WHILE_FALSE_END;

	// Map return value in case TPM is disabled or deactivated to corresponding tool exit code
	if ((TPM_DEACTIVATED == (unReturnValue ^ RC_TPM_MASK)) ||
			(TPM_DISABLED == (unReturnValue ^ RC_TPM_MASK)))
	{
		unReturnValue = RC_E_TPM12_DISABLED_DEACTIVATED;
		ERROR_STORE(unReturnValue, L"Take Ownership failed!");
	}

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

	return unReturnValue;
}

/**
 *	@brief		Parses the update configuration settings
 *	@details	Parses the update configuration settings for a settings file based update flow
 *
 *	@param		PwszSection			Pointer to a wide character array containing the current section
 *	@param		PunSectionSize		Size of the section buffer in elements including the zero termination
 *	@param		PwszKey				Pointer to a wide character array containing the current key
 *	@param		PunKeySize			Size of the key buffer in elements including the zero termination
 *	@param		PwszValue			Pointer to a wide character array containing the current value
 *	@param		PunValueSize		Size of the value buffer in elements including the zero termination
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function. It is NULL or empty.
 *	@retval		RC_E_FAIL			An unexpected error occurred.
 */
_Check_return_
unsigned int
CommandFlow_TpmUpdate_Parse(
	_In_z_count_(PunSectionSize)	const wchar_t*	PwszSection,
	_In_							unsigned int	PunSectionSize,
	_In_z_count_(PunKeySize)		const wchar_t*	PwszKey,
	_In_							unsigned int	PunKeySize,
	_In_z_count_(PunValueSize)		const wchar_t*	PwszValue,
	_In_							unsigned int	PunValueSize)
{
	unsigned int unReturnValue = RC_E_FAIL;
	const wchar_t wszErrorMsgFormat[] = L"PropertyStorage_AddKeyUIntegerValuePair failed while updating the property '%ls'.";

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	UNREFERENCED_PARAMETER(PunValueSize);

	do
	{
		// Check parameters
		if (PLATFORM_STRING_IS_NULL_OR_EMPTY(PwszSection) ||
				0 == PunSectionSize ||
				PLATFORM_STRING_IS_NULL_OR_EMPTY(PwszKey) ||
				0 == PunKeySize ||
				PLATFORM_STRING_IS_NULL_OR_EMPTY(PwszValue) ||
				0 == PunValueSize)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			ERROR_STORE(unReturnValue, L"One or more input parameters are NULL or empty.");
			break;
		}

		// Section Update Type
		if (0 == Platform_StringCompare(PwszSection, CONFIG_SECTION_UPDATE_TYPE, PunSectionSize, TRUE))
		{
			// Check setting tpm12
			if (0 == Platform_StringCompare(PwszKey, CONFIG_UPDATE_TYPE_TPM12, PunKeySize, TRUE))
			{
				if (0 == Platform_StringCompare(PwszValue, CMD_UPDATE_OPTION_TPM12_DEFERREDPP, PunValueSize, TRUE))
				{
					if (!PropertyStorage_AddKeyUIntegerValuePair(PROPERTY_CONFIG_FILE_UPDATE_TYPE12, UPDATE_TYPE_TPM12_DEFERREDPP))
					{
						ERROR_STORE_FMT(unReturnValue, wszErrorMsgFormat, PROPERTY_CONFIG_FILE_UPDATE_TYPE12);
						break;
					}
				}
				else if(0 == Platform_StringCompare(PwszValue, CMD_UPDATE_OPTION_TPM12_TAKEOWNERSHIP, PunValueSize, TRUE))
				{
					if (!PropertyStorage_AddKeyUIntegerValuePair(PROPERTY_CONFIG_FILE_UPDATE_TYPE12, UPDATE_TYPE_TPM12_TAKEOWNERSHIP))
					{
						ERROR_STORE_FMT(unReturnValue, wszErrorMsgFormat, PROPERTY_CONFIG_FILE_UPDATE_TYPE12);
						break;
					}
				}
				else
				{
					unReturnValue = RC_E_INVALID_SETTING;
					ERROR_STORE(unReturnValue, L"Invalid update config-file value for setting CONFIG_UPDATE_TYPE_TPM12 found");
					break;
				}
			}
			// Check setting tpm20
			if (0 == Platform_StringCompare(PwszKey, CONFIG_UPDATE_TYPE_TPM20, PunKeySize, TRUE))
			{
				if(0 == Platform_StringCompare(PwszValue, CMD_UPDATE_OPTION_TPM20_EMPTYPLATFORMAUTH, PunValueSize, TRUE))
				{
					if (!PropertyStorage_AddKeyUIntegerValuePair(PROPERTY_CONFIG_FILE_UPDATE_TYPE20, UPDATE_TYPE_TPM20_EMPTYPLATFORMAUTH))
					{
						ERROR_STORE_FMT(unReturnValue, wszErrorMsgFormat, PROPERTY_CONFIG_FILE_UPDATE_TYPE20);
						break;
					}
				}
				else
				{
					unReturnValue = RC_E_INVALID_SETTING;
					ERROR_STORE(unReturnValue, L"Invalid update config-file value for setting CONFIG_UPDATE_TYPE_TPM12 found");
					break;
				}
			}

			// Unknown setting in current section ignore it
			unReturnValue = RC_SUCCESS;
			break;
		}

		// Section Target Firmware
		if (0 == Platform_StringCompare(PwszSection, CONFIG_SECTION_TARGET_FIRMWARE, PunSectionSize, TRUE))
		{
			// Check setting versionLPC
			if (0 == Platform_StringCompare(PwszKey, CONFIG_TARGET_FIRMWARE_VERSION_LPC, PunKeySize, TRUE))
			{
				if (!PropertyStorage_AddKeyValuePair(PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_LPC, PwszValue))
				{
					ERROR_STORE_FMT(unReturnValue, wszErrorMsgFormat, PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_LPC);
					break;
				}
			}

			// Check setting versionSPI
			if (0 == Platform_StringCompare(PwszKey, CONFIG_TARGET_FIRMWARE_VERSION_SPI, PunKeySize, TRUE))
			{
				if (!PropertyStorage_AddKeyValuePair(PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_SPI, PwszValue))
				{
					ERROR_STORE_FMT(unReturnValue, wszErrorMsgFormat, PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_SPI);
					break;
				}
			}

			// Unknown setting in current section ignore it
			unReturnValue = RC_SUCCESS;
		}

		// Section Firmware Folder
		if (0 == Platform_StringCompare(PwszSection, CONFIG_SECTION_FIRMWARE_FOLDER, PunSectionSize, TRUE))
		{
			// Check setting version
			if (0 == Platform_StringCompare(PwszKey, CONFIG_FIRMWARE_FOLDER_PATH, PunKeySize, TRUE))
			{
				if (!PropertyStorage_AddKeyValuePair(PROPERTY_CONFIG_FIRMWARE_FOLDER_PATH, PwszValue))
				{
					ERROR_STORE_FMT(unReturnValue, wszErrorMsgFormat, PROPERTY_CONFIG_FIRMWARE_FOLDER_PATH);
					break;
				}
			}

			// Unknown setting in current section ignore it
			unReturnValue = RC_SUCCESS;
		}

		// Unknown section ignore it
		unReturnValue = RC_SUCCESS;
	}
	WHILE_FALSE_END;

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

	return unReturnValue;
}

/**
 *	@brief		Initialize config settings parsing
 *	@details
 *
 *	@retval		RC_SUCCESS	The operation completed successfully.
 *	@retval		RC_E_FAIL	An unexpected error occurred.
 */
_Check_return_
unsigned int
CommandFlow_TpmUpdate_InitializeParsing()
{
	unsigned int unReturnValue = RC_SUCCESS;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);
	// Nothing to initialize here
	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

	return unReturnValue;
}

/**
 *	@brief		Finalize config settings parsing
 *	@details
 *
 *	@param		PunReturnValue	Current return code which can be overwritten here.
 *	@retval		PunReturnValue	In case PunReturnValue is not equal to RC_SUCCESS
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		RC_E_FAIL		An unexpected error occurred.
 */
_Check_return_
unsigned int
CommandFlow_TpmUpdate_FinalizeParsing(
	_In_ unsigned int PunReturnValue)
{
	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	do
	{
		unsigned int unIndex = 0;
		wchar_t* prgwszMandatoryProperties[] = {
			PROPERTY_CONFIG_FILE_UPDATE_TYPE12,
			PROPERTY_CONFIG_FILE_UPDATE_TYPE20,
			PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_LPC,
			PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_SPI,
			PROPERTY_CONFIG_FIRMWARE_FOLDER_PATH,
			L""};

		if (RC_SUCCESS != PunReturnValue)
			break;

		while (0 != Platform_StringCompare(prgwszMandatoryProperties[unIndex], L"", RG_LEN(L""), FALSE))
		{
			// Check if all mandatory settings were parsed
			if (!PropertyStorage_ExistsElement(prgwszMandatoryProperties[unIndex]))
			{
				PunReturnValue = RC_E_INVALID_SETTING;
				ERROR_STORE_FMT(PunReturnValue, L"TPM update config file: %ls is mandatory", prgwszMandatoryProperties[unIndex]);
				break;
			}
			unIndex++;
		}
	}
	WHILE_FALSE_END;

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, PunReturnValue);

	return PunReturnValue;
}

/**
 *	@brief		Parse the update config settings file
 *	@details
 *
 *	@param		PpTpmUpdate							Contains information about the current TPM and can be filled up with information for the
 *													corresponding Current return code which can be overwritten here.
 *	@retval		PunReturnValue						In case PunReturnValue is not equal to RC_SUCCESS
 *	@retval		RC_SUCCESS							The operation completed successfully.
 *	@retval		RC_E_FAIL							An unexpected error occurred.
 *	@retval		RC_E_INVALID_CONFIG_OPTION			An config file was given that cannot be opened.
 *	@retval		RC_E_FIRMWARE_UPDATE_NOT_FOUND		A firmware update for the current TPM version cannot be found.
 */
_Check_return_
unsigned int
CommandFlow_TpmUpdate_ProceedUpdateConfig(
	_Inout_ IfxUpdate* PpTpmUpdate)
{
	unsigned int unReturnValue = RC_E_FAIL;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	do
	{
		wchar_t wszConfigFilePath[MAX_STRING_1024] = {0};
		unsigned int unConfigFileNamePathSize = RG_LEN(wszConfigFilePath);

		// Check parameters
		if (NULL == PpTpmUpdate || PpTpmUpdate->unType != STRUCT_TYPE_TpmUpdate || PpTpmUpdate->unSize != sizeof(IfxUpdate))
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			ERROR_STORE(unReturnValue, L"Bad parameter detected. TpmUpdate structure is not in the correct state.");
			break;
		}

		// Set TpmUpdate structure sub type and return value
		PpTpmUpdate->unSubType = STRUCT_SUBTYPE_IS_UPDATABLE;
		PpTpmUpdate->unNewFirmwareValid = GENERIC_TRISTATE_STATE_NA;
		PpTpmUpdate->unReturnCode = RC_E_FAIL;

		// Get config file path from property storage
		if (FALSE == PropertyStorage_GetValueByKey(PROPERTY_CONFIG_FILE_PATH, wszConfigFilePath, &unConfigFileNamePathSize))
		{
			unReturnValue = RC_E_FAIL;
			ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_GetValueByKey failed to get property '%ls'.", PROPERTY_CONFIG_FILE_PATH);
			break;
		}

		// Check if file exists
		if (!FileIO_Exists(wszConfigFilePath))
		{
			unReturnValue = RC_E_INVALID_CONFIG_OPTION;
			ERROR_STORE_FMT(unReturnValue, L"The config file '%ls' does not exist", wszConfigFilePath);
			break;
		}

		// Parse config file using the config module
		unReturnValue = Config_ParseCustom(
							wszConfigFilePath,
							&CommandFlow_TpmUpdate_InitializeParsing,
							&CommandFlow_TpmUpdate_FinalizeParsing,
							&CommandFlow_TpmUpdate_Parse);
		if (RC_SUCCESS != unReturnValue)
		{
			ERROR_STORE(unReturnValue, L"Error while parsing the config file of the config option.");
			break;
		}

		{
			unsigned int unUsedFirmwareImageSize = RG_LEN (PpTpmUpdate->wszUsedFirmwareImage);
			wchar_t wszConfigSettingFirmwarePath[MAX_PATH] = {0};
			unsigned int unConfigSettingFirmwarePathSize = RG_LEN (wszConfigSettingFirmwarePath);
			wchar_t wszTargetVersion[MAX_NAME] = {0};
			unsigned int unTargetVersionSize = RG_LEN(wszTargetVersion);
			wchar_t wszSourceFamily[MAX_NAME] = {0};
			unsigned int unSourceFamilySize = RG_LEN(wszSourceFamily);
			wchar_t wszTargetFamily[MAX_NAME] = {0};
			unsigned int unTargetFamilySize = RG_LEN(wszTargetFamily);
			ENUM_UPDATE_TYPES unUpdateType = UPDATE_TYPE_NONE;

			#define TPM_FIRMWARE_FILE_NAME_PATTERN L"%ls_%ls_to_%ls_%ls.BIN"

			if (!PpTpmUpdate->sTpmState.attribs.bootLoader)
			{
				// Check if TPM is SPI or LPC
				if ((PpTpmUpdate->wszVersionName[0] == L'6' || PpTpmUpdate->wszVersionName[0] == L'7') && PpTpmUpdate->wszVersionName[1] == L'.')
				{
					// Get Target Version String
					if (!PropertyStorage_GetValueByKey(PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_SPI, wszTargetVersion, &unTargetVersionSize))
					{
						unReturnValue = RC_E_FAIL;
						ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_GetValueByKey failed to get property '%ls'.", PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_SPI);
						break;
					}
				}
				else if ((PpTpmUpdate->wszVersionName[0] == L'4' || PpTpmUpdate->wszVersionName[0] == L'5') && PpTpmUpdate->wszVersionName[1] == L'.')
				{
					// Get Target Version String
					if (!PropertyStorage_GetValueByKey(PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_LPC, wszTargetVersion, &unTargetVersionSize))
					{
						unReturnValue = RC_E_FAIL;
						ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_GetValueByKey failed to get property '%ls'.", PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_LPC);
						break;
					}
				}
				else
				{
					unReturnValue = RC_E_UNSUPPORTED_CHIP;
					ERROR_STORE_FMT(unReturnValue, L"The detected TPM version (%ls) is not supported.", PpTpmUpdate->wszVersionName);
					break;
				}

				// Check if firmware is already up to date
				if (0 == Platform_StringCompare(wszTargetVersion, PpTpmUpdate->wszVersionName, unTargetFamilySize, FALSE))
				{
					PpTpmUpdate->unNewFirmwareValid = GENERIC_TRISTATE_STATE_NO;
					PpTpmUpdate->unReturnCode = RC_E_ALREADY_UP_TO_DATE;
					unReturnValue = RC_SUCCESS;
					break;
				}

				// Get firmware folder path
				if (!PropertyStorage_GetValueByKey(PROPERTY_CONFIG_FIRMWARE_FOLDER_PATH, wszConfigSettingFirmwarePath, &unConfigSettingFirmwarePathSize))
				{
					unReturnValue = RC_E_FAIL;
					ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_GetValueByKey failed to get property '%ls'.", PROPERTY_CONFIG_FIRMWARE_FOLDER_PATH);
					break;
				}

				// Detect TPM source family
				if (PpTpmUpdate->sTpmState.attribs.tpm12)
					unReturnValue = Platform_StringCopy(wszSourceFamily, &unSourceFamilySize, TPM12_FAMILY_STRING);
				else if (PpTpmUpdate->sTpmState.attribs.tpm20)
					unReturnValue = Platform_StringCopy(wszSourceFamily, &unSourceFamilySize, TPM20_FAMILY_STRING);
				else
					unReturnValue = RC_E_FAIL;
				if (RC_SUCCESS != unReturnValue)
				{
					unReturnValue = RC_E_FAIL;
					ERROR_STORE(unReturnValue, L"CommandFlow_TpmUpdate_ProceedUpdateConfig failed while detecting the TPM source family.");
					break;
				}

				// Get TPM target family from target version
				if ((wszTargetVersion[0] == L'4' || wszTargetVersion[0] == L'6') && wszTargetVersion[1] == L'.')
					unReturnValue = Platform_StringCopy(wszTargetFamily, &unTargetFamilySize, TPM12_FAMILY_STRING);
				else if ((wszTargetVersion[0] == L'5' || wszTargetVersion[0] == L'7') && wszTargetVersion[1] == L'.')
					unReturnValue = Platform_StringCopy(wszTargetFamily, &unTargetFamilySize, TPM20_FAMILY_STRING);
				else
				{
					unReturnValue = RC_E_INVALID_SETTING;
					ERROR_STORE_FMT(unReturnValue, L"The configuration file contains an unsupported value (%ls) in either the TargetFirmware/version_SLB966x or TargetFirmware/version_SLB9670 field.",
						PpTpmUpdate->wszVersionName);
					break;
				}
				if (RC_SUCCESS != unReturnValue)
				{
					ERROR_STORE(unReturnValue, L"Platform_StringCopy failed while detecting the TPM target family");
					break;
				}

				// Construct firmware binary file path regarding the naming convention of update images
				// Fill in the firmware file name template
				unReturnValue = Platform_StringFormat(
									PpTpmUpdate->wszUsedFirmwareImage,
									&unUsedFirmwareImageSize,
									TPM_FIRMWARE_FILE_NAME_PATTERN,
									wszSourceFamily,
									PpTpmUpdate->wszVersionName,
									wszTargetFamily,
									wszTargetVersion);

				if (RC_SUCCESS != unReturnValue)
				{
					unReturnValue = RC_E_FAIL;
					ERROR_STORE(unReturnValue, L"Platform_StringFormat returned an unexpected value while composing the firmware image file path.");
					break;
				}

				// Compose firmware folder
				{
					wchar_t wszFirmwareFilePath[MAX_STRING_1024] = {0};
					unsigned int unFirmwareFilePathSize = RG_LEN(wszFirmwareFilePath);
					unsigned int unIndex = 0, unLastFolderIndex = 0;

					// Copy config file path to destination buffer
					unReturnValue = Platform_StringCopy(wszFirmwareFilePath, &unFirmwareFilePathSize, wszConfigFilePath);
					if (RC_SUCCESS != unReturnValue)
					{
						ERROR_STORE(unReturnValue, L"Platform_StringCopy returned an unexpected value.");
						break;
					}

					// Cut off file part
					for (unIndex = 0; unIndex < unFirmwareFilePathSize; unIndex++)
					{
						if (wszFirmwareFilePath[unIndex] == L'\\' ||
							wszFirmwareFilePath[unIndex] == L'/')
							unLastFolderIndex = unIndex;
					}

					// Check if index is zero means only cfg file name given or config file name in Linux root
					if (unLastFolderIndex == 0)
					{
						// If cfg file is placed in Linux root only remove the name not the root slash otherwize add a relative folder "." at the beginning
						if (wszFirmwareFilePath[unLastFolderIndex] == L'/')
							unLastFolderIndex++;
						else
							wszFirmwareFilePath[unLastFolderIndex++] = L'.';
					}

					// Set string zero termination
					wszFirmwareFilePath[unLastFolderIndex] = L'\0';

					// Check if config setting firmware path is not the actual folder
					if (0 != Platform_StringCompare(wszConfigSettingFirmwarePath, L".", RG_LEN(L"."), FALSE) &&
						0 != Platform_StringCompare(wszConfigSettingFirmwarePath, L"./", RG_LEN(L"./"), FALSE) &&
						0 != Platform_StringCompare(wszConfigSettingFirmwarePath, L".\\", RG_LEN(L".\\"), FALSE))
					{
						// If so add the config setting firmware folder path to the cfg file folder part
						unFirmwareFilePathSize = RG_LEN(wszFirmwareFilePath);
						unReturnValue = Platform_StringConcatenatePaths(wszFirmwareFilePath, &unFirmwareFilePathSize, wszConfigSettingFirmwarePath);
						if (RC_SUCCESS != unReturnValue)
						{
							ERROR_STORE(unReturnValue, L"Platform_StringConcatenate returned an unexpected value while composing the firmware image file path.");
							break;
						}
					}

					// Add the filled firmware file name template to the composed folder
					unFirmwareFilePathSize = RG_LEN(wszFirmwareFilePath);
					unReturnValue = Platform_StringConcatenatePaths(wszFirmwareFilePath, &unFirmwareFilePathSize, PpTpmUpdate->wszUsedFirmwareImage);
					if (RC_SUCCESS != unReturnValue)
					{
						ERROR_STORE(unReturnValue, L"Platform_StringConcatenate returned an unexpected value while composing the firmware image file path.");
						break;
					}

					// Check if firmware image exists
					if (!FileIO_Exists(wszFirmwareFilePath))
					{
						unReturnValue = RC_E_FIRMWARE_UPDATE_NOT_FOUND;
						ERROR_STORE_FMT(unReturnValue, L"No firmware image found to update the current TPM firmware. (%ls)", wszFirmwareFilePath);
						break;
					}

					// Set property storage attributes
					// Get config file TPM update type depending on the source family version
					if (PpTpmUpdate->sTpmState.attribs.tpm12)
					{
						// Get the update type for TPM1.2 from the config file
						if (!PropertyStorage_GetUIntegerValueByKey(PROPERTY_CONFIG_FILE_UPDATE_TYPE12, (unsigned int*)&unUpdateType))
						{
							unReturnValue = RC_E_FAIL;
							ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_GetUIntegerValueByKey failed to get property '%ls'.", PROPERTY_CONFIG_FILE_UPDATE_TYPE12);
							break;
						}
					}
					else
					{
						// Get the update type for TPM2.0 from the config file
						if (!PropertyStorage_GetUIntegerValueByKey(PROPERTY_CONFIG_FILE_UPDATE_TYPE20, (unsigned int*)&unUpdateType))
						{
							unReturnValue = RC_E_FAIL;
							ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_GetUIntegerValueByKey failed to get property '%ls'.", PROPERTY_CONFIG_FILE_UPDATE_TYPE20);
							break;
						}
					}
					// Set the update type
					if (!PropertyStorage_ChangeUIntegerValueByKey(PROPERTY_UPDATE_TYPE, unUpdateType))
					{
						unReturnValue = RC_E_FAIL;
						ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_ChangeUIntegerValueByKey failed to change property '%ls'.", PROPERTY_UPDATE_TYPE);
						break;
					}
					// Set the firmware file path
					if (!PropertyStorage_AddKeyValuePair(PROPERTY_FIRMWARE_PATH, wszFirmwareFilePath))
					{
						unReturnValue = RC_E_FAIL;
						ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_AddKeyValuePair failed to add property '%ls'.", PROPERTY_FIRMWARE_PATH);
						break;
					}

					// Remember that update is done through config file.
					s_fUpdateThroughConfigFile = TRUE;
				}
			}
			else
			{
				// Config-file properties will not be evaluated if TPM is in boot loader mode
				// Instead the firmware image from TPM_FACTORY_UPD_RUNDATA_FILE will be used.
				if (FileIO_Exists(TPM_FACTORY_UPD_RUNDATA_FILE))
				{
					wchar_t* pwszFirmwareImage = NULL;
					unsigned int unFirmwareImageSize = 0;
					unsigned int unIndex = 0;
					wchar_t* pwszLine = NULL;
					unsigned int unLineSize = 0;

					if (RC_SUCCESS != FileIO_ReadFileToStringBuffer(TPM_FACTORY_UPD_RUNDATA_FILE, &pwszFirmwareImage, &unFirmwareImageSize))
					{
						unReturnValue = RC_E_FAIL;
						Platform_MemoryFree((void**)&pwszFirmwareImage);
						ERROR_STORE_FMT(unReturnValue, L"Unexpected error occurred while reading file '%ls'", TPM_FACTORY_UPD_RUNDATA_FILE);
						break;
					}

					// Get line from buffer
					unReturnValue = Utility_StringGetLine(pwszFirmwareImage, unFirmwareImageSize, &unIndex, &pwszLine, &unLineSize);
					if (RC_SUCCESS != unReturnValue)
					{
						if (RC_E_END_OF_STRING == unReturnValue)
							unReturnValue = RC_SUCCESS;
						else
							ERROR_STORE(unReturnValue, L"Unexpected error occurred while parsing the rundata file content.");
						Platform_MemoryFree((void**)&pwszFirmwareImage);
						Platform_MemoryFree((void**)&pwszLine);
						break;
					}
					Platform_MemoryFree((void**)&pwszFirmwareImage);

					// Set the firmware file path
					if (!PropertyStorage_AddKeyValuePair(PROPERTY_FIRMWARE_PATH, pwszLine))
					{
						unReturnValue = RC_E_FAIL;
						Platform_MemoryFree((void**)&pwszLine);
						ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_AddKeyValuePair failed to add property '%ls'.", PROPERTY_FIRMWARE_PATH);
						break;
					}
					Platform_MemoryFree((void**)&pwszLine);
				}
				else
				{
					// Cannot resume the firmware update without TPM_FACTORY_UPD_RUNDATA_FILE
					unReturnValue = RC_E_RESUME_RUNDATA_NOT_FOUND;
					ERROR_STORE_FMT(unReturnValue, L"File '%s' is missing. This file is required to resume firmware update in interrupted firmware mode.", TPM_FACTORY_UPD_RUNDATA_FILE);
					break;
				}
			}

			PpTpmUpdate->unReturnCode = RC_SUCCESS;
			unReturnValue = RC_SUCCESS;
		}
	}
	WHILE_FALSE_END;

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

	return unReturnValue;
}
